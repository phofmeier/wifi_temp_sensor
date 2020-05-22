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

#define ADC2_EXAMPLE_CHANNEL 6
#define ADC2_EXAMPLE_CHANNEL_2 7

static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const char *NTC_TAG = "NTC";

void task_ntc_sensor(void *ignore)
{
    ESP_LOGI(NTC_TAG, "NTC Task Started");
    uint8_t output_data = 0;
    int read_raw;
    esp_err_t r;

    gpio_num_t adc_gpio_num;

    r = adc1_pad_get_io_num(ADC2_EXAMPLE_CHANNEL, &adc_gpio_num);
    printf(" Return: %d, GPIONUM: %d", r, adc_gpio_num);

    assert(r == ESP_OK);

    printf("ADC2 channel %d @ GPIO %d.\n", ADC2_EXAMPLE_CHANNEL, adc_gpio_num);

    //be sure to do the init before using adc2.
    printf("adc2_init...\n");
    adc1_config_channel_atten(ADC2_EXAMPLE_CHANNEL, ADC_ATTEN_0db);
    adc1_config_width(width);

    vTaskDelay(2 * portTICK_PERIOD_MS);

    printf("start conversion.\n");
    for (;;)
    {
        read_raw = adc1_get_raw(ADC2_EXAMPLE_CHANNEL);
        printf("%d: %d\n", output_data, read_raw);
        
        vTaskDelay(2 * portTICK_PERIOD_MS);
    }
}