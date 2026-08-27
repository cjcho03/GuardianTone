#pragma once

struct SensorData {
    float temperatureC;
    float humidity;
    float accelerationG;
    bool environmentValid;
    bool motionValid;
};

void initSensors();
SensorData readSensors();
