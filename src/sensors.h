#pragma once

struct SensorData {
    float temperatureC;
    float humidity;
    float accelerationG;
};

void initSensors();

SensorData readSensors();