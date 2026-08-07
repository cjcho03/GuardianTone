#include <Arduino.h>
#include "alerts.h"

constexpr float IMPACT_THRESHOLD_G = 2.0f;

void initAlerts() {
    // Real LED/buzzer pins will go here later.
}

bool detectImpact(const SensorData& data, bool armed) {
    if (!armed) {
        return false;
    }

    return data.accelerationG >= IMPACT_THRESHOLD_G;
}

void activateAlert() {
    Serial.println("!!! GUITAR IMPACT DETECTED !!!");

    // TODO:
    // digitalWrite(RED_LED_PIN, HIGH);
    // tone(BUZZER_PIN, 2000);
}

void clearAlert() {
    // TODO:
    // digitalWrite(RED_LED_PIN, LOW);
    // noTone(BUZZER_PIN);
}