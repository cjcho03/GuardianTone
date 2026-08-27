#include <Arduino.h>

#include "sensors.h"
#include "health.h"
#include "alerts.h"
#include "cloud.h"

constexpr unsigned long SENSOR_INTERVAL_MS = 2000;
constexpr unsigned long CLOUD_UPLOAD_INTERVAL_MS = 10000;

unsigned long lastSensorRead = 0;
unsigned long lastCloudUpload = 0;
bool armed = true;

SensorData latestData = {NAN, NAN, NAN, false, false};
int latestHealthScore = -1;
bool latestImpactDetected = false;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("===============================");
    Serial.println("         GuardianTone");
    Serial.println("===============================");

    initSensors();
    initAlerts();
    initCloud();

    Serial.println("GuardianTone started.");
}

void loop() {
    unsigned long now = millis();

    if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
        lastSensorRead = now;

        latestData = readSensors();
        latestHealthScore = calculateHealthScore(latestData);
        latestImpactDetected = detectImpact(latestData, armed);

        if (latestImpactDetected) {
            activateAlert();
        } else {
            clearAlert();
        }
    }

    // Upload less often than we sample so the device can collect data at a
    // useful rate without unnecessarily flooding IoT Hub.
    if (now - lastCloudUpload >= CLOUD_UPLOAD_INTERVAL_MS) {
        lastCloudUpload = now;

        uploadSensorData(
            latestData,
            latestHealthScore,
            armed,
            latestImpactDetected
        );
    }
}
