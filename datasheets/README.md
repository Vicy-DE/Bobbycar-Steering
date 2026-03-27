# Datasheets

Place ESP32 datasheets and technical reference manuals here.

## Required Documents

Download from https://www.espressif.com/en/support/documents/technical-documents

### ESP32-C3
- `esp32-c3_datasheet_en.pdf` — [Download](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- `esp32-c3_technical_reference_manual_en.pdf`

### ESP32-H2
- `esp32-h2_datasheet_en.pdf` — [Download](https://www.espressif.com/sites/default/files/documentation/esp32-h2_datasheet_en.pdf)
- `esp32-h2_technical_reference_manual_en.pdf`

### ESP32-C5
- `esp32-c5_datasheet_en.pdf`
- `esp32-c5_technical_reference_manual_en.pdf`

## Display Module

### 3.5" SPI TFT — ST7796S (TENSTAR ROBOT)
- **Driver IC:** ST7796S (Sitronix)
- **Resolution:** 320 × 480 (landscape via MADCTL rotation)
- **Color Format:** RGB565, 16-bit
- **Interface:** SPI 4-wire (MOSI, SCK, CS, DC, RST, BL)
- **Touch:** None (standalone SPI display module)
- **Product Page:** [AliExpress 1005006175220737](https://de.aliexpress.com/item/1005006175220737.html)
- **ST7796S Datasheet:** Search "ST7796S datasheet Sitronix" — typically available from LCD module vendors
- **Compatible ESP-IDF driver:** `esp_lcd_new_panel_st7789()` with `esp_lcd_panel_invert_color(panel, false)`
