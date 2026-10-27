#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "I2CBus.hpp"
#include "DS1307.hpp"
#include "SSD1306.hpp"
#include "ButtonManager.hpp"
#include "WifiSync.hpp"
#include "Buzzer.hpp"

static const char* TAG = "LAB5_MAIN";

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
static Clock::Buzzer buzzer(GPIO_NUM_27);

static QueueHandle_t buttonQueue = nullptr;
static ClockState currentState = ClockState::CLOCK_MAIN;

// --- ตัวแปร Alarm และ NVS ---
static uint8_t alarmHour = 7;
static uint8_t alarmMinute = 0;
static bool alarmEnabled = false;
static uint8_t alarmEditField = 0; // 0: Hour, 1: Minute
static bool alarmTriggered = false;

// --- ตัวแปร Stopwatch ---
static bool swRunning = false;
static int64_t swStartTimeUs = 0;
static int64_t swElapsedUs = 0;

// --- ตัวแปร Time Set ---
static Clock::DateTime adjustDt;
static uint8_t timeEditField = 0;

// ฟังก์ชันโหลดค่าเวลาปลุกจาก NVS
static void loadAlarmFromNVS() {
    nvs_handle_t my_handle;
    if (nvs_open("clock_storage", NVS_READONLY, &my_handle) == ESP_OK) {
        nvs_get_u8(my_handle, "alm_hour", &alarmHour);
        nvs_get_u8(my_handle, "alm_min", &alarmMinute);
        uint8_t en = 0;
        nvs_get_u8(my_handle, "alm_en", &en);
        alarmEnabled = (en == 1);
        nvs_close(my_handle);
        ESP_LOGI(TAG, "Loaded Alarm from NVS: %02d:%02d, Enabled: %d", alarmHour, alarmMinute, alarmEnabled);
    }
}

// ฟังก์ชันบันทึกค่าเวลาปลุกลง NVS
static void saveAlarmToNVS() {
    nvs_handle_t my_handle;
    if (nvs_open("clock_storage", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_u8(my_handle, "alm_hour", alarmHour);
        nvs_set_u8(my_handle, "alm_min", alarmMinute);
        nvs_set_u8(my_handle, "alm_en", alarmEnabled ? 1 : 0);
        nvs_commit(my_handle);
        nvs_close(my_handle);
        ESP_LOGI(TAG, "Saved Alarm to NVS: %02d:%02d, Enabled: %d", alarmHour, alarmMinute, alarmEnabled);
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting Lab 5: Final Clock Integration...");

    i2cBus.init();
    rtc.init();
    display.init();
    buzzer.init();

    // โหลดค่า Alarm ที่เคยเซฟไว้
    loadAlarmFromNVS();

    display.clear();
    display.drawString(16, 16, "CONNECTING WIFI");
    display.drawString(24, 36, "AND SYNC NTP");
    display.flush();

    wifiSync.initAndSync("Wokwi-GUEST", "");

    buttonQueue = xQueueCreate(10, sizeof(Clock::ButtonId));
    Clock::ButtonManager btnMgr(buttonQueue);
    btnMgr.init();

    char lineBuf[32];
    Clock::DateTime currentDt;
    Clock::ButtonId receivedBtn;
    uint32_t cycleTicks = 0;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(50);

    while (true) {
        cycleTicks++;

        // จัดการ Event ปุ่มกด
        while (xQueueReceive(buttonQueue, &receivedBtn, 0) == pdTRUE) {
            // ถ้าเสียงปลุกดังอยู่ กดปุ่มไหนก็ได้เพื่อหยุดเสียง (Dismiss Alarm)
            if (buzzer.isActive()) {
                buzzer.stop();
                alarmTriggered = false;
                ESP_LOGI(TAG, "Alarm Dismissed!");
                continue;
            }

            if (receivedBtn == Clock::ButtonId::MODE) {
                switch (currentState) {
                    case ClockState::CLOCK_MAIN:
                        currentState = ClockState::ALARM_SET;
                        alarmEditField = 0;
                        break;
                    case ClockState::ALARM_SET:
                        currentState = ClockState::STOPWATCH;
                        break;
                    case ClockState::STOPWATCH:
                        currentState = ClockState::TIME_SET;
                        rtc.getTime(adjustDt);
                        timeEditField = 0;
                        break;
                    case ClockState::TIME_SET:
                        currentState = ClockState::CLOCK_MAIN;
                        break;
                }
                ESP_LOGI(TAG, "Mode Changed to: %d", static_cast<int>(currentState));
            } else {
                switch (currentState) {
                    case ClockState::CLOCK_MAIN:
                        if (receivedBtn == Clock::ButtonId::ENTER) {
                            alarmEnabled = !alarmEnabled;
                            saveAlarmToNVS();
                        }
                        break;

                    case ClockState::ALARM_SET:
                        if (receivedBtn == Clock::ButtonId::ENTER) {
                            alarmEditField = (alarmEditField + 1) % 2;
                            saveAlarmToNVS(); // เซฟลง NVS ทันทีเมื่อกดยืนยันฟิลด์
                        } else if (receivedBtn == Clock::ButtonId::UP) {
                            if (alarmEditField == 0) alarmHour = (alarmHour + 1) % 24;
                            else alarmMinute = (alarmMinute + 1) % 60;
                        } else if (receivedBtn == Clock::ButtonId::DOWN) {
                            if (alarmEditField == 0) alarmHour = (alarmHour == 0) ? 23 : alarmHour - 1;
                            else alarmMinute = (alarmMinute == 0) ? 59 : alarmMinute - 1;
                        }
                        break;

                    case ClockState::STOPWATCH:
                        if (receivedBtn == Clock::ButtonId::UP) {
                            if (!swRunning) {
                                swStartTimeUs = esp_timer_get_time() - swElapsedUs;
                                swRunning = true;
                            } else {
                                swElapsedUs = esp_timer_get_time() - swStartTimeUs;
                                swRunning = false;
                            }
                        } else if (receivedBtn == Clock::ButtonId::DOWN) {
                            swRunning = false;
                            swElapsedUs = 0;
                        }
                        break;

                    case ClockState::TIME_SET:
                        if (receivedBtn == Clock::ButtonId::ENTER) {
                            timeEditField++;
                            if (timeEditField > 2) {
                                rtc.setTime(adjustDt);
                                ESP_LOGI(TAG, "Manual RTC Time Saved!");
                                currentState = ClockState::CLOCK_MAIN;
                            }
                        } else if (receivedBtn == Clock::ButtonId::UP) {
                            if (timeEditField == 0) adjustDt.hour = (adjustDt.hour + 1) % 24;
                            else if (timeEditField == 1) adjustDt.minute = (adjustDt.minute + 1) % 60;
                            else if (timeEditField == 2) adjustDt.second = (adjustDt.second + 1) % 60;
                        } else if (receivedBtn == Clock::ButtonId::DOWN) {
                            if (timeEditField == 0) adjustDt.hour = (adjustDt.hour == 0) ? 23 : adjustDt.hour - 1;
                            else if (timeEditField == 1) adjustDt.minute = (adjustDt.minute == 0) ? 59 : adjustDt.minute - 1;
                            else if (timeEditField == 2) adjustDt.second = (adjustDt.second == 0) ? 59 : adjustDt.second - 1;
                        }
                        break;
                }
            }
        }

        display.clear();
        rtc.getTime(currentDt);

        // --- ระบบตรวจสอบเงื่อนไขเวลาปลุก (Alarm Trigger) ---
        if (alarmEnabled && currentDt.hour == alarmHour && currentDt.minute == alarmMinute && currentDt.second == 0) {
            if (!alarmTriggered) {
                alarmTriggered = true;
                buzzer.start();
                ESP_LOGI(TAG, "ALARM TRIGGERED! BEEPING...");
            }
        }

        // อัปเดตเสียง Buzzer
        buzzer.tick(cycleTicks);

        // --- Render หน้าจอ ---
        switch (currentState) {
            case ClockState::CLOCK_MAIN:
                if (buzzer.isActive()) {
                    // กะพริบหน้าจอเตือนเวลาปลุก
                    display.drawString(20, 16, "** ALARM RINGING! **");
                    display.drawString(14, 38, "PRESS ANY KEY TO STOP");
                } else {
                    display.drawString(20, 2, "[MAIN CLOCK]");
                    snprintf(lineBuf, sizeof(lineBuf), "%02d:%02d:%02d", currentDt.hour, currentDt.minute, currentDt.second);
                    display.drawString(32, 22, lineBuf);
                    snprintf(lineBuf, sizeof(lineBuf), "%02d/%02d/%04d", currentDt.day, currentDt.month, currentDt.year);
                    display.drawString(26, 40, lineBuf);

                    if (alarmEnabled) {
                        snprintf(lineBuf, sizeof(lineBuf), "ALARM: %02d:%02d [ON]", alarmHour, alarmMinute);
                    } else {
                        snprintf(lineBuf, sizeof(lineBuf), "ALARM: [OFF]");
                    }
                    display.drawString(14, 54, lineBuf);
                }
                break;

            case ClockState::ALARM_SET:
                display.drawString(28, 4, "[SET ALARM]");
                snprintf(lineBuf, sizeof(lineBuf), "  %02d : %02d", alarmHour, alarmMinute);
                display.drawString(28, 24, lineBuf);

                if (alarmEditField == 0) display.drawString(28, 38, "  ^^");
                else display.drawString(28, 38, "       ^^");

                display.drawString(10, 52, "UP/DN:ADJ  ENT:SAVE");
                break;

            case ClockState::STOPWATCH: {
                int64_t totalUs = swElapsedUs;
                if (swRunning) {
                    totalUs = esp_timer_get_time() - swStartTimeUs;
                }
                int mins = (totalUs / 60000000LL) % 60;
                int secs = (totalUs / 1000000LL) % 60;
                int millis = (totalUs / 1000LL) % 1000;

                display.drawString(28, 4, "[STOPWATCH]");
                snprintf(lineBuf, sizeof(lineBuf), "%02d:%02d.%03d", mins, secs, millis);
                display.drawString(24, 26, lineBuf);

                if (swRunning) display.drawString(20, 50, "UP:STOP  DN:RESET");
                else display.drawString(20, 50, "UP:START DN:RESET");
                break;
            }

            case ClockState::TIME_SET:
                display.drawString(28, 4, "[SET TIME]");
                snprintf(lineBuf, sizeof(lineBuf), "%02d : %02d : %02d", adjustDt.hour, adjustDt.minute, adjustDt.second);
                display.drawString(20, 24, lineBuf);

                if (timeEditField == 0) display.drawString(20, 38, "^^");
                else if (timeEditField == 1) display.drawString(20, 38, "     ^^");
                else if (timeEditField == 2) display.drawString(20, 38, "          ^^");

                display.drawString(6, 52, "UP/DN:ADJ  ENT:SAVE");
                break;
        }

        display.flush();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}