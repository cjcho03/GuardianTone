#pragma once

#include "sensors.h"

void initCloud();
bool cloudConnected();
bool uploadSensorData(
    const SensorData& data,
    int healthScore,
    bool armed,
    bool alert
);
