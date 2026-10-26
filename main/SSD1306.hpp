#pragma once
#include "I2CBus.hpp"
#include <cstdint>

namespace Clock {

class SSD1306 {
private:
    I2CBus& bus_;
    uint8_t addr_;
    uint8_t buffer_[1024];

    void sendCommand(uint8_t cmd);

public:
    explicit SSD1306(I2CBus& bus, uint8_t addr = 0x3C) : bus_(bus), addr_(addr) {}
    bool init();
    void clear();
    void setPixel(int x, int y, bool color);
    void drawChar(int x, int y, char c);
    void drawString(int x, int y, const char* str);
    void flush();
};

} // namespace Clock