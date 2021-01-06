/* ADC2 Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_system.h"
#include "../send_to_server/send_to_server.c"
#include "../gui/gui.c"

#define SENSOR_ADC_CHANNEL_1 6
#define SENSOR_ADC_CHANNEL_2 7
#define DEFAULT_VREF    1100

static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static esp_adc_cal_characteristics_t *adc_chars;
static const char *NTC_TAG = "NTC";
// Model Parameter
static const double R_N = 103000.00020001;
static const double T_N = 299.11105642;
static const double U_ges = 3.3;
static const double B = 4173.7039348;
static const double R_1 = 24.9e3;

typedef struct
{
    QueueHandle_t guiEvent;
    QueueHandle_t wifiSendEvent;
} GuiWifiQueues_t;

double tempFromMilliVolt(double voltage_m)
{
    double U_meas = voltage_m * 1e-3;
    double R_NTC = U_meas * R_1 / (U_ges - U_meas);
    double T_kelvin = 1 / (log(R_NTC / R_N) / B + 1 / T_N);
    return T_kelvin - 273.15;
}

void task_ntc_sensor(void *EventQueue)
{
    GuiWifiQueues_t * queues;
    queues = EventQueue;
    ESP_LOGI(NTC_TAG, "NTC Task Started");
    QueueHandle_t xWifiSendEventQueue = queues->wifiSendEvent;
    QueueHandle_t xGuiEventQueue = queues->guiEvent;
    
    gpio_num_t adc_gpio_num_1, adc_gpio_num_2;
    esp_err_t r;

    r = adc1_pad_get_io_num(SENSOR_ADC_CHANNEL_1, &adc_gpio_num_1);
    assert(r == ESP_OK);
    r = adc1_pad_get_io_num(SENSOR_ADC_CHANNEL_2, &adc_gpio_num_2);
    assert(r == ESP_OK);
    
    // init
    adc1_config_channel_atten(SENSOR_ADC_CHANNEL_1, atten);
    adc1_config_channel_atten(SENSOR_ADC_CHANNEL_2, atten);
    adc1_config_width(width);

    // calibration
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, atten, width, DEFAULT_VREF, adc_chars);
    
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    ESP_LOGI(NTC_TAG, "ADC initialized. Start reading.");
    double sensor_value_mean_1 = 0;
    double sensor_value_mean_2 = 0;

    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 10 / portTICK_PERIOD_MS;

     // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    WifiSendEvent_t wifiEvent_sensor;
    GuiEvent_t gui_event_s1;
    GuiEvent_t gui_event_s2;
    gui_event_s1.eDataID = GUI_TEMP1_EVENT;
    gui_event_s2.eDataID = GUI_TEMP2_EVENT;
    
    for (int i = 0;; i++)
    {   
        uint32_t reading =  adc1_get_raw(SENSOR_ADC_CHANNEL_1);
        reading = esp_adc_cal_raw_to_voltage(reading, adc_chars);
        sensor_value_mean_1 = sensor_value_mean_1 + 0.1 * ((double)reading - sensor_value_mean_1);
        reading =  adc1_get_raw(SENSOR_ADC_CHANNEL_2);
        reading = esp_adc_cal_raw_to_voltage(reading, adc_chars);
        sensor_value_mean_2 = sensor_value_mean_2 + 0.1 * ((double)reading - sensor_value_mean_2);
        
        if ((i % 50) == 0){
            gui_event_s1.lDataValue = tempFromMilliVolt(sensor_value_mean_1);
            gui_event_s2.lDataValue = tempFromMilliVolt(sensor_value_mean_2);
            xQueueSendToBack(xGuiEventQueue, &gui_event_s1, 0);
            xQueueSendToBack(xGuiEventQueue, &gui_event_s2, 0);
        }

        if (i >= 500)
        {
            // TODO(peter) send both data values together
            struct timeval te; 
            gettimeofday(&te, NULL);
            long long ts_ms = te.tv_sec*1000LL + te.tv_usec/1000;
            wifiEvent_sensor.lTimestamp = ts_ms;
            wifiEvent_sensor.lValueSensor1 = sensor_value_mean_1;
            wifiEvent_sensor.lValueSensor2 = sensor_value_mean_2;
            xQueueSendToBack(xWifiSendEventQueue, &wifiEvent_sensor, 0);
           
            i = 0;
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}