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

## Build and Upload with PlatformIO

```bash
pio run
pio run --target upload
pio device monitor
```

Serial monitor speed is `115200`.

## Planned Features

- Capacitive touch control for arming/disarming
- Yellow LED warning-state behavior for environmental conditions
- Historical cloud storage
- Web dashboard for current readings, history, impact events, and Guitar Health Score
- Improved health-score and impact calibration
- Secure production credential handling / certificate validation

## Notes

If the DHT20 or LSM6DSO is not detected, GuardianTone keeps running and uploads `null` for the unavailable measurement. This makes it possible to continue testing Azure connectivity even while troubleshooting hardware.
