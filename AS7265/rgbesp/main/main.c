/*
 * ESP32-C3 WebSocket Client with AS7265X Spectral Sensor using ESP-IDF
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
#include "driver/i2c_master.h" //I2C driver header file for ESP-IDF.
#include "cJSON.h"

// WiFi Configuration
#define WIFI_SSID      "Internetas"
#define WIFI_PASS      "12345678"

// WebSocket Configuration
#define WS_URI         "ws://10.31.204.51:8765"

// I2C Configuration (ESP32-C3 example pins)
#define I2C_MASTER_SDA_IO     8  //I2C SDA pin number.
#define I2C_MASTER_SCL_IO     9  //I2C SCL pin number.
#define I2C_MASTER_NUM        I2C_NUM_0  //I2C port number.
#define I2C_MASTER_FREQ_HZ    400000  //I2C clock frequency.
#define I2C_MASTER_TX_BUF_DISABLE   0  //I2C transmit buffer disable.
#define I2C_MASTER_RX_BUF_DISABLE   0  //I2C receive buffer disable.

// AS7265X I2C Address
#define AS7265X_ADDRESS       0x49

// AS7265X Register Addresses
#define AS7265X_SLAVE_STATUS_REG     0x00
#define AS7265X_WRITE_REG            0x01
#define AS7265X_READ_REG             0x02
#define AS7265X_TX_VALID             0x02  // Bit 1 in SLAVE_STATUS_REG
#define AS7265X_RX_VALID             0x01  // Bit 0 in SLAVE_STATUS_REG
#define AS7265X_VIRTUAL_REG_WRITE    0x80  // Bit 7 set = write operation
#define AS7265X_VIRTUAL_REG_READ     0x00  // Bit 7 clear = read operation

// Virtual Registers (accessed through WRITE_REG/READ_REG)
#define AS7265X_CONTROL_SETUP        0x04
#define AS7265X_INT_T                0x05
#define AS7265X_DEVICE_TEMP          0x06
#define AS7265X_LED_CONTROL          0x07

// Calibrated Data Registers
#define AS7265X_CAL_A                0x14
#define AS7265X_CAL_B                0x15
#define AS7265X_CAL_C                0x16
#define AS7265X_CAL_D                0x17
#define AS7265X_CAL_E                0x18
#define AS7265X_CAL_F                0x19
#define AS7265X_CAL_G                0x1A
#define AS7265X_CAL_H                0x1B
#define AS7265X_CAL_R                0x1C
#define AS7265X_CAL_I                0x1D
#define AS7265X_CAL_S                0x1E
#define AS7265X_CAL_J                0x1F
#define AS7265X_CAL_T                0x20
#define AS7265X_CAL_U                0x21
#define AS7265X_CAL_V                0x22
#define AS7265X_CAL_W                0x23
#define AS7265X_CAL_K                0x24
#define AS7265X_CAL_L                0x25

// Control Setup bits
#define AS7265X_CONTROL_GAIN_MASK    0x30
#define AS7265X_CONTROL_GAIN_1X      0x00
#define AS7265X_CONTROL_GAIN_3X7     0x10
#define AS7265X_CONTROL_GAIN_16X     0x20
#define AS7265X_CONTROL_GAIN_64X     0x30
#define AS7265X_CONTROL_RESET        0x80

// LED Control bits
#define AS7265X_LED_INDICATOR_CURRENT_MASK    0x07
#define AS7265X_LED_DRIVER_CURRENT_MASK      0x38
#define AS7265X_LED_INDICATOR_ENABLE         0x08
#define AS7265X_LED_DRIVER_ENABLE            0x40

static const char *TAG = "AS7265X_WS";
static esp_websocket_client_handle_t client = NULL;
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t as7265x_handle = NULL;

// Sampling interval (in milliseconds)
static uint32_t sampling_interval_ms = 500;

// WiFi event group
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
const int WEBSOCKET_CONNECTED_BIT = BIT1;

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

// AS7265X I2C Functions
// Read SLAVE_STATUS_REG directly (not a virtual register)
static esp_err_t as7265x_read_slave_status(uint8_t *status)
{
    uint8_t reg = AS7265X_SLAVE_STATUS_REG;
    i2c_master_transmit(as7265x_handle, &reg, 1, -1);
    i2c_master_receive(as7265x_handle, status, 1, -1);
    return ESP_OK;
}

// Helper function: Wait for TX_VALID == 0
static esp_err_t wait_for_tx_ready(uint8_t *status, int timeout)
{
    int retries = 0;
    do {
        as7265x_read_slave_status(status);
        if (!(*status & AS7265X_TX_VALID)) {
            return ESP_OK; // Ready
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        retries++;
        if (retries > timeout) {
            return ESP_ERR_TIMEOUT;
        }
    } while (*status & AS7265X_TX_VALID);
    return ESP_OK;
}

// Write virtual register following AS7265X protocol:
// Typical flow: wait for TX_VALID == 0 → write → wait for TX_VALID == 0 → write → wait for TX_VALID == 0
// 1. Wait for TX_VALID == 0 (ready for write)
// 2. Write virtual register address (with bit 7 = 1 for write) to WRITE_REG
// 3. Wait for TX_VALID == 0 (address written)
// 4. Write data value to WRITE_REG
// 5. Wait for TX_VALID == 0 (data written, operation complete)
static esp_err_t as7265x_write_virtual_register(uint8_t virtual_reg, uint8_t data)
{
    uint8_t status;
    uint8_t write_buffer[2];
    int timeout = 100; // Maximum retries
    
    // Step 1: Wait for TX_VALID == 0
    if (wait_for_tx_ready(&status, timeout) != ESP_OK) {
        ESP_LOGE(TAG, "Timeout waiting for TX_VALID == 0 (before write address)");
        return ESP_ERR_TIMEOUT;
    }
    
    // Step 2: Write virtual register address to WRITE_REG (bit 7 = 1 for write)
    write_buffer[0] = AS7265X_WRITE_REG;
    write_buffer[1] = virtual_reg | AS7265X_VIRTUAL_REG_WRITE; // Set bit 7 for write
    i2c_master_transmit(as7265x_handle, write_buffer, 2, -1);
    
    // Step 3: Wait for TX_VALID == 0 (address written)
    if (wait_for_tx_ready(&status, timeout) != ESP_OK) {
        ESP_LOGE(TAG, "Timeout waiting for TX_VALID == 0 (after write address)");
        return ESP_ERR_TIMEOUT;
    }
    
    // Step 4: Write data value to WRITE_REG
    write_buffer[0] = AS7265X_WRITE_REG;
    write_buffer[1] = data;
    i2c_master_transmit(as7265x_handle, write_buffer, 2, -1);
    
    // Step 5: Wait for TX_VALID == 0 (data written, operation complete)
    if (wait_for_tx_ready(&status, timeout) != ESP_OK) {
        ESP_LOGE(TAG, "Timeout waiting for TX_VALID == 0 (after write data)");
        return ESP_ERR_TIMEOUT;
    }
    
    return ESP_OK;
}

// Helper function: Wait for RX_VALID == 1
static esp_err_t wait_for_rx_ready(uint8_t *status, int timeout)
{
    int retries = 0;
    do {
        as7265x_read_slave_status(status);
        if (*status & AS7265X_RX_VALID) {
            return ESP_OK; // Data ready
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        retries++;
        if (retries > timeout) {
            return ESP_ERR_TIMEOUT;
        }
    } while (!(*status & AS7265X_RX_VALID));
    return ESP_OK;
}

// Read virtual register following AS7265X protocol:
// Typical flow: wait for TX_VALID == 0 → write address → wait for TX_VALID == 0 → wait for RX_VALID == 1 → read
// 1. Wait for TX_VALID == 0 (ready for read request)
// 2. Write virtual register address (with bit 7 = 0 for read) to WRITE_REG
// 3. Wait for TX_VALID == 0 (address written)
// 4. Wait for RX_VALID == 1 (data ready)
// 5. Read data from READ_REG
static esp_err_t as7265x_read_virtual_register(uint8_t virtual_reg, uint8_t *data)
{
    uint8_t status;
    uint8_t write_buffer[2];
    int timeout = 100; // Maximum retries
    
    // Step 1: Wait for TX_VALID == 0
    if (wait_for_tx_ready(&status, timeout) != ESP_OK) {
        ESP_LOGE(TAG, "Timeout waiting for TX_VALID == 0 (before read)");
        return ESP_ERR_TIMEOUT;
    }
    
    // Step 2: Write virtual register address to WRITE_REG (bit 7 = 0 for read)
    write_buffer[0] = AS7265X_WRITE_REG;
    write_buffer[1] = virtual_reg & ~AS7265X_VIRTUAL_REG_WRITE; // Clear bit 7 for read
    i2c_master_transmit(as7265x_handle, write_buffer, 2, -1);
    
    // Step 3: Wait for TX_VALID == 0 (address written)
    if (wait_for_tx_ready(&status, timeout) != ESP_OK) {
        ESP_LOGE(TAG, "Timeout waiting for TX_VALID == 0 (after write read address)");
        return ESP_ERR_TIMEOUT;
    }
    
    // Step 4: Wait for RX_VALID == 1 (data ready)
    if (wait_for_rx_ready(&status, timeout) != ESP_OK) {
        ESP_LOGE(TAG, "Timeout waiting for RX_VALID == 1");
        return ESP_ERR_TIMEOUT;
    }
    
    // Step 5: Read data from READ_REG
    uint8_t read_reg = AS7265X_READ_REG;
    i2c_master_transmit(as7265x_handle, &read_reg, 1, -1);
    i2c_master_receive(as7265x_handle, data, 1, -1);
    
    return ESP_OK;
}

static esp_err_t as7265x_read_calibrated_value(uint8_t reg, float *value)
{
    uint8_t data[4];
    uint8_t reg_addr = reg;
    
    for (int i = 0; i < 4; i++) {
        as7265x_read_virtual_register(reg_addr + i, &data[i]);
    }
    
    // Convert 4 bytes to float (little-endian)
    uint32_t raw = (uint32_t)data[0] | ((uint32_t)data[1] << 8) | 
                   ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
    *value = *((float*)&raw);
    
    return ESP_OK;
}

static esp_err_t as7265x_init(void)
{
    uint8_t status;
    
    // Check if device is present by reading SLAVE_STATUS_REG directly
    as7265x_read_slave_status(&status);
    ESP_LOGI(TAG, "AS7265X SLAVE_STATUS: 0x%02X", status);
    
    // Reset device
    if (as7265x_write_virtual_register(AS7265X_CONTROL_SETUP, AS7265X_CONTROL_RESET) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset AS7265X");
        return ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Set gain to 16x
    if (as7265x_write_virtual_register(AS7265X_CONTROL_SETUP, AS7265X_CONTROL_GAIN_16X) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set gain");
        return ESP_FAIL;
    }
    
    // Set integration time (50 cycles = ~140ms)
    if (as7265x_write_virtual_register(AS7265X_INT_T, 50) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set integration time");
        return ESP_FAIL;
    }
    
    // Disable indicator LED
    if (as7265x_write_virtual_register(AS7265X_LED_CONTROL, 0x00) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LED control");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "AS7265X initialized");
    return ESP_OK;
}

static esp_err_t as7265x_take_measurement_with_bulb(void)
{
    uint8_t led_control;
    uint8_t control_setup;
    
    // Enable LED driver
    as7265x_read_virtual_register(AS7265X_LED_CONTROL, &led_control);
    led_control |= AS7265X_LED_DRIVER_ENABLE;
    as7265x_write_virtual_register(AS7265X_LED_CONTROL, led_control);
    
    // Start measurement
    // Set DATA_RDY bit (bit 1) in CONTROL_SETUP to start measurement
    // Bit 1 behavior: 1 = device busy (measuring), 0 = device idle (measurement complete)
    as7265x_read_virtual_register(AS7265X_CONTROL_SETUP, &control_setup);
    control_setup |= 0x02; // Set DATA_RDY bit (bit 1) to start measurement
    as7265x_write_virtual_register(AS7265X_CONTROL_SETUP, control_setup);
    
    // Small delay to allow hardware to process the command
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // Wait for measurement to complete
    // Bit 1 will be 1 while measuring, and cleared to 0 when measurement is complete
    // So we wait for bit 1 to become 0 (device idle)
    uint8_t status;
    int timeout = 1000; // Maximum retries (10 seconds)
    int retries = 0;
    do {
        vTaskDelay(pdMS_TO_TICKS(10));
        as7265x_read_virtual_register(AS7265X_CONTROL_SETUP, &status);
        retries++;
        if (retries > timeout) {
            ESP_LOGE(TAG, "Timeout waiting for measurement to complete (bit 1 still set)");
            return ESP_ERR_TIMEOUT;
        }
    } while (status & 0x02); // Wait for DATA_RDY bit (bit 1) to clear (0 = measurement complete)
    
    // Disable LED driver (use the saved led_control value)
    led_control &= ~AS7265X_LED_DRIVER_ENABLE;
    as7265x_write_virtual_register(AS7265X_LED_CONTROL, led_control);
    
    return ESP_OK;
}

// Send spectral data over WebSocket
void send_spectral_data(float data[18])
{
    if (!esp_websocket_client_is_connected(client)) {
        ESP_LOGW(TAG, "WebSocket not connected, skipping send");
        return;
    }

    // Create JSON object matching Arduino format
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "spectral_data");
    cJSON_AddStringToObject(root, "device_id", "esp32-as7265x");
    
    // Add channels array
    cJSON *channels = cJSON_CreateArray();
    const char* channelNames[] = {"A", "B", "C", "D", "E", "F", "G", "H", "R", "I", "S", "J", "T", "U", "V", "W", "K", "L"};
    const int wavelengths[] = {410, 435, 460, 485, 510, 535, 560, 585, 610, 645, 680, 705, 730, 760, 810, 860, 900, 940};
    
    for (int i = 0; i < 18; i++) {
        cJSON *channel = cJSON_CreateObject();
        cJSON_AddStringToObject(channel, "name", channelNames[i]);
        cJSON_AddNumberToObject(channel, "wavelength_nm", wavelengths[i]);
        cJSON_AddNumberToObject(channel, "value", data[i]);
        cJSON_AddItemToArray(channels, channel);
    }
    
    cJSON_AddItemToObject(root, "channels", channels);
    cJSON_AddNumberToObject(root, "timestamp", esp_timer_get_time() / 1000);

    char *json_string = cJSON_PrintUnformatted(root);
    
    if (json_string) {
        ESP_LOGI(TAG, "Sending spectral data");
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
                        // Take immediate measurement
                        float ch[18];
                        as7265x_take_measurement_with_bulb();
                        as7265x_read_calibrated_value(AS7265X_CAL_A, &ch[0]);
                        as7265x_read_calibrated_value(AS7265X_CAL_B, &ch[1]);
                        as7265x_read_calibrated_value(AS7265X_CAL_C, &ch[2]);
                        as7265x_read_calibrated_value(AS7265X_CAL_D, &ch[3]);
                        as7265x_read_calibrated_value(AS7265X_CAL_E, &ch[4]);
                        as7265x_read_calibrated_value(AS7265X_CAL_F, &ch[5]);
                        as7265x_read_calibrated_value(AS7265X_CAL_G, &ch[6]);
                        as7265x_read_calibrated_value(AS7265X_CAL_H, &ch[7]);
                        as7265x_read_calibrated_value(AS7265X_CAL_R, &ch[8]);
                        as7265x_read_calibrated_value(AS7265X_CAL_I, &ch[9]);
                        as7265x_read_calibrated_value(AS7265X_CAL_S, &ch[10]);
                        as7265x_read_calibrated_value(AS7265X_CAL_J, &ch[11]);
                        as7265x_read_calibrated_value(AS7265X_CAL_T, &ch[12]);
                        as7265x_read_calibrated_value(AS7265X_CAL_U, &ch[13]);
                        as7265x_read_calibrated_value(AS7265X_CAL_V, &ch[14]);
                        as7265x_read_calibrated_value(AS7265X_CAL_W, &ch[15]);
                        as7265x_read_calibrated_value(AS7265X_CAL_K, &ch[16]);
                        as7265x_read_calibrated_value(AS7265X_CAL_L, &ch[17]);
                        send_spectral_data(ch);
                    } else if (strcmp(action->valuestring, "set_interval") == 0) {
                        cJSON *interval = cJSON_GetObjectItem(root, "interval_ms");
                        if (interval && cJSON_IsNumber(interval)) {
                            sampling_interval_ms = (uint32_t)cJSON_GetNumberValue(interval);
                            ESP_LOGI(TAG, "Sampling interval set to: %lu ms", sampling_interval_ms);
                        }
                    } else if (strcmp(action->valuestring, "bulb_on") == 0) {
                        ESP_LOGI(TAG, "Bulb ON command received");
                        uint8_t control;
                        as7265x_read_virtual_register(AS7265X_LED_CONTROL, &control);
                        control |= AS7265X_LED_DRIVER_ENABLE;
                        as7265x_write_virtual_register(AS7265X_LED_CONTROL, control);
                    } else if (strcmp(action->valuestring, "bulb_off") == 0) {
                        ESP_LOGI(TAG, "Bulb OFF command received");
                        uint8_t control;
                        as7265x_read_virtual_register(AS7265X_LED_CONTROL, &control);
                        control &= ~AS7265X_LED_DRIVER_ENABLE;
                        as7265x_write_virtual_register(AS7265X_LED_CONTROL, control);
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
            xEventGroupSetBits(wifi_event_group, WEBSOCKET_CONNECTED_BIT);
            send_status("ESP32-C3 AS7265X connected");
            break;
            
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WebSocket disconnected");
            xEventGroupClearBits(wifi_event_group, WEBSOCKET_CONNECTED_BIT);
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
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        // Wait for WiFi and WebSocket connection
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WEBSOCKET_CONNECTED_BIT, 
                           false, true, portMAX_DELAY);
        
        if (esp_websocket_client_is_connected(client)) {
            // Take measurement with bulb
            as7265x_take_measurement_with_bulb();
            
            // Read all 18 calibrated channels
            float ch[18];
            as7265x_read_calibrated_value(AS7265X_CAL_A, &ch[0]);
            as7265x_read_calibrated_value(AS7265X_CAL_B, &ch[1]);
            as7265x_read_calibrated_value(AS7265X_CAL_C, &ch[2]);
            as7265x_read_calibrated_value(AS7265X_CAL_D, &ch[3]);
            as7265x_read_calibrated_value(AS7265X_CAL_E, &ch[4]);
            as7265x_read_calibrated_value(AS7265X_CAL_F, &ch[5]);
            as7265x_read_calibrated_value(AS7265X_CAL_G, &ch[6]);
            as7265x_read_calibrated_value(AS7265X_CAL_H, &ch[7]);
            as7265x_read_calibrated_value(AS7265X_CAL_R, &ch[8]);
            as7265x_read_calibrated_value(AS7265X_CAL_I, &ch[9]);
            as7265x_read_calibrated_value(AS7265X_CAL_S, &ch[10]);
            as7265x_read_calibrated_value(AS7265X_CAL_J, &ch[11]);
            as7265x_read_calibrated_value(AS7265X_CAL_T, &ch[12]);
            as7265x_read_calibrated_value(AS7265X_CAL_U, &ch[13]);
            as7265x_read_calibrated_value(AS7265X_CAL_V, &ch[14]);
            as7265x_read_calibrated_value(AS7265X_CAL_W, &ch[15]);
            as7265x_read_calibrated_value(AS7265X_CAL_K, &ch[16]);
            as7265x_read_calibrated_value(AS7265X_CAL_L, &ch[17]);
            
            // Print CSV format to serial (all on one line)
            char csv_line[512];
            int offset = 0;
            for (int i = 0; i < 18; i++) {
                offset += snprintf(csv_line + offset, sizeof(csv_line) - offset, 
                                  "%.4f%s", ch[i], (i < 17) ? ", " : "");
            }
            ESP_LOGI(TAG, "%s", csv_line);
            
            // Send via WebSocket
            send_spectral_data(ch);
        }
        
        // Wait for sampling interval
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(sampling_interval_ms));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32-C3 AS7265X WebSocket Client ===");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize I2C
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };
    
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AS7265X_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &as7265x_handle));
    
    // Initialize AS7265X sensor
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for sensor to stabilize
    if (as7265x_init() != ESP_OK) {
        ESP_LOGE(TAG, "AS7265X initialization failed. Check wiring.");
        return;
    }
    
    ESP_LOGI(TAG, "AS7265X initialized. Channels: A,B,C,D,E,F,G,H,R,I,S,J,T,U,V,W,K,L");
    
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
    xTaskCreate(sensor_task, "sensor_task", 8192, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Application started. Ready to send spectral data via WebSocket!");
}