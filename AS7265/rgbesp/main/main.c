#include <stdio.h>
#include "i2c_driver.h"
#include "as7265x.h"
#include "esp_log.h"
#include "wifi.h"
#include "websocket.h"
#include "nvs_flash.h"


void app_main(void) {
    ESP_LOGI("MAIN", "Starting...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_init_sta();




    i2c_master_init();
    ESP_LOGI("AS7265X", "I2C initialized");

    while (1) {
        float ch[18]; // 6 channels per device × 3 devices
        int idx = 0;

        for (uint8_t dev = 0; dev < 3; dev++) {
            // Select device
            as7265x_set_device(dev);

            // Each device has 6 channels starting at 0x14
            for (uint8_t base = 0x14; base < 0x2C; base += 4) {
                if (as7265x_read_calibrated_value(base, &ch[idx]) != ESP_OK) {
                    ESP_LOGE("AS7265X", "Read failed dev=%d chan=%d", dev, idx % 6);
                    ch[idx] = 0.0f; // fallback
                }
                idx++;
            }
        }

        // Print CSV
        for (int i = 0; i < 18; i++)
            printf("%.4f%s", ch[i], (i < 17 ? ", " : "\n"));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
