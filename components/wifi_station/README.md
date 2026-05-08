# Wi-Fi Station Component

Reusable ESP-IDF Wi-Fi station helper.

It reads `CONFIG_WIFI_STATION_SSID` and `CONFIG_WIFI_STATION_PASSWORD` from its
own `Kconfig`, starts Wi-Fi in station mode, reconnects after disconnects, and
signals when an IP address is received.

The current WhatsDown app uses the simpler Wi-Fi setup in `main/app_main.cpp`,
but this component is kept as a reusable option for future refactoring.
