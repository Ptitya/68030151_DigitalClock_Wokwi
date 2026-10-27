#include "WifiSync.hpp"
#include "esp_log.h"
#include <ctime>
#include <sys/time.h>

static const char* TAG = "WIFI_SYNC";

namespace Clock {

void WifiSync::wifiEventHandler(void* arg, esp_event_base_t eventBase,
                                int32_t eventId, void* eventData) {
    auto* self = static_cast<WifiSync*>(arg);
    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Wi-Fi connected and got IP!");
        xEventGroupSetBits(self->wifiEventGroup_, WIFI_CONNECTED_BIT);
    }
}

WifiSync::WifiSync(Clock::DS1307& rtc) : rtc_(rtc) {
    wifiEventGroup_ = xEventGroupCreate();
}

bool WifiSync::initAndSync(const char* ssid, const char* pass) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, this, nullptr);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, this, nullptr);

    wifi_config_t wifiConfig = {};
    snprintf(reinterpret_cast<char*>(wifiConfig.sta.ssid), sizeof(wifiConfig.sta.ssid), "%s", ssid);
    snprintf(reinterpret_cast<char*>(wifiConfig.sta.password), sizeof(wifiConfig.sta.password), "%s", pass);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
    esp_wifi_start();

    ESP_LOGI(TAG, "Connecting to Wi-Fi SSID: %s ...", ssid);

    EventBits_t bits = xEventGroupWaitBits(wifiEventGroup_, WIFI_CONNECTED_BIT,
                                           pdFALSE, pdTRUE, pdMS_TO_TICKS(15000));

    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi");
        return false;
    }

    ESP_LOGI(TAG, "Initializing SNTP...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    setenv("TZ", "ICT-7", 1);
    tzset();

    int retry = 0;
    const int maxRetries = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry <= maxRetries) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, maxRetries);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    if (retry > maxRetries) {
        ESP_LOGW(TAG, "SNTP sync timeout. Will retry later in background.");
        return false;
    }

    return updateRtcFromSystem();
}

bool WifiSync::updateRtcFromSystem() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    Clock::DateTime dt = {
        static_cast<uint8_t>(timeinfo.tm_sec),
        static_cast<uint8_t>(timeinfo.tm_min),
        static_cast<uint8_t>(timeinfo.tm_hour),
        static_cast<uint8_t>(timeinfo.tm_wday + 1),
        static_cast<uint8_t>(timeinfo.tm_mday),
        static_cast<uint8_t>(timeinfo.tm_mon + 1),
        static_cast<uint16_t>(timeinfo.tm_year + 1900)
    };

    ESP_LOGI(TAG, "RTC Updated to: %04d-%02d-%02d %02d:%02d:%02d",
             dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);

    return rtc_.setTime(dt);
}

} // namespace Clock