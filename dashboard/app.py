import json
import os
import sqlite3
import threading
from datetime import datetime, timezone
from pathlib import Path

from azure.eventhub import EventHubConsumerClient
from dotenv import load_dotenv
from flask import Flask, jsonify, render_template, request

BASE_DIR = Path(__file__).resolve().parent
DB_PATH = BASE_DIR / "data" / "guardiantone.db"
load_dotenv(BASE_DIR / ".env")

app = Flask(__name__)

EVENTHUB_CONNECTION_STRING = os.getenv("EVENTHUB_CONNECTION_STRING", "")
EVENTHUB_NAME = os.getenv("EVENTHUB_NAME", "")
CONSUMER_GROUP = os.getenv("EVENTHUB_CONSUMER_GROUP", "guardiantone-dashboard")
DEVICE_ID = os.getenv("DEVICE_ID", "guardiantone-esp32")

consumer_thread = None
consumer_started = False
consumer_error = None


def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    DB_PATH.parent.mkdir(parents=True, exist_ok=True)
    with get_db() as conn:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS readings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                received_at TEXT NOT NULL,
                device_id TEXT,
                temperature REAL,
                humidity REAL,
                acceleration REAL,
                health_score INTEGER,
                armed INTEGER,
                alert INTEGER,
                uptime_ms INTEGER
            )
            """
        )
        conn.execute(
            "CREATE INDEX IF NOT EXISTS idx_readings_received_at ON readings(received_at DESC)"
        )
        conn.commit()


def save_reading(payload: dict, device_id: str | None, received_at: datetime | None):
    timestamp = (received_at or datetime.now(timezone.utc)).astimezone(timezone.utc).isoformat()
    with get_db() as conn:
        conn.execute(
            """
            INSERT INTO readings (
                received_at, device_id, temperature, humidity, acceleration,
                health_score, armed, alert, uptime_ms
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                timestamp,
                device_id,
                payload.get("temperature"),
                payload.get("humidity"),
                payload.get("acceleration"),
                payload.get("healthScore"),
                int(bool(payload.get("armed"))) if payload.get("armed") is not None else None,
                int(bool(payload.get("alert"))) if payload.get("alert") is not None else None,
                payload.get("uptimeMs"),
            ),
        )
        conn.commit()


def decode_event(event):
    try:
        raw = b"".join(event.body).decode("utf-8")
        return json.loads(raw)
    except Exception:
        try:
            return event.body_as_json(encoding="UTF-8")
        except Exception:
            return None


def on_event(partition_context, event):
    payload = decode_event(event)
    if not isinstance(payload, dict):
        print("Ignored non-JSON telemetry event")
        return

    system_properties = event.system_properties or {}
    raw_device_id = system_properties.get(b"iothub-connection-device-id")
    if isinstance(raw_device_id, bytes):
        event_device_id = raw_device_id.decode("utf-8", errors="replace")
    else:
        event_device_id = raw_device_id or DEVICE_ID

    if DEVICE_ID and event_device_id and event_device_id != DEVICE_ID:
        return

    save_reading(payload, event_device_id, event.enqueued_time)
    print(f"Stored telemetry: {payload}")


def on_error(partition_context, error):
    global consumer_error
    consumer_error = str(error)
    partition = partition_context.partition_id if partition_context else "all"
    print(f"Event Hub receive error on partition {partition}: {error}")


def consume_events():
    global consumer_error
    consumer_error = None
    try:
        kwargs = {
            "conn_str": EVENTHUB_CONNECTION_STRING,
            "consumer_group": CONSUMER_GROUP,
        }
        if EVENTHUB_NAME:
            kwargs["eventhub_name"] = EVENTHUB_NAME

        client = EventHubConsumerClient.from_connection_string(**kwargs)
        print(f"Listening for {DEVICE_ID or 'all devices'} on consumer group {CONSUMER_GROUP}...")
        with client:
            client.receive(
                on_event=on_event,
                on_error=on_error,
                starting_position="@latest",
            )
    except Exception as exc:
        consumer_error = str(exc)
        print(f"Unable to start Azure Event Hub consumer: {exc}")


def start_consumer():
    global consumer_thread, consumer_started
    if consumer_started or not EVENTHUB_CONNECTION_STRING:
        return
    consumer_started = True
    consumer_thread = threading.Thread(target=consume_events, daemon=True)
    consumer_thread.start()


def row_to_dict(row):
    if row is None:
        return None
    result = dict(row)
    if result.get("armed") is not None:
        result["armed"] = bool(result["armed"])
    if result.get("alert") is not None:
        result["alert"] = bool(result["alert"])
    return result


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/latest")
def api_latest():
    with get_db() as conn:
        row = conn.execute("SELECT * FROM readings ORDER BY received_at DESC LIMIT 1").fetchone()
    return jsonify(row_to_dict(row))


@app.route("/api/history")
def api_history():
    limit = max(1, min(request.args.get("limit", default=120, type=int), 1000))
    with get_db() as conn:
        rows = conn.execute(
            "SELECT * FROM readings ORDER BY received_at DESC LIMIT ?", (limit,)
        ).fetchall()
    return jsonify([row_to_dict(row) for row in reversed(rows)])


@app.route("/api/alerts")
def api_alerts():
    limit = max(1, min(request.args.get("limit", default=20, type=int), 100))
    with get_db() as conn:
        rows = conn.execute(
            "SELECT * FROM readings WHERE alert = 1 ORDER BY received_at DESC LIMIT ?", (limit,)
        ).fetchall()
    return jsonify([row_to_dict(row) for row in rows])


@app.route("/api/status")
def api_status():
    with get_db() as conn:
        count = conn.execute("SELECT COUNT(*) FROM readings").fetchone()[0]
    return jsonify(
        {
            "azureConfigured": bool(EVENTHUB_CONNECTION_STRING),
            "consumerGroup": CONSUMER_GROUP,
            "deviceId": DEVICE_ID,
            "storedReadings": count,
            "consumerError": consumer_error,
        }
    )


if __name__ == "__main__":
    init_db()
    start_consumer()
    app.run(host="127.0.0.1", port=5000, debug=False)
