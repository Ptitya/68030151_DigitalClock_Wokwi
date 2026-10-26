#include "ButtonManager.hpp"

namespace Clock {

// กำหนดตัวแปร Static ไว้นอก IRAM เพื่อหลีกเลี่ยง dangerous relocation
DRAM_ATTR static int64_t s_lastTime = 0;

void IRAM_ATTR ButtonManager::isrHandler(void* arg) {
    auto* self = static_cast<ButtonManager*>(arg);
    int64_t now = esp_timer_get_time();
    
    if (now - s_lastTime > DEBOUNCE_TIME_US) {
        s_lastTime = now;
        
        ButtonId pressed = ButtonId::MODE;
        if (gpio_get_level(GPIO_NUM_18) == 0) pressed = ButtonId::MODE;
        else if (gpio_get_level(GPIO_NUM_19) == 0) pressed = ButtonId::UP;
        else if (gpio_get_level(GPIO_NUM_4) == 0) pressed = ButtonId::DOWN;
        else if (gpio_get_level(GPIO_NUM_5) == 0) pressed = ButtonId::ENTER;
        else return;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(self->eventQueue_, &pressed, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

ButtonManager::ButtonManager(QueueHandle_t& queue) : eventQueue_(queue) {}

bool ButtonManager::init() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL << 18) | (1ULL << 19) | (1ULL << 4) | (1ULL << 5);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_18, isrHandler, this);
    gpio_isr_handler_add(GPIO_NUM_19, isrHandler, this);
    gpio_isr_handler_add(GPIO_NUM_4, isrHandler, this);
    gpio_isr_handler_add(GPIO_NUM_5, isrHandler, this);

    return true;
}

} // namespace Clock