#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include "esp_wifi.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <u8g2.h>
#include <time.h>
#include <sys/time.h>

#include "sdkconfig.h"
#include "u8g2_esp32_hal.h"
#define U8G2_16BIT
// SDA - GPIO4
#define PIN_SDA 4

// SCL - GPIO15
#define PIN_SCL 15

// Reset Pin GPIO16
#define PIN_RST 16

static const char *TAG = "GUI";

typedef enum
{
    GUI_TEMP1_EVENT,
    GUI_TEMP2_EVENT
} GuiEventID_t;

typedef struct
{
    GuiEventID_t eDataID;
    int32_t lDataValue;
} GuiEvent_t;


const int MIN_RSSI = -100;
/** Anything better than or equal to this will show the max bars. */
const int MAX_RSSI = -55;

 int calculateSignalLevel(int rssi, int numLevels) {
  if(rssi <= MIN_RSSI) {
    return 0;
  } else if (rssi >= MAX_RSSI) {
    return numLevels - 1;
  } else {
    float inputRange = (MAX_RSSI -MIN_RSSI);
    float outputRange = (numLevels - 1);
    return (int)((float)(rssi - MIN_RSSI) * outputRange / inputRange);
  }
}

void task_gui(void *EventQueue)
{
    QueueHandle_t xGuiEventQueue = EventQueue;
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.sda = PIN_SDA;
    u8g2_esp32_hal.scl = PIN_SCL;
    u8g2_esp32_hal.reset = PIN_RST;
    u8g2_esp32_hal_init(u8g2_esp32_hal);

    u8g2_t u8g2; // a structure which will contain all the data for one display
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
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
    u8g2_DrawUTF8(&u8g2, 0, 16, "T1:     °C");
    u8g2_DrawUTF8(&u8g2, 0, 32, "T2:     °C");
    ESP_LOGI(TAG, "u8g2_SendBuffer");
    u8g2_SendBuffer(&u8g2);

    ESP_LOGI(TAG, "Initialization done!");
    for (;;)
    {
        u8g2_ClearBuffer(&u8g2);
        // Format Time
        struct timeval tv;
        gettimeofday(&tv, NULL); // get current time
        time_t nowtime = tv.tv_sec;
        struct tm *nowtm = localtime(&nowtime);
        char time_string[26];
        strftime(time_string, sizeof time_string, "%d.%m.%Y %H:%M", nowtm);
        u8g2_DrawStr(&u8g2, 0, 64, time_string);
        //u8g2_UpdateDisplayArea(&u8g2, 0, 6, 16, 2);

        wifi_ap_record_t ap_info;
        esp_err_t error = esp_wifi_sta_get_ap_info(&ap_info);
        if (error == ESP_OK)
        {   
            char connection_string[26];
            int wifi_rssi_level = calculateSignalLevel(ap_info.rssi, 5);
            snprintf(connection_string, 26, "Wifi rssi: %i", wifi_rssi_level);
            u8g2_DrawStr(&u8g2, 0, 48, connection_string);
        }
        u8g2_UpdateDisplayArea(&u8g2, 0, 4, 16, 4);

        GuiEvent_t lReceivedValue;
        if (!xQueueReceive(xGuiEventQueue, &lReceivedValue, pdMS_TO_TICKS(1000)))
        {
            ESP_LOGI(TAG, "Queue Time Out");
            continue;
        };
        ESP_LOGI(TAG, "Event Arrived");
        char str[4] = "---";
        if (lReceivedValue.lDataValue != 0)
        {
            sprintf(str, "%*d", 3, lReceivedValue.lDataValue);
        }

        if (lReceivedValue.eDataID == GUI_TEMP1_EVENT)
        {
            u8g2_DrawStr(&u8g2, 32, 16, str);
            u8g2_UpdateDisplayArea(&u8g2, 4, 0, 4, 2);
        }
        else if (lReceivedValue.eDataID == GUI_TEMP2_EVENT)
        {
            u8g2_DrawStr(&u8g2, 32, 32, str);
            u8g2_UpdateDisplayArea(&u8g2, 4, 2, 4, 2);
        }
        else
        {
            ESP_LOGI(TAG, "Gui Event not Known");
        }
    }

    vTaskDelete(NULL);
}
