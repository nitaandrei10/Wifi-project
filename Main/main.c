#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "HD44780.h"

#define WIFI_SSID "WIFI_SSID"
#define PASSWORD "WIFI_PASSWORD"
#define BUFFER_SIZE 2048

static const char *TAG = "wifi_demo";

static const uint8_t SDA_PIN = 19;
static const uint8_t SCL_PIN = 18;
static const uint8_t COLS = 16;
static const u_int8_t ROWS = 2;
static const u_int8_t addr = 0x27;

// HTTP event handler to process incoming chunks
void parse_and_display(const char *json_raw)
{
    cJSON *json = cJSON_Parse(json_raw);
    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("Error :%s\n", error_ptr);
        }
        cJSON_Delete(json);
        return;
    }
    cJSON *current_price = cJSON_GetObjectItemCaseSensitive(json, "c");

    if (cJSON_IsNumber(current_price))
    {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "Price: $%.2f", current_price->valuedouble);
        LCD_setCursor(0, 1);
        LCD_writeStr(buffer);
    }

    cJSON_Delete(json);
}
void http_task_loop(void *Param)
{
    char *buffer = (char *)malloc(BUFFER_SIZE);
    if (buffer == NULL)
    {
        printf("Failed to allocate memory\n");
        vTaskDelete(NULL);
    }
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_http_client_config_t config = {
            .url = "https://finnhub.io/api/v1/quote?symbol=NXPI&token=dag4q61r01quf8mslatgdag4q61r01quf8mslau0",
            .crt_bundle_attach = esp_crt_bundle_attach,
            .timeout_ms = 5000,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_FAIL)
        {
            printf("Failed to open http connectionL %s\n", esp_err_to_name(err));
        }
        else
        {
            esp_http_client_fetch_headers(client);

            int data_read = esp_http_client_read_response(client, buffer, BUFFER_SIZE - 1);
            if (data_read >= 0)
            {
                buffer[data_read] = '\0';
                parse_and_display(buffer);
            }
        }
        esp_http_client_cleanup(client);
    }
}

void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}
void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Disconnected from wifi, trying again...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xTaskCreate(http_task_loop, "Http loop", 8192, NULL, 1, NULL);
    }
}
void app_main(void)
{
    init_nvs();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    LCD_init(addr, SDA_PIN, SCL_PIN, COLS, ROWS);
    LCD_setCursor(0, 0);
    LCD_clearScreen();
    LCD_writeStr("NXP PRICE");

    // Register WiFi event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    // Register IP event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // Set WiFi configuration
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}