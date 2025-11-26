#pragma once

#include "esp_err.h"
#include <stdint.h>


esp_err_t as7265x_virtual_write(uint8_t reg, uint8_t value);
esp_err_t as7265x_virtual_read(uint8_t reg, uint8_t *value);
esp_err_t as7265x_set_device(uint8_t dev);
esp_err_t as7265x_read_calibrated_value(uint8_t reg, float *value);
