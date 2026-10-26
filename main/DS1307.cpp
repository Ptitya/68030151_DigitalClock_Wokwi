#include "DS1307.hpp"

namespace Clock {

bool DS1307::init() {
    uint8_t secReg = 0;
    if (bus_.readRegs(I2C_ADDR, 0x00, &secReg, 1)) {
        // เคลียร์บิต CH (Clock Halt) บิตที่ 7 เพื่อให้นาฬิกาเริ่มเดิน
        if (secReg & 0x80) {
            bus_.writeReg(I2C_ADDR, 0x00, secReg & 0x7F);
        }
        return true;
    }
    return false;
}

bool DS1307::getTime(DateTime& dt) {
    uint8_t raw[7];
    if (!bus_.readRegs(I2C_ADDR, 0x00, raw, 7)) return false;
    dt.second = bcd2dec(raw[0] & 0x7F);
    dt.minute = bcd2dec(raw[1]);
    dt.hour = bcd2dec(raw[2] & 0x3F);
    dt.dayOfWeek = bcd2dec(raw[3]);
    dt.dayOfMonth = bcd2dec(raw[4]);
    dt.month = bcd2dec(raw[5]);
    dt.year = 2000 + bcd2dec(raw[6]);
    return true;
}

bool DS1307::setTime(const DateTime& dt) {
    uint8_t raw[7];
    raw[0] = dec2bcd(dt.second) & 0x7F; // CH = 0
    raw[1] = dec2bcd(dt.minute);
    raw[2] = dec2bcd(dt.hour);
    raw[3] = dec2bcd(dt.dayOfWeek);
    raw[4] = dec2bcd(dt.dayOfMonth);
    raw[5] = dec2bcd(dt.month);
    raw[6] = dec2bcd(static_cast<uint8_t>(dt.year >= 2000 ? dt.year - 2000 : dt.year));
    return bus_.writeBytes(I2C_ADDR, 0x00, raw, 7);
}

} // namespace Clock