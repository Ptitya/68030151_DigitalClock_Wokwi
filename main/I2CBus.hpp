#pragma once

#include "driver/i2c.h"
#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

namespace Clock {

class I2CBus {
private:
    i2c_port_t port_;
    gpio_num_t sdaPin_;
    gpio_num_t sclPin_;

public:
    I2CBus(i2c_port_t port, gpio_num_t sda, gpio_num_t scl)
        : port_(port), sdaPin_(sda), sclPin_(scl) {}

    bool init(uint32_t clkSpeed = 400000) {
        i2c_config_t conf = {};
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = sdaPin_;
        conf.scl_io_num = sclPin_;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = clkSpeed;
        
        esp_err_t err = i2c_param_config(port_, &conf);
        if (err != ESP_OK) return false;
        
        return i2c_driver_install(port_, conf.mode, 0, 0, 0) == ESP_OK;
    }

    // เขียน 1 ไบต์ลง Register (ที่ SSD1306 เรียกหา)
    bool writeReg(uint8_t devAddr, uint8_t reg, uint8_t val) {
        return writeBytes(devAddr, reg, &val, 1);
    }

    // เขียน Array ของข้อมูลลง Register (ที่ SSD1306 และ DS1307 เรียกหา)
    bool writeBytes(uint8_t devAddr, uint8_t reg, const uint8_t* data, size_t len) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_write(cmd, const_cast<uint8_t*>(data), len, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(port_, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);
        return (ret == ESP_OK);
    }

    // อ่านข้อมูลหลายไบต์จาก Register (ที่ DS1307 เรียกหา)
    bool readBytes(uint8_t devAddr, uint8_t reg, uint8_t* data, size_t len) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_READ, true);
        if (len > 1) {
            i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
        }
        i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(port_, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);
        return (ret == ESP_OK);
    }

    // ฟังก์ชันเสริมแบบใช้ชื่อ write / read
    bool write(uint8_t devAddr, uint8_t reg, const uint8_t* data, size_t len) {
        return writeBytes(devAddr, reg, data, len);
    }

    bool read(uint8_t devAddr, uint8_t reg, uint8_t* data, size_t len) {
        return readBytes(devAddr, reg, data, len);
    }
};

} // namespace Clock