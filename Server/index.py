from flask import Flask, request, Response, jsonify
import threading
import torch
import numpy as np
import cv2

app = Flask(__name__)

latest_frame = None
lock = threading.Lock()

# ==============================
# Device
# ==============================
device = torch.device("mps" if torch.backends.mps.is_available() else "cpu")


# ==============================
# Model definition (MNISTCNN)
# ==============================
class MNISTCNN(torch.nn.Module):
    def __init__(self):
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

    def forward(self, x):
        return self.fc(self.conv(x))


# ==============================
# Load model
# ==============================
model = MNISTCNN().to(device)
model.load_state_dict(torch.load("mnist_cnn.pt", map_location=device))
model.eval()


# ==============================
# Predict endpoint
# ==============================
@app.route("/predict", methods=["POST"])
def predict():
    # Read raw bytes
    data = request.get_data()

    # Read headers
    try:
        width = int(request.headers.get("X-Width"))
        height = int(request.headers.get("X-Height"))
        fmt = request.headers.get("X-Format")
        print(f"Received image: {width}x{height}, format={fmt}")
    except:
        return jsonify({"error": "Missing headers"}), 400

    if fmt != "gray8":
        return jsonify({"error": "Unsupported format"}), 400

    # Validate buffer size
    expected_size = width * height
    if len(data) != expected_size:
        return jsonify({"error": "Invalid data size"}), 400

    # Convert to numpy image
    img = np.frombuffer(data, dtype=np.uint8).reshape((height, width))

    # ==============================
    # Preprocessing
    # ==============================
    # Threshold
    _, img = cv2.threshold(img, 127, 255, cv2.THRESH_BINARY)

    # Invert: MNIST = white digit on black background
    img = 255 - img
    print("Preprocessed image shape:", img.shape)

    # Resize → 28x28 (MNIST)
    img = cv2.resize(img, (28, 28))

    # Normalize 0-1
    img = img / 255.0

    # Convert to tensor
    tensor = torch.tensor(img, dtype=torch.float32).unsqueeze(0).unsqueeze(0).to(device)

    # ==============================
    # Inference
    # ==============================
    with torch.no_grad():
        output = model(tensor)
        probs = torch.softmax(output, dim=1)
        pred = int(probs.argmax(dim=1).item())
        confidence = float(probs.max().item())

    print(f"Predicted: {pred} (confidence: {confidence:.4f})")

    return jsonify({"digit": pred})


def frame_generator():
    while True:
        with lock:
            if latest_frame is None:
                continue
            frame = latest_frame

        yield (b"--frame\r\nContent-Type: image/jpeg\r\n\r\n" + frame + b"\r\n")


# ==============================
# Upload endpoint
# ==============================
@app.route("/upload", methods=["POST"])
def upload():
    global latest_frame
    data = request.get_data()

    with lock:
        latest_frame = data

    return "OK", 200


# ==============================
# Get latest frame
# ==============================
@app.route("/latest")
def latest():
    with lock:
        if latest_frame is None:
            return "No frame yet", 503
        frame = latest_frame

    return Response(frame, mimetype="image/jpeg")


# ==============================
# Video stream
# ==============================
@app.route("/video_feed")
def video_feed():
    return Response(
        frame_generator(), mimetype="multipart/x-mixed-replace; boundary=frame"
    )


@app.route("/")
def index():
    return "MNIST server running"


# ==============================
# RUN
# ==============================
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080)
