idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES display esp_adc esp_driver_gpio led_strip
)
