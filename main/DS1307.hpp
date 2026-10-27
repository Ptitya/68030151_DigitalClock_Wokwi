#pragma once
#include <stdint.h>
#include "I2CBus.hpp"

namespace Clock {

struct DateTime {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t dayOfWeek;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

class DS1307 {
private:
    I2CBus& bus_;
    static constexpr uint8_t ADDR = 0x68;

    static uint8_t bcd2dec(uint8_t val) {
        return ((val >> 4) * 10) + (val & 0x0F);
    }

    static uint8_t dec2bcd(uint8_t val) {
        return ((val / 10) << 4) | (val % 10);
    }

public:
    explicit DS1307(I2CBus& bus) : bus_(bus) {}

    bool init();
    bool getTime(DateTime& dt);
    bool setTime(const DateTime& dt);
};

} // namespace Clock