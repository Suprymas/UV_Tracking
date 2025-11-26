#include "websocket.h"
#include "wifi.h"

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WS_URI "ws://10.31.204.51:8765"

static const char *TAG = "WS";
static esp_websocket_client_handle_t client = NULL;

static void handle_incoming_message(const char *data, int len)
{
    char *msg = strndup(data, len);
    if (!msg) return;

    ESP_LOGI(TAG, "Received: %s", msg);

    cJSON *root = cJSON_Parse(msg);
    if (root) {
        cJSON *type = cJSON_GetObjectItem(root, "type");

        if (cJSON_IsString(type)) {
            if (strcmp(type->valuestring, "command") == 0) {
                cJSON *action = cJSON_GetObjectItem(root, "action");
                if (cJSON_IsString(action)) {
                    ESP_LOGI(TAG, "Command: %s", action->valuestring);

                    if (!strcmp(action->valuestring, "read_sensor")) {
                        send_sensor_data();
                    }
                }
            }
        }
        cJSON_Delete(root);
    }
    free(msg);
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                    int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected");
            send_status("ESP connected");
            break;

        case WEBSOCKET_EVENT_DATA:
            if (data->op_code == 0x1)
                handle_incoming_message(data->data_ptr, data->data_len);
            break;

        default:
            break;
    }
}

void send_sensor_data(void)
{
    if (!esp_websocket_client_is_connected(client)) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "sensor");
    cJSON_AddNumberToObject(root, "temperature", 20.0);
    cJSON_AddNumberToObject(root, "timestamp", esp_timer_get_time() / 1000);

    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
        free(json);
    }
    cJSON_Delete(root);
}

static void sensor_task(void *pvParameters)
{
    while (1) {
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

        if (esp_websocket_client_is_connected(client)) {
            send_sensor_data();
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void send_status(const char *message)
{
    if (!esp_websocket_client_is_connected(client)) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "status");
    cJSON_AddStringToObject(root, "message", message);

    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
        free(json);
    }
    cJSON_Delete(root);
}

void websocket_start(void)
{
    esp_websocket_client_config_t cfg = {
        .uri = WS_URI,
        .reconnect_timeout_ms = 5000
    };

    client = esp_websocket_client_init(&cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY,
                                  websocket_event_handler, NULL);

    esp_websocket_client_start(client);

    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}
