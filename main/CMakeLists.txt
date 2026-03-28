idf_component_register(
    SRCS "main.c"
         "my_platform.c"
         "storage.c"
         "motor.c"
         "steering_algo.c"
         "console.c"
         "console_cmds.c"
         "console_cmds_fs.c"
         "xmodem.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES display
                  esp_adc
                  esp_driver_gpio
                  led_strip
                  bluepad32
                  btstack
                  joltwallet__littlefs
)
