idf_component_register(
    SRCS "main.c" "my_platform.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES display esp_adc esp_driver_gpio led_strip bluepad32 btstack
)
