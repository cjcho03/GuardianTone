#pragma once

#include "sensors.h"

void initCloud();

void uploadSensorData(
    const SensorData& data,
    int healthScore,
    bool armed,
    bool alert
);
