#pragma once
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

namespace Clock {

class I2CBus {
private:
    i2c_port_t port_;
    gpio_num_t sdaPin_;
    gpio_num_t sclPin_;
    SemaphoreHandle_t mutex_{nullptr};

public:
    I2CBus(i2c_port_t port, gpio_num_t sda, gpio_num_t scl)
        : port_(port), sdaPin_(sda), sclPin_(scl) {
        mutex_ = xSemaphoreCreateMutex();
    }

    bool init() {
        i2c_config_t conf = {};
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = sdaPin_;
        conf.scl_io_num = sclPin_;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = 400000;
        
        esp_err_t err = i2c_param_config(port_, &conf);
        if (err != ESP_OK) return false;
        
        return i2c_driver_install(port_, conf.mode, 0, 0, 0) == ESP_OK;
    }

    bool writeReg(uint8_t devAddr, uint8_t reg, uint8_t data) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_write_byte(cmd, data, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(port_, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);
        xSemaphoreGive(mutex_);
        return ret == ESP_OK;
    }

    bool writeBytes(uint8_t devAddr, uint8_t reg, const uint8_t* data, size_t len) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_write(cmd, const_cast<uint8_t*>(data), len, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(port_, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);
        xSemaphoreGive(mutex_);
        return ret == ESP_OK;
    }

    bool readRegs(uint8_t devAddr, uint8_t startReg, uint8_t* buffer, size_t len) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, startReg, true);
        
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_READ, true);
        if (len > 1) {
            i2c_master_read(cmd, buffer, len - 1, I2C_MASTER_ACK);
        }
        i2c_master_read_byte(cmd, buffer + len - 1, I2C_MASTER_NACK);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(port_, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);
        xSemaphoreGive(mutex_);
        return ret == ESP_OK;
    }
};

} // namespace Clock