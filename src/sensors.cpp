#include <Arduino.h>
#include "sensors.h"

void initSensors() {
    Serial.println("Sensors initialized in simulation mode.");
}

SensorData readSensors() {
    SensorData data;

    data.temperatureC = random(200, 261) / 10.0f;
    data.humidity = random(400, 601) / 10.0f;
    data.accelerationG = random(0, 80) / 100.0f;

    // Occassionally simulate an impact
    if (random(0, 20) == 0) {
        data.accelerationG = random(200, 400) / 100.0f;
    }

    return data;
}