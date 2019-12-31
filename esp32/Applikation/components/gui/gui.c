#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <u8g2.h>

#include "sdkconfig.h"
#include "u8g2_esp32_hal.h"

// SDA - GPIO4
#define PIN_SDA 4

// SCL - GPIO15
#define PIN_SCL 15

// Reset Pin GPIO16
#define PIN_RST 16

static const char *TAG = "GUI";

typedef struct
{
    int eDataID;
    int32_t lDataValue;
} GuiEvent_t;

void task_gui(void *EventQueue)
{
    QueueHandle_t xGuiEventQueue = EventQueue;
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.sda = PIN_SDA;
    u8g2_esp32_hal.scl = PIN_SCL;
    u8g2_esp32_hal.reset = PIN_RST;
    u8g2_esp32_hal_init(u8g2_esp32_hal);

    u8g2_t u8g2; // a structure which will contain all the data for one display
    u8g2_Setup_ssd1306_i2c_128x32_univision_f(
        &u8g2,
        U8G2_R0,
        //u8x8_byte_sw_i2c,
        u8g2_esp32_i2c_byte_cb,
        u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);

    ESP_LOGI(TAG, "u8g2_InitDisplay");
    u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,

    ESP_LOGI(TAG, "u8g2_SetPowerSave");
    u8g2_SetPowerSave(&u8g2, 0); // wake up display
    ESP_LOGI(TAG, "u8g2_ClearBuffer");
    u8g2_ClearBuffer(&u8g2);

    ESP_LOGI(TAG, "u8g2_SetFont");

    // u8g2_SetFont(&u8g2, u8g2_font_ncenB12_tr);
    u8g2_SetFont(&u8g2, u8g2_font_crox1c_tf);
    ESP_LOGI(TAG, "u8g2_DrawDefaultStr");
    u8g2_DrawStr(&u8g2, 0, 16, "T1:");
    u8g2_DrawStr(&u8g2, 0, 32, "T2:");
    ESP_LOGI(TAG, "u8g2_SendBuffer");
    u8g2_SendBuffer(&u8g2);

    ESP_LOGI(TAG, "Initialization done!");
    for (;;)
    {
        GuiEvent_t lReceivedValue;
        xQueueReceive(xGuiEventQueue, &lReceivedValue, portMAX_DELAY);
        ESP_LOGI(TAG, "Event Arrived");
        char str[4];
        sprintf(str, "%d", lReceivedValue.lDataValue);
        u8g2_ClearBuffer(&u8g2);
        if (lReceivedValue.eDataID == 0)
        {
            u8g2_DrawStr(&u8g2, 32, 16, str);
            u8g2_UpdateDisplayArea(&u8g2, 4, 0, 4, 2);
        }
        else if (lReceivedValue.eDataID == 1)
        {
            u8g2_DrawStr(&u8g2, 32, 32, str);
            u8g2_UpdateDisplayArea(&u8g2, 4, 2, 4, 2);
        }
        else
        {
            ESP_LOGI(TAG, "Gui Event no Known");
        }
    }

    vTaskDelete(NULL);
}
