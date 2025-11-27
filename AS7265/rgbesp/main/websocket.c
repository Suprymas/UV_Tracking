#include "websocket.h"
#include "wifi.h"
#include "as7265x.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WS_URI "ws://10.98.101.51:8765"

static const char *TAG = "WS";
static esp_websocket_client_handle_t client = NULL;
volatile bool debug_enabled = false;
TaskHandle_t debug_task_handle = NULL;


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

                    if (!strcmp(action->valuestring, "read_sensor") && !debug_enabled) {
                        send_sensor_data();
                    }

                    if (!strcmp(action->valuestring, "debug_on")) {
                        debug_enabled = true;
                    }

                    if (!strcmp(action->valuestring, "debug_off")) {
                        debug_enabled = false;
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

    float sumCh[18] = {0};
    //We are going to measure ten times to get the valid data
    for (int i = 0; i < 10; i++) {
        int idx = 0;

        for (uint8_t dev = 0; dev < 3; dev++) {
        as7265x_set_device(dev);
            for (uint8_t base = 0x14; base < 0x2C; base += 4) {
                float value;
                if (as7265x_read_calibrated_value(base, &value) != ESP_OK) {
                    ESP_LOGE("AS7265X", "Read failed dev=%d chan=%d", dev, idx % 6);
                    value = 0.0f; // fallback
                }
                sumCh[idx++] += value; 
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    for (int i = 0; i < 18; i++) {
        sumCh[i] /= 10.0f;
    }
    

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "sensor");

    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < 18; i++) {
        cJSON_AddItemToArray(arr, cJSON_CreateNumber(sumCh[i]));
    }
    cJSON_AddItemToObject(root, "readings", arr);

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

void debug_task(void *pvParameters)
{
    while (1)
    {
        if (debug_enabled && esp_websocket_client_is_connected(client))
        {
            float ch[18];
            int idx = 0;

            for (uint8_t dev = 0; dev < 3; dev++)
            {
                as7265x_set_device(dev);
                for (uint8_t base = 0x14; base < 0x2C; base += 4)
                {
                    if (as7265x_read_calibrated_value(base, &ch[idx]) != ESP_OK)
                    {
                        ch[idx] = 0.0f; // fallback
                    }
                    idx++;
                }
            }

            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", "sensor");
            cJSON_AddStringToObject(root, "mode", "debug");

            cJSON *arr = cJSON_CreateArray();
            for (int i = 0; i < 18; i++)
            {
                cJSON_AddItemToArray(arr, cJSON_CreateNumber(ch[i]));
            }
            cJSON_AddItemToObject(root, "readings", arr);

            char *json = cJSON_PrintUnformatted(root);
            if (json)
            {
                esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
                free(json);
            }
            cJSON_Delete(root);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
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

    if (debug_task_handle == NULL)
    {
        xTaskCreate(debug_task, "debug_task", 4096, NULL, 5, &debug_task_handle);
    }

    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);

}
