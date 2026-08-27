#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <math.h>

#include "cloud.h"
#include "secrets.h"

namespace {
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

String telemetryUrl() {
    return String("https://") + AZURE_IOT_HUB_NAME +
           ".azure-devices.net/devices/" + AZURE_DEVICE_ID +
           "/messages/events?api-version=2021-04-12";
}

void appendFloatOrNull(String& payload, const char* key, float value, bool valid, int decimals) {
    payload += '"';
    payload += key;
    payload += "\":";

    if (valid && !isnan(value)) {
        payload += String(value, decimals);
    } else {
        payload += "null";
    }
}

String buildPayload(const SensorData& data, int healthScore, bool armed, bool alert) {
    String payload;
    payload.reserve(220);
    payload += '{';

    appendFloatOrNull(payload, "temperature", data.temperatureC, data.environmentValid, 1);
    payload += ',';
    appendFloatOrNull(payload, "humidity", data.humidity, data.environmentValid, 1);
    payload += ',';
    appendFloatOrNull(payload, "acceleration", data.accelerationG, data.motionValid, 3);

    payload += ",\"healthScore\":";
    if (healthScore >= 0) {
        payload += String(healthScore);
    } else {
        payload += "null";
    }

    payload += ",\"armed\":";
    payload += armed ? "true" : "false";
    payload += ",\"alert\":";
    payload += alert ? "true" : "false";
    payload += ",\"uptimeMs\":";
    payload += String(millis());
    payload += '}';

    return payload;
}

bool connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
        delay(250);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi connection failed/timed out.");
        return false;
    }

    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
    return true;
}
}  // namespace

void initCloud() {
    if (connectWiFi()) {
        Serial.print("Azure IoT Hub endpoint ready: ");
        Serial.println(telemetryUrl());
    } else {
        Serial.println("Azure upload will retry when telemetry is sent.");
    }
}

bool cloudConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool uploadSensorData(const SensorData& data, int healthScore, bool armed, bool alert) {
    String payload = buildPayload(data, healthScore, armed, alert);

    Serial.println("--- Azure Telemetry Payload ---");
    Serial.println(payload);

    if (!connectWiFi()) {
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = telemetryUrl();

    if (!https.begin(client, url)) {
        Serial.println("Failed to initialize HTTPS connection.");
        return false;
    }

    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", AZURE_SAS_TOKEN);

    int httpCode = https.POST(payload);
    https.end();

    if (httpCode == 204) {
        Serial.println("Azure telemetry sent successfully (HTTP 204).");
        return true;
    }

    Serial.print("Azure telemetry upload failed. HTTP code: ");
    Serial.println(httpCode);

    if (httpCode == 401) {
        Serial.println("Check the device ID and whether the SAS token is expired or belongs to the wrong device.");
    }

    return false;
}
