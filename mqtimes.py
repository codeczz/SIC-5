import json
from flask import Flask, request, jsonify
from datetime import datetime

app = Flask(__name__)

UDARA_DATA = []

@app.route("/")
def root_route():
    return "Hello world!"

@app.route("/udara")
def get_udara():
    return jsonify(UDARA_DATA)

@app.route("/submit-udara", methods=["POST"])
def post_udara():
    pesan = request.data.decode("utf8")
    timestamp = datetime.now().isoformat()
    UDARA_DATA.append({"udara": float(pesan), "timestamp": timestamp})
    print(f"Received udara: {pesan} at {timestamp}")
    return f"Received udara {pesan} at {timestamp}"

@app.route("/submit", methods=["POST"])
def post_data():
    pesan = request.data.decode("utf8")
    pesan = json.loads(pesan)
    timestamp = datetime.now().isoformat()
    UDARA_DATA.append({"udara": float(pesan["udara"]), "timestamp": timestamp})
    print(f"Received data: {pesan} at {timestamp}")
    return f"Received data {pesan} at {timestamp}"

if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0')
