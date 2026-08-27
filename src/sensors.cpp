#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <math.h>

#include "sensors.h"

namespace {
constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;
constexpr uint32_t I2C_FREQUENCY_HZ = 50000;

constexpr uint8_t DHT_PIN = 13;
constexpr uint8_t DHT_TYPE = DHT11;
DHT dht(DHT_PIN, DHT_TYPE);

constexpr uint8_t LSM6DSO_ADDRESS_LOW = 0x6A;
constexpr uint8_t LSM6DSO_ADDRESS_HIGH = 0x6B;

constexpr uint8_t LSM6DSO_WHO_AM_I = 0x0F;
constexpr uint8_t LSM6DSO_CTRL1_XL = 0x10;
constexpr uint8_t LSM6DSO_CTRL2_G = 0x11;
constexpr uint8_t LSM6DSO_OUTX_L_A = 0x28;
constexpr uint8_t LSM6DSO_EXPECTED_ID = 0x6C;

// +/- 2 g sensitivity from the LSM6DSO datasheet: 0.061 mg/LSB.
constexpr float LSM6DSO_G_PER_LSB = 0.000061f;

uint8_t lsm6dsoAddress = 0;
bool lsm6dsoAvailable = false;

bool i2cDevicePresent(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool readRegister(
    uint8_t address,
    uint8_t reg,
    uint8_t& value
) {
    Wire.beginTransmission(address);
    Wire.write(reg);

    if (Wire.endTransmission(true) != 0) {
        return false;
    }

    delayMicroseconds(100);

    if (Wire.requestFrom(
            static_cast<int>(address),
            1
        ) != 1) {
        return false;
    }

    value = Wire.read();
    return true;
}

void scanI2CBus() {
    Serial.println("Scanning I2C bus (LSM6DSO only; DHT11 will not appear here)...");
    int count = 0;

    for (uint8_t address = 1; address < 127; ++address) {
        if (i2cDevicePresent(address)) {
            Serial.print("  Found device at 0x");
            if (address < 0x10) Serial.print('0');
            Serial.println(address, HEX);
            ++count;
        }
    }

    if (count == 0) {
        Serial.println("  No I2C devices detected.");
    }
}

bool readDHT11(float& temperatureC, float& humidity) {
    humidity = dht.readHumidity();
    temperatureC = dht.readTemperature();

    if (isnan(humidity) || isnan(temperatureC)) {
        return false;
    }

    return true;
}

bool initLSM6DSO() {
    uint8_t candidateAddresses[] = {LSM6DSO_ADDRESS_LOW, LSM6DSO_ADDRESS_HIGH};

    for (uint8_t address : candidateAddresses) {
        uint8_t whoAmI = 0;
        if (!readRegister(address, LSM6DSO_WHO_AM_I, whoAmI)) {
            continue;
        }

        Serial.print("LSM6DSO candidate at 0x");
        Serial.print(address, HEX);
        Serial.print(" WHO_AM_I=0x");
        Serial.println(whoAmI, HEX);

        if (whoAmI == LSM6DSO_EXPECTED_ID) {
            lsm6dsoAddress = address;

            // Accelerometer: 104 Hz output data rate, +/- 2 g full scale.
            if (!writeRegister(lsm6dsoAddress, LSM6DSO_CTRL1_XL, 0x40)) {
                return false;
            }

            writeRegister(lsm6dsoAddress, LSM6DSO_CTRL2_G, 0x40);

            delay(20);
            Serial.println("LSM6DSO initialized.");
            return true;
        }
    }

    Serial.println("LSM6DSO not detected at 0x6A/0x6B.");
    return false;
}

bool readLSM6DSO(float& accelerationG) {
    if (!lsm6dsoAvailable) {
        return false;
    }

    Wire.beginTransmission(lsm6dsoAddress);
    Wire.write(LSM6DSO_OUTX_L_A);

    if (Wire.endTransmission(true) != 0) {
        return false;
    }

    delayMicroseconds(100);

    if (Wire.requestFrom(
            static_cast<int>(lsm6dsoAddress),
            6
        ) != 6) {
        return false;
    }

    uint8_t raw[6];

    for (int i = 0; i < 6; ++i) {
        raw[i] = Wire.read();
    }

    int16_t xRaw =
        static_cast<int16_t>((raw[1] << 8) | raw[0]);

    int16_t yRaw =
        static_cast<int16_t>((raw[3] << 8) | raw[2]);

    int16_t zRaw =
        static_cast<int16_t>((raw[5] << 8) | raw[4]);

    float xG = xRaw * LSM6DSO_G_PER_LSB;
    float yG = yRaw * LSM6DSO_G_PER_LSB;
    float zG = zRaw * LSM6DSO_G_PER_LSB;

    accelerationG =
        sqrtf(xG * xG + yG * yG + zG * zG);

    return true;
}
} // namespace

void initSensors() {
    // DHT11 is a one-wire-style digital sensor, separate from the I2C bus.
    dht.begin();
    Serial.print("DHT11 initialized on GPIO ");
    Serial.println(DHT_PIN);

    // Give the DHT11 time to stabilize before the first read.
    delay(2000);

    // The LSM6DSO remains on I2C.
    Wire.begin(SDA_PIN, SCL_PIN, I2C_FREQUENCY_HZ);
    delay(50);

    Serial.print("I2C initialized (SDA=");
    Serial.print(SDA_PIN);
    Serial.print(", SCL=");
    Serial.print(SCL_PIN);
    Serial.println(").");

    scanI2CBus();
    lsm6dsoAvailable = initLSM6DSO();

    if (!lsm6dsoAvailable) {
        Serial.println("LSM6DSO unavailable; acceleration will be uploaded as null.");
    }
}

SensorData readSensors() {
    SensorData data = {NAN, NAN, NAN, false, false};

    data.environmentValid = readDHT11(data.temperatureC, data.humidity);
    data.motionValid = readLSM6DSO(data.accelerationG);

    Serial.println("--- Sensor Reading ---");
    if (data.environmentValid) {
        Serial.print("Temperature: ");
        Serial.print(data.temperatureC, 1);
        Serial.println(" C");
        Serial.print("Humidity: ");
        Serial.print(data.humidity, 1);
        Serial.println(" %");
    } else {
        Serial.println("DHT11 read failed; temperature/humidity unavailable.");
    }

    if (data.motionValid) {
        Serial.print("Acceleration magnitude: ");
        Serial.print(data.accelerationG, 3);
        Serial.println(" g");
    } else {
        Serial.println("Acceleration: unavailable");
    }

    return data;
}
