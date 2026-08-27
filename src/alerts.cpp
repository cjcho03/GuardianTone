#include <Arduino.h>
#include "alerts.h"

constexpr float IMPACT_THRESHOLD_G = 2.0f;

// GuardianTone output pins
constexpr uint8_t BUZZER_PIN = 27;
constexpr uint8_t YELLOW_LED_PIN = 26;
constexpr uint8_t RED_LED_PIN = 25;

void initAlerts() {
    // I2C devices use SDA 21 and SCL 22.
    // The capacitive touch sensor is currently planned for GPIO 33.
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);

    // Start with all alerts disabled.
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    noTone(BUZZER_PIN);

    Serial.println("Alert outputs initialized.");
}

bool detectImpact(const SensorData& data, bool armed) {
    if (!armed) {
        return false;
    }

    return data.motionValid && data.accelerationG >= IMPACT_THRESHOLD_G;
}

void activateAlert() {
    Serial.println("!!! GUITAR IMPACT DETECTED !!!");

    // A detected impact is treated as a critical alert.
    digitalWrite(RED_LED_PIN, HIGH);
    tone(BUZZER_PIN, 2000);
}

void clearAlert() {
    digitalWrite(RED_LED_PIN, LOW);
    noTone(BUZZER_PIN);
}
