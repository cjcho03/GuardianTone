#include <Arduino.h>

#include "sensors.h"
#include "health.h"
#include "alerts.h"
#include "cloud.h"

constexpr unsigned long SENSOR_INTERNAL_MS = 2000;

unsigned long lastSensorRead = 0;
bool armed = true;

void setup() {
    Serial.begin(115200);

    initSensors();
    initAlerts();
    initCloud();

    Serial.println("GuardianTone started");
}

void loop() {
    unsigned long now = millis();

    if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
        lastSensorRead = now;

        SensorData data = readSensor();

        int healthScore = calculateHealthScore(data);

        bool impactDetected = detectImpact(data, armed);

        if (impactDetected) {
            activateAlert();
        } else {
            clearAlert();
        }

        uploadSensorData(
            data,
            healthScore,
            armed,
            impactDetected
        );
    }
}