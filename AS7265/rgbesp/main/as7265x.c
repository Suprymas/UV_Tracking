#include "as7265x.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include <string.h>

#define I2C_MASTER_NUM I2C_NUM_0
#define AS7265X_ADDR 0x49

#define AS7265X_SLAVE_STATUS_REG  0x00
#define AS7265X_SLAVE_WRITE_REG   0x01
#define AS7265X_SLAVE_READ_REG    0x02

#define AS7265X_TX_VALID 0x02
#define AS7265X_RX_VALID 0x01

esp_err_t as7265x_virtual_write(uint8_t reg, uint8_t value) {
    uint8_t status;

    do {
        i2c_master_write_read_device(I2C_MASTER_NUM, AS7265X_ADDR,
            (uint8_t[]){ AS7265X_SLAVE_STATUS_REG }, 1,
            &status, 1, 100);
    } while (status & AS7265X_TX_VALID);

    uint8_t buf[2] = { AS7265X_SLAVE_WRITE_REG, reg | 0x80 };
    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, AS7265X_ADDR, buf, 2, 100));

    do {
        i2c_master_write_read_device(I2C_MASTER_NUM, AS7265X_ADDR,
            (uint8_t[]){ AS7265X_SLAVE_STATUS_REG }, 1,
            &status, 1, 100);
    } while (status & AS7265X_TX_VALID);

    uint8_t data_buf[2] = { AS7265X_SLAVE_WRITE_REG, value };
    return i2c_master_write_to_device(I2C_MASTER_NUM, AS7265X_ADDR, data_buf, 2, 100);
}

esp_err_t as7265x_virtual_read(uint8_t reg, uint8_t *value) {
    uint8_t status;

    do {
        i2c_master_write_read_device(I2C_MASTER_NUM, AS7265X_ADDR,
            (uint8_t[]){ AS7265X_SLAVE_STATUS_REG }, 1,
            &status, 1, 100);
    } while (status & AS7265X_TX_VALID);

    uint8_t buf[2] = { AS7265X_SLAVE_WRITE_REG, reg & 0x7F };
    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, AS7265X_ADDR, buf, 2, 100));

    do {
        i2c_master_write_read_device(I2C_MASTER_NUM, AS7265X_ADDR,
            (uint8_t[]){ AS7265X_SLAVE_STATUS_REG }, 1,
            &status, 1, 100);
    } while (!(status & AS7265X_RX_VALID));

    return i2c_master_write_read_device(I2C_MASTER_NUM, AS7265X_ADDR,
                                        (uint8_t[]){ AS7265X_SLAVE_READ_REG }, 1,
                                        value, 1, 100);
}

esp_err_t as7265x_set_device(uint8_t dev) {
    return as7265x_virtual_write(0x4F, dev); // 0=master, 1=slave1, 2=slave2
}

esp_err_t as7265x_read_calibrated_value(uint8_t reg, float *value) {
    uint32_t tmp = 0;
    uint8_t byte;

    for (int i = 0; i < 4; i++) {
        esp_err_t ret = as7265x_virtual_read(reg + i, &byte);
        if (ret != ESP_OK) return ret;
        tmp = (tmp << 8) | byte;  // build big-endian 32-bit value
    }

    memcpy(value, &tmp, sizeof(float)); // safely convert to float
    return ESP_OK;
}