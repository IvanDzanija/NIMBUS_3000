from flask import Flask, request, Response
import threading

app = Flask(__name__)

latest_frame = None
lock = threading.Lock()


def frame_generator():
    """Generator that yields the latest frame for MJPEG streaming."""
    while True:
        with lock:
            if latest_frame is None:
                continue
            # Standard MJPEG format
            yield (
                b"--frame\r\nContent-Type: image/jpeg\r\n\r\n" + latest_frame + b"\r\n"
            )


@app.route("/upload", methods=["POST"])
def upload():
    global latest_frame
    # We use request.get_data() for speed
    data = request.get_data()

    with lock:
        latest_frame = data

    return "OK", 200


@app.route("/latest")
def latest():
    """Single JPEG frame for ESP32 Kid Board polling (FrameReceiver)."""
    with lock:
        frame = latest_frame
    if frame is None:
        return "No frame yet", 503
    return Response(frame, mimetype="image/jpeg")


@app.route("/video_feed")
def video_feed():
    """Endpoint for the second ESP32 or a Browser to watch the stream."""
    return Response(
        frame_generator(), mimetype="multipart/x-mixed-replace; boundary=frame"
    )


@app.route("/")
def index():
    return """
    <html>
        <body>
            <h1>ESP32 Real-Time Stream</h1>
            <img src="/video_feed" width="320" height="240"/>
        </body>
    </html>
    """


if __name__ == "__main__":
    # Use threaded=True to handle upload and download simultaneously
    app.run(host="0.0.0.0", port=8080, threaded=True, debug=False)
