idf_component_register(
    SRCS 
        "main.cpp"
        "DS1307.cpp"
        "SSD1306.cpp"
        "ButtonManager.cpp"
        "WifiSync.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
        driver
        esp_timer
        esp_wifi
        esp_event
        nvs_flash
        lwip
)