#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "DS1307.hpp"

namespace Clock {

class WifiSync {
private:
    Clock::DS1307& rtc_;
    EventGroupHandle_t wifiEventGroup_;
    static const int WIFI_CONNECTED_BIT = BIT0;

    static void wifiEventHandler(void* arg, esp_event_base_t eventBase,
                                int32_t eventId, void* eventData);

public:
    explicit WifiSync(Clock::DS1307& rtc);
    bool initAndSync(const char* ssid, const char* pass);
    bool updateRtcFromSystem();
};

} // namespace Clock