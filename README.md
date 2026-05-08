# WhatsDown

WhatsDown is an ESP32-based communication and activity prototype for a child and
parent. The main ESP32 board runs a touch UI with quick messages, received
messages, an alarm, a camera preview/gallery, a sketchpad with digit prediction,
color sensing, and embedded sound playback. A companion ESP32-CAM streams JPEG
frames to a Python server, and a Flutter app lets a parent chat with the board
over MQTT.

The on-device UI text is mostly Croatian because the prototype was built for
that demo context. The repository documentation is in English for GitHub.

## What Is Inside

| Path | Purpose |
| --- | --- |
| `main/` | ESP-IDF firmware entry point, Wi-Fi setup, MQTT chat, camera receive/send modes, sensors, audio, and app wiring. |
| `components/ui_app/` | LVGL/SquareLine UI component, custom screen logic, message bridge, camera canvas, and photo gallery buffer. |
| `components/lvgl/`, `components/lvgl_esp32_drivers/` | Third-party LVGL and ESP32 display/touch drivers used by the firmware. |
| `Server/` | Flask service for camera relay and sketchpad MNIST prediction, plus Mosquitto broker config. |
| `whatsdownmobile/` | Flutter parent chat app that talks to the same MQTT broker as the ESP32 board. |
| `sounds/` | PCM sound assets embedded into the firmware. |
| `scripts/` | Helper scripts for applying the LVGL driver patch and regenerating compact UI image C files. |
| `patches/` | Patch needed for the LVGL ESP32 driver version used by this project. |
| `doc/` | Hardware documentation and schematics for the ByteLab development kit. |

## Architecture

1. The ESP32 kid board connects to Wi-Fi, starts LVGL, subscribes to
   `chat/<parent>/<kid>`, and publishes quick replies to `chat/<kid>/<parent>`.
2. The ESP32-CAM firmware mode posts JPEG frames to `POST /upload` on the
   Python server.
3. The kid board pulls `GET /latest`, decodes the JPEG into an LVGL canvas, and
   can store small thumbnails in the gallery.
4. The sketchpad posts a 96x96 grayscale image to `POST /predict`; the server
   returns the predicted digit and confidence.
5. The Flutter app connects to the same MQTT broker and exchanges messages with
   the kid board.

## Firmware Setup

Requirements:

- ESP-IDF 5.3 or newer
- ESP32 display board compatible with the configured LVGL drivers
- Optional ESP32-CAM board for the camera node
- Mosquitto broker and the Python server running on the same network

Build the main kid board firmware:

```bash
. "$IDF_PATH/export.sh"
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

In `menuconfig`, open `WhatsDown application` and set:

- `Wi-Fi SSID`
- `Wi-Fi password`
- `Python server base URL`, for example `http://192.168.1.10:8080`
- `MQTT broker URI`, for example `mqtt://192.168.1.10:1883`

To flash the ESP32-CAM node, enable `Build as ESP32-CAM streaming node` in the
same menu before building.

## Server Setup

Terminal 1:

```bash
cd Server
mkdir -p /tmp/mosquitto
mosquitto -c mosquitto.conf -v
```

Terminal 2:

```bash
cd Server
uv sync
uv run python index.py
```

The server listens on port `8080`. The MQTT broker listens on port `1883`.

## Mobile App

```bash
cd whatsdownmobile
flutter pub get
flutter run --dart-define=BROKER_IP=192.168.1.10 --dart-define=MY_ID=Mom
```

Use `MY_ID=Dad` to run the second parent identity. `KID_ID` defaults to `kid`.

## Repository Cleanup

Generated editor history, local VS Code settings, and downloaded MNIST raw data
are ignored. The trained `Server/mnist_cnn.pt` model is kept because it is needed
to run the prediction server without retraining.
