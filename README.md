# GuardianTone

GuardianTone is an ESP32-based IoT guitar monitoring system designed to help protect wooden guitars from unsafe environmental conditions and accidental impacts.

The system will monitor temperature, humidity, and movement around the instrument, generate alerts when potentially unsafe conditions occur, and eventually upload monitoring data to a cloud dashboard. :contentReference[oaicite:0]{index=0}

## Currently Implemented

The current version is an early software prototype and does not require the physical hardware yet.

- Simulated temperature and humidity readings
- Simulated accelerometer / movement readings
- Basic impact detection
- Preliminary Guitar Health Score calculation
- Armed / disarmed monitoring state
- Alert handling placeholders
- Cloud upload placeholder using JSON-style Serial output
- Modular code structure for sensors, health scoring, alerts, and cloud communication

## Planned Features

Once the hardware is available, the simulated components will be replaced with real sensor integration.

Planned additions include:

- Real temperature and humidity sensor support
- LSM6DSO accelerometer and gyroscope integration
- Capacitive touch control for arming and disarming monitoring
- LED and buzzer alerts
- Wi-Fi connectivity
- Cloud data storage and communication
- Historical temperature and humidity tracking
- Movement and impact event logging
- Web dashboard
- Improved Guitar Health Score calculation

The final system is intended to display current environmental conditions, historical data, movement events, alerts, and the Guitar Health Score through an online dashboard. :contentReference[oaicite:1]{index=1}

## Hardware

Planned hardware includes:

- ESP32 DevKit
- Temperature / humidity sensor
- LSM6DSO accelerometer / gyroscope
- Capacitive touch sensor
- LEDs
- Buzzer
- Push button
- Breadboard and jumper wires

## Status

🚧 **Early Development**

The project currently uses simulated sensor data so that the core application logic can be developed before hardware integration.