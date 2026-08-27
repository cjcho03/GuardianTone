#include <Arduino.h>
#include "alerts.h"

constexpr float IMPACT_THRESHOLD_G = 2.0f;

// GuardianTone output pins
constexpr uint8_t BUZZER_PIN = 27;
constexpr uint8_t YELLOW_LED_PIN = 26;
constexpr uint8_t RED_LED_PIN = 25;
constexpr uint8_t BUZZER_CHANNEL = 0;

void initAlerts()
{
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);

    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);

    ledcSetup(BUZZER_CHANNEL, 2000, 8);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWrite(BUZZER_CHANNEL, 0);

    Serial.println("Alert outputs initialized.");
}

bool detectImpact(const SensorData& data, bool armed) {
    if (!armed) {
        return false;
    }

    return data.motionValid && data.accelerationG >= IMPACT_THRESHOLD_G;
}

void activateAlert()
{
    digitalWrite(RED_LED_PIN, HIGH);
    ledcWriteTone(BUZZER_CHANNEL, 2000);
}

void clearAlert()
{
    digitalWrite(RED_LED_PIN, LOW);
    ledcWriteTone(BUZZER_CHANNEL, 0);
}
