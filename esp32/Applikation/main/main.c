/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "freertos/"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "../components/gui/gui.c"
#include "../components/send_to_server/send_to_server.c"
#include "../components/ntc_sensor/ntc_sensor.c"

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

void app_main(void)
{
    // initialize Event Queues
    QueueHandle_t xGuiEventQueue;
    xGuiEventQueue = xQueueCreate(5, sizeof(GuiEvent_t));

    QueueHandle_t xWifiSendEventQueue;
    xWifiSendEventQueue = xQueueCreate(5, sizeof(WifiSendEvent_t));

    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
           CHIP_NAME,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Start display\n");
    xTaskCreate(task_gui, "GUI",
                100000,
                xGuiEventQueue, /* Parameter passed into the task. */
                tskIDLE_PRIORITY + 2,
                NULL);
    xTaskCreate(task_send_to_server, "SendToServer",
                100000,
                xWifiSendEventQueue, /* Parameter passed into the task. */
                tskIDLE_PRIORITY + 1,
                NULL);
    xTaskCreate(task_ntc_sensor, "NTC",
                10000,
                xWifiSendEventQueue, /* Parameter passed into the task. */
                tskIDLE_PRIORITY + 3,
                NULL);

    printf("Display Test\n");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    for (int32_t i = 400; i >= 0; i = i - 20)
    {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        GuiEvent_t guiEvent;
        guiEvent.lDataValue = i;
        guiEvent.eDataID = GUI_TEMP2_EVENT;
        xQueueSendToBack(xGuiEventQueue, &guiEvent, 0);

        WifiSendEvent_t wifiEvent;
        wifiEvent.eSensorId = SENSOR_1;
        struct timeval te; 
        gettimeofday(&te, NULL);
        wifiEvent.lTimestamp = te.tv_sec*1000LL + te.tv_usec/1000;
        wifiEvent.lValue = (double)i;
        xQueueSendToBack(xWifiSendEventQueue, &wifiEvent, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        guiEvent.eDataID = GUI_TEMP1_EVENT;
        xQueueSendToBack(xGuiEventQueue, &guiEvent, 0);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    for(;;){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    };
    esp_restart();
}
