# รายงานผลการดำเนินงานโครงงานระบบนาฬิกาดิจิทัลอัจฉริยะบน ESP32
**Modular Object-Oriented Digital Clock Simulation Project on ESP-IDF & Wokwi**

---

### ข้อมูลผู้จัดทำ
- **ชื่อ - นามสกุล:** นางสาวปทิตญา ภูกิจคุณาเดชากร
- **รหัสนักศึกษา:** 68030151
- **แพลตฟอร์มการพัฒนา:** ESP-IDF v5.2.7, C++23/GNU++2b, FreeRTOS, Wokwi Simulator

---

## 1. บทนำและภาพรวมโครงงาน (Project Overview)

โครงงานนี้มีเป้าหมายเพื่อพัฒนา **ระบบนาฬิกาดิจิทัลอัจฉริยะ (Modular Digital Clock System)** ที่ทำงานบนไมโครคอนโทรลเลอร์ **ESP32** ภายใต้เฟรมเวิร์ก **ESP-IDF** และจำลองการทำงานบน **Wokwi Simulator** โดยเน้นการออกแบบสถาปัตยกรรมซอฟต์แวร์ตามหลักการเชิงวัตถุ (**Object-Oriented Programming: OOP**) ควบคู่กับการใช้ฟีเจอร์ระดับต่ำของฮาร์ดแวร์และการจัดการงานแบบหลายภารกิจ (**FreeRTOS Multi-Tasking & Queues**)

### อุปกรณ์และส่วนประกอบหลัก (Hardware Components)
1. **บอร์ดประมวลผล:** ESP32 DevKit v4
2. **ชิปนาฬิกาเวลาจริง:** DS1307 Real-Time Clock (I2C Address: `0x68`)
3. **จอแสดงผล:** SSD1306 0.96" OLED Display 128x64 pixels (I2C Address: `0x3C`)
4. **ปุ่มกดควบคุม (Pushbuttons):** 4 ปุ่มเชื่อมต่อแบบ Active-Low ร่วมกับวงจร Internal Pull-up และ Hardware Interrupt
   - **GPIO 18:** ปุ่ม `MODE`
   - **GPIO 19:** ปุ่ม `UP`
   - **GPIO 4:** ปุ่ม `DOWN`
   - **GPIO 5:** ปุ่ม `ENTER`
5. **อุปกรณ์กำเนิดเสียงแจ้งเตือน:** Passive Piezo Buzzer เชื่อมต่อกับ **GPIO 27** ควบคุมผ่าน ESP32 LEDC (Hardware PWM)

---

## 2. รายละเอียดการดำเนินงานจำแนกตามแต่ละปฏิบัติการ (Lab Modules)

---

### 2.1 ปฏิบัติการที่ 1 (Lab 1): I2C Hardware Abstraction Layer & Peripherals Interfacing

#### 1) วัตถุประสงค์
- เพื่อออกแบบและสร้างคลาสควมคุมบัส **I2C (`Clock::I2CBus`)** ที่ทำงานแบบ Object-Oriented เป็นชั้นฐานข้อมูล (HAL) ให้กับอุปกรณ์อื่น
- เพื่อพัฒนา Driver สำหรับติดต่อกับชิป **DS1307 RTC** และจอแสดงผล **SSD1306 OLED** บน I2C บัสเดียวกัน (SDA: GPIO 21, SCL: GPIO 22)
- เพื่ออ่านเวลาจาก RTC และแสดงผลขึ้นบนหน้าจอ OLED ได้อย่างถูกต้อง

#### 2) ขั้นตอนการดำเนินงาน
1. ออกแบบคลาส `I2CBus` ห่อหุ้มฟังก์ชัน `i2c_param_config` และ `i2c_driver_install`
2. สร้างคลาส `DS1307` ใช้อ่าน-เขียน Register วันที่และเวลาผ่านการแปลงเลขฐานรหัส BCD (Binary-Coded Decimal)
3. พัฒนาคลาส `SSD1306` บริหารจัดการ Framebuffer ขนาด $128 \times 64$ บิต พร้อมชุดตาราง Font Matrix สำหรับวาดตัวอักษรลงบัฟเฟอร์แล้วส่งเข้า I2C ผ่านฟังก์ชัน `flush()`
4. เริ่มต้นระบบใน `main.cpp` เพื่อแสดงข้อความและเวลาที่นับเดินหน้าทีละวินาที

#### 3) ปัญหาที่พบและแนวทางแก้ไข
- **ปัญหา:** ชิป RTC DS1307 ในการบูตครั้งแรกไม่ยอมเริ่มนับเวลา ค้างอยู่ที่วินาทีเริ่มต้น
- **สาเหตุ:** บิตที่ 7 ของ Register 0x00 บน DS1307 คือบิต Clock Halt (CH) ซึ่งมีค่าเริ่มต้นเป็น `1` ทำให้ Oscillator หยุดทำงาน
- **การแก้ไข:** ในฟังก์ชัน `DS1307::init()` ได้ทำการอ่านค่าไบต์แรก หากพบว่ามีบิต `0x80` ให้ทำ Bitwise AND ด้วย `0x7F` เพื่อเคลียร์บิต CH ให้เป็น `0` แล้วเขียนกลับลงไป ทำให้ระบบสร้างสัญญาณนาฬิกาเริ่มทำงานได้ทันที

#### 4) ภาพผลลัพธ์การทดลอง
![alt text](68030151_DigitalClock_Wokwi/pictures/lab1.png)

---

### 2.2 ปฏิบัติการที่ 2 (Lab 2): Hardware Interrupts, Debouncing & FreeRTOS Queue

#### 1) วัตถุประสงค์
- เพื่อพัฒนาระบบรับสัญญาณอินพุตจากปุ่มกด 4 ปุ่มด้วยระบบ **Hardware GPIO Interrupt** แทนการเขียนคำสั่ง Polling เพื่อลดการสิ้นเปลืองรอบประมวลผลของ CPU
- เพื่อแก้ปัญหาการเด้งของหน้าสัมผัสกลไกปุ่มกด (**Contact Bounce / Debounce**)
- เพื่อส่งต่อ Event การกดปุ่มจากระดับ ISR ไปประมวลผลในฝั่ง Application Task ผ่าน **FreeRTOS Queue**

#### 2) ขั้นตอนการดำเนินงาน
1. ออกแบบคลาส `ButtonManager` กำหนดขา GPIO 18, 19, 4, 5 ให้มีสถานะเป็น Input พร้อมเปิด Pull-up
2. ติดตั้ง ISR Service ด้วย `gpio_install_isr_service(0)` และลงทะเบียน Interrupt Handler ตรวจจับขอบขาลง (Falling-edge trigger)
3. ใช้ระบบหน่วงเวลาจับคู่กับ `esp_timer_get_time()` ตรวจสอบค่าเวลา Diff เกิน 150 ms เพื่อกรองสัญญาณรบกวน (Debounce)
4. เมื่อตรวจพบการกดที่ถูกต้อง ISR จะส่งค่า Enum `ButtonId` ผ่านฟังก์ชัน `xQueueSendFromISR` เข้าสู่ `buttonQueue`

#### 3) ปัญหาที่พบและแนวทางแก้ไข
- **ปัญหา:** การสลับหน้าจอหรือรับค่าปุ่มมีอาการเบิ้ล หรือบางครั้งกดแล้ว FreeRTOS เกิด Guru Meditation Error
- **สาเหตุ:** การเรียกใช้ฟังก์ชันธรรมดาที่ไม่ใช่ ISR-safe ภายในฟังก์ชัน Interrupt Callback หรือไม่มีการกรองเวลา Debounce ที่รัดกุม
- **การแก้ไข:** เปลี่ยนมาใช้ `xQueueSendFromISR` ร่วมกับตัวแปรแฟล็ก `pxHigherPriorityTaskWoken` และเพิ่มเงื่อนไขตรวจสอบระยะห่างระหว่างการกดให้อย่างน้อย 150–200 ms

#### 4) ภาพผลลัพธ์การทดลอง
![alt text](68030151_DigitalClock_Wokwi/pictures/lab2_1.png)

![alt text](68030151_DigitalClock_Wokwi/pictures/lab2_2.png)

![alt text](68030151_DigitalClock_Wokwi/pictures/lab2_3.png)

---

### 2.3 ปฏิบัติการที่ 3 (Lab 3): Wi-Fi Station & SNTP Internet Time Synchronization

#### 1) วัตถุประสงค์
- เพื่อเชื่อมต่อ ESP32 เข้ากับเครือข่ายอินเทอร์เน็ตไร้สาย Wi-Fi (โหมด Station: STA)
- เพื่อดึงเวลาสากลที่แม่นยำสูงผ่านโปรโตคอล **SNTP (Simple Network Time Protocol)** จากเซิร์ฟเวอร์ `pool.ntp.org`
- เพื่อซิงโครไนซ์เวลาจากระบบ POSIX System Clock ไปอัปเดตลงในหน่วยความจำของ RTC DS1307 อัตโนมัติ

#### 2) ขั้นตอนการดำเนินงาน
1. สร้างคลาส `WifiSync` รองรับการต่อเน็ตเวิร์กจำลอง `Wokwi-GUEST`
2. จัดการ Event Loop ผ่าน `esp_event_handler_instance_register` รอ Event `IP_EVENT_STA_GOT_IP` โดยใช้ `FreeRTOS EventGroup`
3. ตั้งค่าสภาพแวดล้อมเขตเวลาของประเทศไทย (`ICT-7`) ผ่าน `setenv("TZ", "ICT-7", 1); tzset();`
4. เมื่อได้รับเวลาจาก SNTP สำเร็จ เรียกคำสั่ง `localtime_r` นำค่าโครงสร้าง `tm` แปลงเป็น `Clock::DateTime` และสั่งเขียนลง RTC ผ่าน `rtc.setTime(dt)`

#### 3) ปัญหาที่พบและแนวทางแก้ไข
- **ปัญหา:** คอมไพเลอร์ฟ้อง Error บรรทัด `ESP_LOGI` และ `dt.date` ระบุว่า `struct Clock::DateTime has no member named 'date'` และเกิดข้อผิดพลาดฟังก์ชัน `read`/`write` ไม่ตรงกัน
- **สาเหตุ:** ชื่อตัวแปรวันที่ใน `struct DateTime` ถูกนิยามไม่ตรงกันระหว่างไฟล์ Header (`DS1307.hpp`) กับไฟล์เรียกใช้ (`day` เทียบกับ `date`) และคลาส `I2CBus` ขาดเมธอดที่รองรับชื่อ `writeBytes`/`readBytes`
- **การแก้ไข:** ทำการ Refactor คลาส `DateTime` ให้มีตัวแปร `day` เป็นมาตรฐานเดียวกันทุกไฟล์ และปรับแต่งคลาส `I2CBus` ให้มีเมธอด Overload ทั้ง `writeBytes`, `readBytes`, และ `writeReg` ครอบคลุมทุกการเรียกใช้

#### 4) ภาพผลลัพธ์การทดลอง
![alt text](68030151_DigitalClock_Wokwi/pictures/lab3_1.png)

![alt text](68030151_DigitalClock_Wokwi/pictures/lab3_2.png)

![alt text](68030151_DigitalClock_Wokwi/pictures/lab3_3.png)

![alt text](68030151_DigitalClock_Wokwi/pictures/lab3_4.png)

![alt text](68030151_DigitalClock_Wokwi/pictures/lab3_5.png)

---

### 2.4 ปฏิบัติการที่ 4 (Lab 4): Finite State Machine (FSM) & Interactive Feature Set

#### 1) วัตถุประสงค์
- เพื่อพัฒนาระบบควบคุมการทำงานของนาฬิกาด้วยโครงสร้าง **Finite State Machine (FSM)** ที่รองรับการสลับโหมดผ่านปุ่ม `MODE`
- เพื่อสร้างหน้าจอและระบบการทำงานแบบโต้ตอบ 4 สถานะหลัก:
  1. `CLOCK_MAIN`: แสดงเวลาหลัก พร้อมเปิด/ปิดระบบเตือน
  2. `ALARM_SET`: ปรับตั้งเวลาปลุก (ชั่วโมง/นาที)
  3. `STOPWATCH`: ระบบนาฬิกาจับเวลาละเอียดระดับมิลลิวินาที
  4. `TIME_SET`: โหมดปรับแต่งเวลาบน RTC ด้วยตนเอง (Manual Clock Adjust)

#### 2) ขั้นตอนการดำเนินงาน
1. กำหนด `enum class ClockState` เพื่อควบคุมสเตตในลูปหลักของแอปพลิเคชัน
2. ออกแบบระบบรับและดึงค่าจาก `buttonQueue` มาประมวลผลตามบริบทของสถานะปัจจุบัน (Contextual Button Handling)
3. พัฒนาระบบ Stopwatch โดยคำนวณเวลาส่วนต่างผ่านฟังก์ชันความละเอียดสูง `esp_timer_get_time()`
4. ออกแบบ UI Cursor Indicator (`^^`) บนจอ SSD1306 เพื่อชี้บอกผู้ใช้งานว่ากำลังแก้ไขตัวเลขฟิลด์ใดอยู่

#### 3) ปัญหาที่พบและแนวทางแก้ไข
- **ปัญหา:** ตัวเลขมิลลิวินาทีของโหมดจับเวลา (Stopwatch) แสดงผลกระตุกหรือไม่ลื่นไหล
- **สาเหตุ:** ค่าความถี่ของลูปหลัก `vTaskDelay` ถูกตั้งไว้ช้าเกินไป (100 ms หรือ 10 Hz)
- **การแก้ไข:** ปรับลดรอบ Delay ลงมาอยู่ที่ $50\text{ ms}$ (ความถี่ 20 Hz) ร่วมกับคำสั่ง `vTaskDelayUntil()` เพื่อควบคุมจังหวะรอบให้คงที่ ทำให้ตัวจับเวลาแสดงผลระดับมิลลิวินาทีได้อย่างแม่นยำและนุ่มนวล

#### 4) ภาพผลลัพธ์การทดลอง
![alt text](68030151_DigitalClock_Wokwi/pictures/lab4_1.png)

![alt text](68030151_DigitalClock_Wokwi/pictures/lab4_2.png)

![alt text](68030151_DigitalClock_Wokwi/pictures/lab4_3.png)

![alt text](68030151_DigitalClock_Wokwi/pictures/lab4_4.png)

![alt text](68030151_DigitalClock_Wokwi/pictures/lab4_5.png)

---

### 2.5 ปฏิบัติการที่ 5 (Lab 5): Non-Volatile Storage (NVS) & Alert Buzzer Integration

#### 1) วัตถุประสงค์
- เพื่อผสานรวมระบบแจ้งเตือนด้วยเสียงผ่าน **Piezo Buzzer** ที่ขับด้วยสัญญาณความถี่เสียงผ่านวงจร **ESP32 LEDC (PWM)**
- เพื่อพัฒนาการบันทึกสถานะและเวลาปลุกคงทนลงในหน่วยความจำถาวร **NVS (Non-Volatile Storage)** ป้องกันการสูญหายของข้อมูลเมื่อปิดเครื่องหรือเกิดไฟตก
- เพื่อสร้างระบบตรวจจับเงื่อนไขการปลุกและฟังก์ชันหยุดเสียงเตือนฉุกเฉิน (Alarm Dismissal)

#### 2) ขั้นตอนการดำเนินงาน
1. เพิ่มอุปกรณ์ Buzzer ในไฟล์ `diagram.json` โดยต่อสัญญาณเข้าที่ **GPIO 27**
2. ออกแบบคลาส `Clock::Buzzer` โดยใช้ ESP32 LEDC Timer กำหนดความถี่เสียงคลื่นสี่เหลี่ยมที่ $2{,}000\text{ Hz}$ (Duty Cycle 50%) และสร้างฟังก์ชัน `tick()` สำหรับสร้างจังหวะบี๊บเป็นช่วงๆ
3. ใช้ไลบรารี `nvs_flash` ในการสร้างฟังก์ชัน `loadAlarmFromNVS()` และ `saveAlarmToNVS()` เพื่อจัดเก็บตัวแปร `alarmHour`, `alarmMinute`, และ `alarmEnabled`
4. สร้างเงื่อนไขเปรียบเทียบเวลาแบบเรียลไทม์ หากเวลาปัจจุบันตรงกับเวลาปลุก จะสั่งให้ Buzzer ทำงานและเปลี่ยนหน้าจอ OLED เป็นหน้าจอกะพริบเตือน โดยผู้ใช้สามารถกดปุ่มใดก็ได้เพื่อหยุดการปลุกทันที

#### 3) ปัญหาที่พบและแนวทางแก้ไข
- **ปัญหา:** ในโปรแกรมจำลอง Wokwi มีไอคอนตัวโน้ตดนตรี 🎵 ลอยออกมาจาก Buzzer แต่ไม่มีเสียงออกจากลำโพงคอมพิวเตอร์
- **สาเหตุ:** ในขั้นต้นใช้ฟังก์ชัน `gpio_set_level` สลับเปิด-ปิดช้าๆ ซึ่งชิ้นส่วนใน Wokwi ทำงานเป็น Passive Piezo Buzzer ที่ต้องการความถี่เสียงจริง และระบบความปลอดภัยของเว็บบราวเซอร์ (Autoplay Policy / VS Code Webview) ปิดกั้นเสียงจาก Web Audio API อัตโนมัติหากยังไม่มีการคลิกบนหน้าต่าง
- **การแก้ไข:** ปรับเปลี่ยนไดรเวอร์ของ Buzzer มาใช้สัญญาณ PWM ความถี่ $2{,}000\text{ Hz}$ ผ่านไดรเวอร์ `ledc` ของ ESP-IDF และปรับเพิ่มค่า Volume เป็น `1.0` ในไฟล์ `diagram.json` พร้อมทำการคลิกที่หน้าต่างจำลองเพื่อให้ Browser อนุญาตการเล่นเสียง

#### 4) ภาพผลลัพธ์การทดลอง
![alt text](68030151_DigitalClock_Wokwi/pictures/lab5.png)

---

## 3. สรุปผลการทดลองและโครงสร้างสถาปัตยกรรมระบบ (Summary & Architecture)

การทดลองตลอดทั้ง 5 ปฏิบัติการประสบความสำเร็จอย่างสมบูรณ์ ระบบนาฬิกาดิจิทัลสามารถทำงานร่วมกันได้อย่างมีประสิทธิภาพ มีความทนทานต่อข้อผิดพลาด และแยกส่วนโค้ดออกเป็นโมดูลตามหลักการออกแบบสถาปัตยกรรมที่ดี:

```
                  +----------------------------------------------+
                  |                 main.cpp                     |
                  |     (Application Logic & FSM State Machine)  |
                  +-------+--------------+---------------+-------+
                          |              |               |
     +--------------------+   +----------+--------+   +--+-------------------+
     |   ButtonManager    |   |     WifiSync      |   |      Buzzer          |
     | (ISR & FreeRTOS Q) |   | (Wi-Fi STA & SNTP)|   | (LEDC PWM Alert Sys) |
     +--------------------+   +----------+--------+   +----------------------+
                                         |
     +--------------------+              |            +----------------------+
     |      SSD1306       |              |            |       DS1307         |
     | (OLED Framebuffer) |              |            |   (Real-Time Clock)  |
     +---------+----------+              |            +----------+-----------+
               |                         |                       |
               +-------------------+     |     +-----------------+
                                   |     |     |
                            +------+-----+-----+------+
                            |       Clock::I2CBus     |
                            | (Hardware I2C Master)   |
                            +-------------------------+
```

### การจัดการโครงงานผ่าน Git Version Control
โครงงานนี้ถูกจัดระเบียบและบันทึกประวัติการพัฒนาแยกตาม Branch อย่างชัดเจน:
- `lab1`: พัฒนา I2C HAL, DS1307 RTC, และ SSD1306 Display Driver
- `lab2`: ติดตั้งระบบ Button Interrupts และ FreeRTOS Queue Management
- `lab3`: เชื่อมต่อ Wi-Fi และซิงก์เวลาโลกผ่าน SNTP
- `lab4`: พัฒนาระบบ FSM, โหมดตั้งปลุก, นาฬิกาจับเวลา, และปรับเวลา Manual
- `lab5`: ผสานรวมระบบทั้งหมด พร้อมเพิ่มหน่วยความจำถาวร NVS และระบบเสียงเตือน Buzzer
- `main`: รวมโค้ดตัวเต็มของทุกแล็บเข้าสู่ระบบหลักเพื่อพร้อมส่งงานและใช้งานจริง