# Components

ESP-IDF components used by the firmware.

- `ui_app/` is the project-owned LVGL/SquareLine UI component.
- `wifi_station/` is a reusable Wi-Fi station helper component kept for future
  use; the current app uses the configurable Wi-Fi setup in `main/app_main.cpp`.
- `lvgl/` is the vendored LVGL library.
- `lvgl_esp32_drivers/` is the vendored ESP32 display/touch driver layer patched
  for this project.

In normal project work, edit `ui_app/` and `main/`. Treat `lvgl/` and
`lvgl_esp32_drivers/` as third-party code unless you are updating the hardware
port.
