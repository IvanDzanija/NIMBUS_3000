from flask import Flask, request, jsonify

app = Flask(__name__)

messages = []


@app.route("/send", methods=["POST"])
def send():
    data = request.json
    messages.append(data)
    return jsonify({"status": "ok"})


@app.route("/recv", methods=["GET"])
def recv():
    if messages:
        return jsonify(messages.pop(0))
    return jsonify({"status": "empty"})


app.run(host="0.0.0.0", port=8080)
