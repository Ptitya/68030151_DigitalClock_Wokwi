idf_component_register(
    SRCS 
        "main.cpp"
        "DS1307.cpp"
        "SSD1306.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
        driver
)