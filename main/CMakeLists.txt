idf_component_register(
    SRCS "main.c"
         "my_platform.c"
         "storage.c"
         "motor.cpp"
         "steering_algo.cpp"
         "console.c"
         "console_cmds.c"
         "console_cmds_fs.c"
         "xmodem.c"
         "power_mgmt.c"
         "ini_parser.c"
         "bp32_config.c"
         "ble_console.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES display
                  esp_adc
                  esp_driver_gpio
                  led_strip
                  bluepad32
                  btstack
                  joltwallet__littlefs
)
