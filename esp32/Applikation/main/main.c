/*
  Main Application
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "freertos/"
#include "esp_system.h"
#include "esp_spi_flash.h"
// #include "../components/gui/gui.c"
// #include "../components/send_to_server/send_to_server.c"
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
    xWifiSendEventQueue = xQueueCreate(20, sizeof(WifiSendEvent_t));

    GuiWifiQueues_t xGuiWifiQueues;
    xGuiWifiQueues.guiEvent = xGuiEventQueue;
    xGuiWifiQueues.wifiSendEvent = xWifiSendEventQueue;

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
                &xGuiWifiQueues, /* Parameter passed into the task. */
                tskIDLE_PRIORITY + 3,
                NULL);

    for(;;){
        vTaskDelay(100000 / portTICK_PERIOD_MS);
    };
    esp_restart();
}
