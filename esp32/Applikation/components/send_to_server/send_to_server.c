#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <nvs_flash.h>

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_sntp.h"

#include "wifi_config.h"
#define MAX_HTTP_RECV_BUFFER 512

#define EXAMPLE_ESP_MAXIMUM_RETRY 10

typedef enum
{
    NTC_SENSOR_HIGH,
    NTC_SENSOR_LOW,
    NTC_SENSOR_OK
} NtcSensorState_t;

typedef struct
{
    long long lTimestamp;
    double lValueSensor1;
    NtcSensorState_t lSensorState1;
    double lValueSensor2;
    NtcSensorState_t lSensorState2;
} WifiSendEvent_t;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

// static const char *SendToServer_TAG = "wifi station";
static const char *SendToServer_TAG = "SendToServer";

static int s_retry_num = 0;
esp_err_t wifi_connect_ret = ESP_ERR_WIFI_CONN;
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        wifi_connect_ret = esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            wifi_connect_ret = esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            s_retry_num++;
            ESP_LOGI(SendToServer_TAG, "retry to connect to the AP");
        }
        ESP_LOGI(SendToServer_TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(SendToServer_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS},
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(SendToServer_TAG, "wifi_init_sta finished.");
    ESP_LOGI(SendToServer_TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void wifi_sendViaHttp(WifiSendEvent_t sendEvent)
{
    esp_http_client_config_t config = {
        .url = "http://192.168.2.106:5000/input"};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    // Format value
    char value_string_1[16];
    snprintf(value_string_1, 16, "%.2f", sendEvent.lValueSensor1);
    char value_string_2[16];
    snprintf(value_string_2, 16, "%.2f", sendEvent.lValueSensor2);
    // Format Time
    bool time_valid = false;
    char time_string[40];
    if (sendEvent.lTimestamp > 1000000000)
    {
        snprintf(time_string, 40, "%lld", sendEvent.lTimestamp);
        time_valid = true;
    }

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    char data[160] = "";
    if (sendEvent.lSensorState1 == NTC_SENSOR_OK)
    {
        strcat(data, "Measurement,device=TempSens,Sensor=1 value=");
        strcat(data, value_string_1);
        if (time_valid)
        {
            strcat(data, " ");
            strcat(data, time_string);
            strcat(data, "000");
        }
        strcat(data, "\n");
    }

    if (sendEvent.lSensorState2 == NTC_SENSOR_OK)
    {
        strcat(data, "Measurement,device=TempSens,Sensor=2 value=");
        strcat(data, value_string_2);
        if (time_valid)
        {
            strcat(data, " ");
            strcat(data, time_string);
            strcat(data, "000");
        }
    }

    esp_http_client_set_post_field(client, data, strlen(data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(SendToServer_TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        ESP_LOGI(SendToServer_TAG, "SendToServer Data: %s", data);
    }
    else
    {
        ESP_LOGE(SendToServer_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void wifi_init_sntp()
{
    ESP_LOGI(SendToServer_TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();
}

void task_send_to_server(void *EventQueue)
{
    QueueHandle_t xWifiSendEventQueue = EventQueue;
    ESP_LOGI(SendToServer_TAG, "SendToServer started");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(SendToServer_TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    ESP_LOGI(SendToServer_TAG, "Push to server");
    wifi_init_sntp();
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    for (;;)
    {
        WifiSendEvent_t lWifiSendEvent;

        if (!xQueueReceive(xWifiSendEventQueue, &lWifiSendEvent, portMAX_DELAY))
        {
            ESP_LOGI(SendToServer_TAG, "Queue Time Out");
            continue;
        }
        // TODO: Check if wifi connected and send properly
        if ( wifi_connect_ret == ESP_OK && (lWifiSendEvent.lSensorState1 == NTC_SENSOR_OK || lWifiSendEvent.lSensorState2 == NTC_SENSOR_OK))
        {
            wifi_sendViaHttp(lWifiSendEvent);
        }
    }

    vTaskDelete(NULL);
}
