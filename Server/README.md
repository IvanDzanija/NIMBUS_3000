# Server

This directory contains the local services used by the WhatsDown demo:

- `index.py` runs a Flask server for camera relay and MNIST sketchpad
  prediction.
- `mnist.py` trains a small CNN and writes `mnist_cnn.pt`.
- `mnist_cnn.pt` is the trained model loaded by `index.py`.
- `mosquitto.conf` starts a local MQTT broker on port `1883`.
- `pyproject.toml` and `uv.lock` define the Python environment.

## Run

Start MQTT in one terminal:

```bash
mkdir -p /tmp/mosquitto
mosquitto -c ./mosquitto.conf -v
```

Start Flask in another terminal:

```bash
uv sync
uv run python index.py
```

## Endpoints

- `POST /upload` stores the latest JPEG frame from the ESP32-CAM node.
- `GET /latest` returns the latest JPEG frame for the kid board.
- `GET /video_feed` streams the latest frames for browser debugging.
- `POST /predict` accepts raw `gray8` image bytes with `X-Width` and `X-Height`
  headers and returns `{ "digit": ..., "confidence": ... }`.

`Server/data/` is ignored because MNIST data is downloaded again by `mnist.py`
when retraining is needed.
