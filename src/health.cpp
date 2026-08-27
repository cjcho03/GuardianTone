#include <Arduino.h>
#include <math.h>
#include "health.h"

constexpr float MIN_SAFE_HUMIDITY = 45.0f;
constexpr float MAX_SAFE_HUMIDITY = 55.0f;
constexpr float MIN_SAFE_TEMP_C = 18.0f;
constexpr float MAX_SAFE_TEMP_C = 27.0f;

int calculateHealthScore(const SensorData& data) {
    // If the environmental sensor is unavailable, do not invent a score.
    if (!data.environmentValid || isnan(data.temperatureC) || isnan(data.humidity)) {
        return -1;
    }

    int score = 100;

    if (data.humidity < MIN_SAFE_HUMIDITY) {
        score -= static_cast<int>((MIN_SAFE_HUMIDITY - data.humidity) * 3);
    } else if (data.humidity > MAX_SAFE_HUMIDITY) {
        score -= static_cast<int>((data.humidity - MAX_SAFE_HUMIDITY) * 3);
    }

    if (data.temperatureC < MIN_SAFE_TEMP_C) {
        score -= static_cast<int>((MIN_SAFE_TEMP_C - data.temperatureC) * 2);
    } else if (data.temperatureC > MAX_SAFE_TEMP_C) {
        score -= static_cast<int>((data.temperatureC - MAX_SAFE_TEMP_C) * 2);
    }

    return constrain(score, 0, 100);
}
