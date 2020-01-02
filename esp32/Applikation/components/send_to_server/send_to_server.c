#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *SendToServer_TAG = "SendToServer";
void task_send_to_server(void *ignore)
{
    ESP_LOGI(SendToServer_TAG, "SendToServer started");
    for (;;)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    vTaskDelete(NULL);
}