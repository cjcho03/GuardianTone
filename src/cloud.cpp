#include <Arduino.h>
#include "cloud.h"

void initCloud() {
    Serial.println("Cloud connection placeholder initialized.");
}

void uploadSensorData(const SensorData& data,
    int healthScore, bool armed, bool alert)
{
    Serial.println("---Cloud Payload---");

    Serial.print("{");

    Serial.print("\"temperature\":");
    Serial.print(data.temperatureC);

    Serial.print(",\"humidity\":");
    Serial.print(data.humidity);

    Serial.print(",\"acceleration\":");
    Serial.print(data.accelerationG);

    Serial.print(",\"healthScore\":");
    Serial.print(healthScore);

    Serial.print(",\"armed\":");
    Serial.print(armed ? "true" : "false");

    Serial.print(",\"alert\":");
    Serial.print(alert ? "true" : "false");

    Serial.println("}");
}