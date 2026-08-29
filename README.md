# GuardianTone

GuardianTone is an ESP32-based IoT guitar monitoring system designed to help protect wooden guitars from unsafe environmental conditions and accidental impacts.

## Currently Implemented

- DHT20 temperature/humidity collection over I2C (`0x38`)
- LSM6DSO acceleration collection over I2C (`0x6A`/`0x6B`)
- I2C startup scan and sensor diagnostics
- Basic impact detection
- Preliminary Guitar Health Score calculation
- Armed/disarmed monitoring state in software
- Red LED and buzzer output for detected impacts
- ESP32 Wi-Fi connection
- Azure IoT Hub device-to-cloud telemetry over HTTPS
- JSON telemetry containing temperature, humidity, acceleration, health score, armed state, alert state, and uptime
- Graceful `null` values when a sensor is not detected instead of fabricated measurements
- Flask dashboard for live readings, history, impact alerts, and Guitar Health Score
- SQLite storage for dashboard telemetry history

## Azure / Wi-Fi Setup

1. In Azure, create an **IoT Hub** and add a device identity for the ESP32.
2. Generate a **device-scoped SAS token** for that device. Do not use the `iothubowner` service policy in firmware.
3. Copy `include/secrets.example.h` to `include/secrets.h`.
4. Fill in:
   - `WIFI_SSID`
   - `WIFI_PASSWORD`
   - `AZURE_IOT_HUB_NAME`
   - `AZURE_DEVICE_ID`
   - `AZURE_SAS_TOKEN`
5. For a secure final setup, also add an Azure-trusted root CA PEM to `AZURE_ROOT_CA`. If it is left empty, the current prototype will use insecure TLS and print a warning.

The device sends telemetry to:

```text
https://<hub>.azure-devices.net/devices/<device>/messages/events?api-version=2021-04-12
```

A successful upload prints `HTTP 204` in the serial monitor.

Example payload:

```json
{
  "temperature": 23.4,
  "humidity": 49.8,
  "acceleration": 1.003,
  "healthScore": 100,
  "armed": true,
  "alert": false,
  "uptimeMs": 30000
}
```

## Hardware / Pins

- I2C SDA: GPIO 21
- I2C SCL: GPIO 22
- DHT20: I2C address `0x38`
- LSM6DSO: I2C address `0x6A` or `0x6B`
- Red LED: GPIO 25
- Yellow LED: GPIO 26
- Buzzer: GPIO 27
- Capacitive touch sensor (planned): GPIO 33

## Build, Upload, and Run

### 1. Build and upload the ESP32 firmware

From the project root:

```bash
pio run
pio run --target upload
pio device monitor
```

Serial monitor speed is `115200`.

A successful Azure upload should print:

```text
Azure telemetry sent successfully (HTTP 204).
```

Leave the ESP32 powered on after uploading so it can continue sending telemetry to Azure IoT Hub.

### 2. Set up the dashboard Python environment

The dashboard requires Python 3.11 or newer.

From the `dashboard` directory, create a virtual environment.

**macOS / Linux**

```bash
cd dashboard
python3.11 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
pip install -r requirements.txt
```

**Windows PowerShell**

```powershell
cd dashboard
py -3.11 -m venv .venv
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
pip install -r requirements.txt
```

The dashboard dependencies include Flask, Azure Event Hubs support, and `python-dotenv`.

### 3. Configure the dashboard Azure connection

Create `dashboard/.env`. This file contains credentials and must not be committed to Git.

```text
EVENTHUB_CONNECTION_STRING=Endpoint=sb://<event-hub-endpoint>/;SharedAccessKeyName=service;SharedAccessKey=<service-key>;EntityPath=<event-hub-path>
EVENTHUB_CONSUMER_GROUP=guardiantone-dashboard
DEVICE_ID=guardiantone-esp32
EVENTHUB_NAME=
```

The Event Hubs-compatible endpoint and entity path come from the IoT Hub's built-in Events endpoint.

The consumer group must exist in the IoT Hub. For this project it is:

```text
guardiantone-dashboard
```

Do not commit `dashboard/.env`.

### 4. Run the dashboard

With the dashboard virtual environment active:

```bash
python app.py
```

Open the local address printed by Flask in a browser.

Typical address:

```text
http://127.0.0.1:5000
```

On macOS, port `5000` may already be used by AirPlay Receiver. If so, run the Flask app on another port such as `5001` and open:

```text
http://127.0.0.1:5001
```

The dashboard should show:

- Current temperature
- Current humidity
- Acceleration magnitude
- Guitar Health Score
- Armed/disarmed state
- Alert state
- Environment history
- Movement history
- Recent alerts
- Number of stored readings

### 5. Normal demo workflow

Run the system in this order:

```text
1. Upload firmware with PlatformIO
2. Start the PlatformIO serial monitor
3. Confirm Azure telemetry returns HTTP 204
4. Start the dashboard virtual environment
5. Run python app.py
6. Open the local dashboard in a browser
7. Leave the ESP32 running so new readings continue to populate the dashboard
```

Data flow:

```text
Sensors -> ESP32 -> Azure IoT Hub -> Flask dashboard -> SQLite -> Browser
```

## Planned Features

- Capacitive touch control for arming/disarming
- Yellow LED warning-state behavior for environmental conditions
- Improved health-score and impact calibration
- Secure production credential handling / certificate validation

## Notes

If the DHT20 or LSM6DSO is not detected, GuardianTone keeps running and uploads `null` for the unavailable measurement. This makes it possible to continue testing Azure connectivity even while troubleshooting hardware.
