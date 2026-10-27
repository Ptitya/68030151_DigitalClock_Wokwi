#include "DS1307.hpp"

namespace Clock {

bool DS1307::init() {
    uint8_t reg = 0x00;
    uint8_t data = 0;
    if (!bus_.readBytes(ADDR, reg, &data, 1)) return false;
    if (data & 0x80) {
        data &= 0x7F;
        return bus_.writeBytes(ADDR, reg, &data, 1);
    }
    return true;
}

bool DS1307::getTime(DateTime& dt) {
    uint8_t raw[7];
    if (!bus_.readBytes(ADDR, 0x00, raw, 7)) return false;

    dt.second    = bcd2dec(raw[0] & 0x7F);
    dt.minute    = bcd2dec(raw[1]);
    dt.hour      = bcd2dec(raw[2] & 0x3F);
    dt.dayOfWeek = raw[3];
    dt.day       = bcd2dec(raw[4]);
    dt.month     = bcd2dec(raw[5]);
    dt.year      = bcd2dec(raw[6]) + 2000;
    return true;
}

bool DS1307::setTime(const DateTime& dt) {
    uint8_t raw[7];
    raw[0] = dec2bcd(dt.second);
    raw[1] = dec2bcd(dt.minute);
    raw[2] = dec2bcd(dt.hour);
    raw[3] = dt.dayOfWeek;
    raw[4] = dec2bcd(dt.day);
    raw[5] = dec2bcd(dt.month);
    raw[6] = dec2bcd(static_cast<uint8_t>(dt.year % 100));

    return bus_.writeBytes(ADDR, 0x00, raw, 7);
}

} // namespace Clock