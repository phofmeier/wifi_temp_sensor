/* ADC2 Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_system.h"
#include "../send_to_server/send_to_server.c"
#include "../gui/gui.c"

#define SENSOR_ADC_CHANNEL_1 6
#define SENSOR_ADC_CHANNEL_2 7

static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const char *NTC_TAG = "NTC";

typedef struct
{
    QueueHandle_t guiEvent;
    QueueHandle_t wifiSendEvent;
} GuiWifiQueues_t;

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
    adc1_config_channel_atten(SENSOR_ADC_CHANNEL_1, ADC_ATTEN_0db);
    adc1_config_channel_atten(SENSOR_ADC_CHANNEL_2, ADC_ATTEN_0db);
    adc1_config_width(width);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

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
        sensor_value_mean_1 = sensor_value_mean_1 + 0.1 * ((double)reading - sensor_value_mean_1);
        reading =  adc1_get_raw(SENSOR_ADC_CHANNEL_2);
        sensor_value_mean_2 = sensor_value_mean_2 + 0.1 * ((double)reading - sensor_value_mean_2);
        
        if ((i % 50) == 0){
            gui_event_s1.lDataValue = sensor_value_mean_1;
            gui_event_s2.lDataValue = sensor_value_mean_2;
            xQueueSendToBack(xGuiEventQueue, &gui_event_s1, 0);
            xQueueSendToBack(xGuiEventQueue, &gui_event_s2, 0);
        }

        if (i >= 1000)
        {
            // TODO(peter) send both data values together
            struct timeval te; 
            gettimeofday(&te, NULL);
            long long ts_ms = te.tv_sec*1000LL + te.tv_usec/1000;
            wifiEvent_sensor.lTimestamp = ts_ms;
            wifiEvent_sensor.lValueSensor1 = sensor_value_mean_1;
            wifiEvent_sensor.lValueSensor2 = sensor_value_mean_2;
            xQueueSendToBack(xWifiSendEventQueue, &wifiEvent_sensor, 0);
           
            printf("ts : %lld S_1: %f S_2: %f\n", ts_ms, sensor_value_mean_1, sensor_value_mean_2);
            i = 0;
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}