#pragma once
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_attr.h"

namespace Clock {

enum class ButtonId : uint8_t {
    MODE = 18,
    UP = 19,
    DOWN = 4,
    ENTER = 5
};

class ButtonManager {
private:
    QueueHandle_t& eventQueue_;
    static constexpr int64_t DEBOUNCE_TIME_US = 50000; // 50 ms

    // ให้ประกาศเฉพาะหัวฟังก์ชัน ISR ที่นี่
    static void IRAM_ATTR isrHandler(void* arg);

public:
    explicit ButtonManager(QueueHandle_t& queue);
    bool init();
};

} // namespace Clock