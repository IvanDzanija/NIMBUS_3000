# Firmware Main Component

This is the ESP-IDF application component for the NIMBUS 3000 boards.

## Important Files

- `app_main.cpp` selects the board mode, connects Wi-Fi, starts LVGL, MQTT,
  camera receiving/streaming, sketchpad prediction, alarm checks, and audio.
- `Kconfig.projbuild` exposes Wi-Fi, server, MQTT, kid ID, and camera-node mode
  in `idf.py menuconfig`.
- `MessengerApp.h`, `ChatManager.h`, `ChatSession.h`, `Message.h`, and
  `MQTTManager.h` implement the MQTT chat flow.
- `CameraStreamer.h` posts ESP32-CAM JPEG frames to the Flask server.
- `FrameReceiver.h` pulls JPEG frames from the Flask server for the kid board
  UI.
- `max98357a.*`, `potentiometer.*`, and `sounds/` handle embedded audio.
- `sensors.*`, `APDS_9960.*`, `veml7700.*`, and `i2c_manager.*` handle sensor
  access used by the sketchpad/color features.
- `gui.*` initializes LVGL, display, touch input, and the UI component.

The firmware now sends sketchpad recognition to the Python server, so old unused
embedded MNIST model files were removed from this component.
