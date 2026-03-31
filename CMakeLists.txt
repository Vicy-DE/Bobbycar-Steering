# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html

cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF build system
set(BLUEPAD32_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/components/bluepad32)
set(BTSTACK_ROOT ${BLUEPAD32_ROOT}/external/btstack)
set(EXTRA_COMPONENT_DIRS
    "components"
    "${BLUEPAD32_ROOT}/src/components"
)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(bobbycar-steering)

# Display hardware variant (visible to all components):
#   0 = ILI9488 4.0" + XPT2046 resistive SPI touch  (18-bit RGB666)
#   1 = ST7796S 4.0" + FT6236 capacitive I2C touch   (16-bit RGB565)
add_compile_definitions(DISPLAY_VARIANT=0)

# Suppress format warnings in third-party Bluepad32 / BTstack components.
# RISC-V toolchain defines int32_t as 'long int', causing -Wformat errors
# when %d is used for int32_t in Bluepad32 source.
if(TARGET __idf_bluepad32)
    target_compile_options(__idf_bluepad32 PRIVATE -Wno-format)
endif()
if(TARGET __idf_btstack)
    target_compile_options(__idf_btstack PRIVATE -Wno-format)
endif()

# ESP32-H2 Kconfig does not expose CONFIG_BT_LE_MSYS_BUF_FROM_HEAP.
# Without it, the BLE controller's r_esp_ble_msys_init() fails with -4.
# Force heap-based MSYS allocation so the controller can allocate buffers.
if(CONFIG_IDF_TARGET_ESP32H2)
    if(TARGET __idf_bt)
        target_compile_definitions(__idf_bt PRIVATE CONFIG_BT_LE_MSYS_BUF_FROM_HEAP=1)
    endif()
endif()
