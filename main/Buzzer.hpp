#pragma once
#include "driver/ledc.h"

namespace Clock {

class Buzzer {
private:
    gpio_num_t pin_;
    bool active_ = false;
    static constexpr ledc_channel_t LEDC_CH = LEDC_CHANNEL_0;
    static constexpr ledc_timer_t   LEDC_TMR = LEDC_TIMER_0;

public:
    explicit Buzzer(gpio_num_t pin) : pin_(pin) {}

    void init() {
        // ตั้งค่าตัวจับเวลา LEDC Timer ที่ความถี่ 2000 Hz
        ledc_timer_config_t timer_conf = {};
        timer_conf.speed_mode       = LEDC_LOW_SPEED_MODE;
        timer_conf.duty_resolution  = LEDC_TIMER_10_BIT;
        timer_conf.timer_num        = LEDC_TMR;
        timer_conf.freq_hz          = 2000; // ความถี่ 2 kHz สำหรับเสียงเตือน
        timer_conf.clk_cfg          = LEDC_AUTO_CLK;
        ledc_timer_config(&timer_conf);

        // กำหนด Channel และผูกกับขา GPIO
        ledc_channel_config_t ch_conf = {};
        ch_conf.gpio_num       = pin_;
        ch_conf.speed_mode     = LEDC_LOW_SPEED_MODE;
        ch_conf.channel        = LEDC_CH;
        ch_conf.intr_type      = LEDC_INTR_DISABLE;
        ch_conf.timer_sel      = LEDC_TMR;
        ch_conf.duty           = 0;
        ch_conf.hpoint         = 0;
        ledc_channel_config(&ch_conf);

        stop();
    }

    void start() {
        active_ = true;
    }

    void stop() {
        active_ = false;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CH);
    }

    bool isActive() const {
        return active_;
    }

    // สร้างจังหวะเสียงบี๊บ บี๊บ สลับเปิด/ปิด
    void tick(uint32_t cycleCount) {
        if (!active_) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CH);
            return;
        }

        // จ่ายคลื่นเสียง 50% Duty Cycle สลับกับปิดเสียง
        if (cycleCount % 4 < 2) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH, 512); // Duty 50% (512 จาก 1023)
        } else {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH, 0);
        }
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CH);
    }
};

} // namespace Clock