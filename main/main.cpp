#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "I2CBus.hpp"
#include "DS1307.hpp"
#include "SSD1306.hpp"
#include "ButtonManager.hpp"
#include "WifiSync.hpp"

static const char* TAG = "LAB3_MAIN";

enum class ClockState {
    CLOCK_MAIN,
    ALARM_SET,
    STOPWATCH,
    TIME_SET
};

static Clock::I2CBus i2cBus(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22);
static Clock::DS1307 rtc(i2cBus);
static Clock::SSD1306 display(i2cBus, 0x3C);
static Clock::WifiSync wifiSync(rtc);

static QueueHandle_t buttonQueue = nullptr;
static ClockState currentState = ClockState::CLOCK_MAIN;

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting Lab 3: Wi-Fi & SNTP Sync...");

    i2cBus.init();
    rtc.init();
    display.init();

    // 1. แสดงสถานะกำลังเชื่อมต่อ Wi-Fi และซิงก์เวลา
    display.clear();
    display.drawString(16, 16, "CONNECTING WIFI");
    display.drawString(24, 36, "AND SYNC NTP");
    display.flush();

    // 2. ดึงเวลามาตรฐานสากลผ่าน Wi-Fi จำลองของ Wokwi (Wokwi-GUEST)
    wifiSync.initAndSync("Wokwi-GUEST", "");

    // 3. เริ่มต้นระบบจัดการปุ่มกดและ Interrupt Queue
    buttonQueue = xQueueCreate(10, sizeof(Clock::ButtonId));
    Clock::ButtonManager btnMgr(buttonQueue);
    btnMgr.init();

    char lineBuf[24];
    Clock::DateTime dt;
    Clock::ButtonId receivedBtn;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);

    while (true) {
        // ตรวจสอบคิวปุ่มกดเพื่อสลับสถานะ FSM
        if (xQueueReceive(buttonQueue, &receivedBtn, 0) == pdTRUE) {
            if (receivedBtn == Clock::ButtonId::MODE) {
                switch (currentState) {
                    case ClockState::CLOCK_MAIN: currentState = ClockState::ALARM_SET; break;
                    case ClockState::ALARM_SET:  currentState = ClockState::STOPWATCH; break;
                    case ClockState::STOPWATCH:  currentState = ClockState::TIME_SET; break;
                    case ClockState::TIME_SET:   currentState = ClockState::CLOCK_MAIN; break;
                }
                ESP_LOGI(TAG, "Mode Changed to: %d", static_cast<int>(currentState));
            }
        }

        display.clear();
        rtc.getTime(dt);

        switch (currentState) {
            case ClockState::CLOCK_MAIN:
                display.drawString(20, 4, "[MAIN CLOCK: NTP]");
                snprintf(lineBuf, sizeof(lineBuf), "%02d:%02d:%02d", dt.hour, dt.minute, dt.second);
                display.drawString(32, 26, lineBuf);
                snprintf(lineBuf, sizeof(lineBuf), "%02d/%02d/%04d", dt.day, dt.month, dt.year);
                display.drawString(26, 46, lineBuf);
                break;

            case ClockState::ALARM_SET:
                display.drawString(28, 4, "[SET ALARM]");
                display.drawString(32, 28, "07:00:00");
                display.drawString(16, 50, "UP/DOWN/ENTER");
                break;

            case ClockState::STOPWATCH:
                display.drawString(28, 4, "[STOPWATCH]");
                display.drawString(24, 28, "00:00.000");
                display.drawString(20, 50, "UP:START/STOP");
                break;

            case ClockState::TIME_SET:
                display.drawString(28, 4, "[SET TIME]");
                snprintf(lineBuf, sizeof(lineBuf), "%02d:%02d:%02d", dt.hour, dt.minute, dt.second);
                display.drawString(32, 28, lineBuf);
                display.drawString(16, 50, "ADJUST CLOCK");
                break;
        }

        display.flush();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}