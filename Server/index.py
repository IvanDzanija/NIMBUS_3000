from pathlib import Path
import threading
import time
from typing import Optional

import cv2
from flask import Flask, Response, jsonify, request
import numpy as np
import torch


app = Flask(__name__)

BASE_DIR = Path(__file__).resolve().parent
MODEL_PATH = BASE_DIR / "mnist_cnn.pt"

latest_frame: Optional[bytes] = None
lock = threading.Lock()

device = torch.device("mps" if torch.backends.mps.is_available() else "cpu")


class MNISTCNN(torch.nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.conv = torch.nn.Sequential(
            torch.nn.Conv2d(1, 32, 3, padding=1),
            torch.nn.ReLU(),
            torch.nn.Conv2d(32, 64, 3, padding=1),
            torch.nn.ReLU(),
            torch.nn.MaxPool2d(2),
            torch.nn.Conv2d(64, 128, 3, padding=1),
            torch.nn.ReLU(),
            torch.nn.MaxPool2d(2),
        )

        self.fc = torch.nn.Sequential(
            torch.nn.Flatten(),
            torch.nn.Linear(128 * 7 * 7, 128),
            torch.nn.ReLU(),
            torch.nn.Dropout(0.3),
            torch.nn.Linear(128, 10),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.fc(self.conv(x))


model = MNISTCNN().to(device)
model.load_state_dict(torch.load(MODEL_PATH, map_location=device))
model.eval()


@app.route("/predict", methods=["POST"])
def predict():
    data = request.get_data()

    try:
        width = int(request.headers.get("X-Width", "0"))
        height = int(request.headers.get("X-Height", "0"))
    except ValueError:
        return jsonify({"error": "Invalid image size headers"}), 400

    fmt = request.headers.get("X-Format", "")
    if width <= 0 or height <= 0:
        return jsonify({"error": "Missing image size headers"}), 400
    if fmt != "gray8":
        return jsonify({"error": "Unsupported format"}), 400

    expected_size = width * height
    if len(data) != expected_size:
        return jsonify({"error": "Invalid data size"}), 400

    img = np.frombuffer(data, dtype=np.uint8).reshape((height, width))
    _, img = cv2.threshold(img, 127, 255, cv2.THRESH_BINARY)
    img = 255 - img
    img = cv2.resize(img, (28, 28))
    img = img / 255.0

    tensor = torch.tensor(img, dtype=torch.float32).unsqueeze(0).unsqueeze(0).to(device)

    with torch.no_grad():
        output = model(tensor)
        probs = torch.softmax(output, dim=1)
        pred = int(probs.argmax(dim=1).item())
        confidence = float(probs.max().item())

    return jsonify({"digit": pred, "confidence": round(confidence * 100)})


@app.route("/upload", methods=["POST"])
def upload():
    global latest_frame

    data = request.get_data()
    if not data:
        return "Empty frame", 400

    with lock:
        latest_frame = data

    return "OK", 200


@app.route("/latest")
def latest():
    with lock:
        frame = latest_frame

    if frame is None:
        return "No frame yet", 503

    return Response(frame, mimetype="image/jpeg")


@app.route("/video_feed")
def video_feed():
    return Response(
        frame_generator(), mimetype="multipart/x-mixed-replace; boundary=frame"
    )


@app.route("/")
def index():
    return "WhatsDown server running"


def frame_generator():
    while True:
        with lock:
            frame = latest_frame

        if frame is None:
            time.sleep(0.05)
            continue

        yield b"--frame\r\nContent-Type: image/jpeg\r\n\r\n" + frame + b"\r\n"


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080)
