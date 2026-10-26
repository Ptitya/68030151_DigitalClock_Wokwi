#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "I2CBus.hpp"
#include "DS1307.hpp"
#include "SSD1306.hpp"

static const char* TAG = "DIGITAL_CLOCK";

static Clock::I2CBus i2cBus(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22);
static Clock::DS1307 rtc(i2cBus);
static Clock::SSD1306 display(i2cBus, 0x3C);

// ฟังก์ชันแปลงเวลาจากเครื่องคอมพิวเตอร์ตอนกดคอมไพล์
void setTimeFromBuild(Clock::DS1307& rtcRef) {
    const char* t = __TIME__; // ดึงเวลาคอมพิวเตอร์ "HH:MM:SS"
    int h = atoi(t);
    int m = atoi(t + 3);
    int s = atoi(t + 6);

    Clock::DateTime dt = {
        static_cast<uint8_t>(s),
        static_cast<uint8_t>(m),
        static_cast<uint8_t>(h),
        1, 26, 10, 2026
    };
    rtcRef.setTime(dt);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting Digital Clock...");

    if (!i2cBus.init()) {
        ESP_LOGE(TAG, "I2C Bus Init FAILED!");
        return;
    }

    rtc.init();
    display.init();

    // ดึงเวลาปัจจุบันจากคอมพิวเตอร์บันทึกลง RTC ทันทีตอนเริ่มเปิดเครื่อง
    setTimeFromBuild(rtc);

    char timeStr[20];
    Clock::DateTime dt;

    // ระบบจับจังหวะรอบของ FreeRTOS ให้คงที่ 1000 ms เป๊ะ
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);

    while (true) {
        if (rtc.getTime(dt)) {
            display.clear();

            // แสดงข้อความหัวเรื่อง
            display.drawString(28, 8, "ESP32 CLOCK");

            // แสดงเวลาแบบ HH:MM:SS
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", dt.hour, dt.minute, dt.second);
            display.drawString(40, 32, timeStr);

            display.flush();
        }

        // ชดเชยเวลาที่สูญเสียไปกับการเรนเดอร์หน้าจอ เพื่อให้รอบเวลาคงที่ 1 วินาทีเสมอ
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}