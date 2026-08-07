#pragma once

#include "sensors.h"

void initAlerts();

bool detectImpact(const SensorData& data, bool armed);

void activateAlert();
void clearAlert();