/*
 * ESP32-C3 WebSocket Client using ESP-IDF
 * main/main.c
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_websocket_client.h"
#include "cJSON.h"

// WiFi Configuration
#define WIFI_SSID      "Internetas"
#define WIFI_PASS      "12345678"

// WebSocket Configuration
#define WS_URI         "ws://10.31.204.51:8765"

static const char *TAG = "WS_CLIENT";
static esp_websocket_client_handle_t client = NULL;

// WiFi event group
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from WiFi, reconnecting...");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Initialize WiFi
void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization complete");
}

// Send sensor data
void send_sensor_data(void)
{
    if (!esp_websocket_client_is_connected(client)) {
        ESP_LOGW(TAG, "WebSocket not connected, skipping send");
        return;
    }

    // Create JSON object
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "sensor");
    cJSON_AddNumberToObject(root, "temperature", 20.0 + (esp_random() % 100) / 10.0);
    cJSON_AddNumberToObject(root, "humidity", 40.0 + (esp_random() % 300) / 10.0);
    cJSON_AddNumberToObject(root, "timestamp", esp_timer_get_time() / 1000);

    char *json_string = cJSON_PrintUnformatted(root);
    
    if (json_string) {
        ESP_LOGI(TAG, "Sending: %s", json_string);
        esp_websocket_client_send_text(client, json_string, strlen(json_string), portMAX_DELAY);
        free(json_string);
    }
    
    cJSON_Delete(root);
}

// Send status message
void send_status(const char *message)
{
    if (!esp_websocket_client_is_connected(client)) {
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "status");
    cJSON_AddStringToObject(root, "message", message);
    cJSON_AddNumberToObject(root, "timestamp", esp_timer_get_time() / 1000);

    char *json_string = cJSON_PrintUnformatted(root);
    
    if (json_string) {
        ESP_LOGI(TAG, "Status: %s", message);
        esp_websocket_client_send_text(client, json_string, strlen(json_string), portMAX_DELAY);
        free(json_string);
    }
    
    cJSON_Delete(root);
}

// Handle incoming WebSocket messages
void handle_incoming_message(const char *data, int len)
{
    char *buffer = malloc(len + 1);
    if (!buffer) return;
    
    memcpy(buffer, data, len);
    buffer[len] = '\0';
    
    ESP_LOGI(TAG, "Received: %s", buffer);
    
    cJSON *root = cJSON_Parse(buffer);
    if (root) {
        cJSON *type = cJSON_GetObjectItem(root, "type");
        
        if (type && cJSON_IsString(type)) {
            if (strcmp(type->valuestring, "command") == 0) {
                cJSON *action = cJSON_GetObjectItem(root, "action");
                if (action && cJSON_IsString(action)) {
                    ESP_LOGI(TAG, "Command received: %s", action->valuestring);
                    
                    if (strcmp(action->valuestring, "read_sensor") == 0) {
                        send_sensor_data();
                    } else if (strcmp(action->valuestring, "led_on") == 0) {
                        ESP_LOGI(TAG, "LED ON");
                        // Add your LED control code here
                    } else if (strcmp(action->valuestring, "led_off") == 0) {
                        ESP_LOGI(TAG, "LED OFF");
                        // Add your LED control code here
                    }
                }
            } else if (strcmp(type->valuestring, "ack") == 0) {
                ESP_LOGI(TAG, "Acknowledgment received");
            }
        }
        
        cJSON_Delete(root);
    }
    
    free(buffer);
}

// WebSocket event handler
static void websocket_event_handler(void *handler_args, esp_event_base_t base, 
                                     int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket connected");
            send_status("ESP32-C3 connected");
            break;
            
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WebSocket disconnected");
            break;
            
        case WEBSOCKET_EVENT_DATA:
            if (data->op_code == 0x01) {  // Text frame
                handle_incoming_message(data->data_ptr, data->data_len);
            }
            break;
            
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "WebSocket error");
            break;
            
        default:
            break;
    }
}

// Task to send periodic sensor data
void sensor_task(void *pvParameters)
{
    while (1) {
        // Wait for WiFi and WebSocket connection
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        
        if (esp_websocket_client_is_connected(client)) {
            send_sensor_data();
        }
        
        vTaskDelay(5000 / portTICK_PERIOD_MS);  // Send every 5 seconds
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32-C3 WebSocket Client ===");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize WiFi
    wifi_init_sta();
    
    // Wait for WiFi connection
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    
    // Configure WebSocket client
    esp_websocket_client_config_t websocket_cfg = {
        .uri = WS_URI,
        .reconnect_timeout_ms = 5000,
        .network_timeout_ms = 10000,
    };
    
    ESP_LOGI(TAG, "Connecting to %s", WS_URI);
    
    client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);
    
    esp_websocket_client_start(client);
    
    // Create sensor data task
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Application started");
}