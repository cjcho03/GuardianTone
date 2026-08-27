#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "sensors.h"

namespace {
constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;
constexpr uint32_t I2C_FREQUENCY_HZ = 100000;

constexpr uint8_t DHT20_ADDRESS = 0x38;
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
bool dht20Available = false;
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

bool readRegister(uint8_t address, uint8_t reg, uint8_t& value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (Wire.requestFrom(static_cast<int>(address), 1) != 1) {
        return false;
    }

    value = Wire.read();
    return true;
}

void scanI2CBus() {
    Serial.println("Scanning I2C bus...");
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

bool initDHT20() {
    if (!i2cDevicePresent(DHT20_ADDRESS)) {
        Serial.println("DHT20 not detected at 0x38.");
        return false;
    }

    // Soft reset. The DHT20 normally comes up calibrated; the reset also
    // gives it a clean state after reconnecting power/wiring.
    Wire.beginTransmission(DHT20_ADDRESS);
    Wire.write(0xBA);
    if (Wire.endTransmission() != 0) {
        Serial.println("DHT20 reset command failed.");
        return false;
    }
    delay(25);

    Serial.println("DHT20 detected at 0x38.");
    return true;
}

bool readDHT20(float& temperatureC, float& humidity) {
    if (!dht20Available) return false;

    // Trigger measurement: 0xAC 0x33 0x00.
    Wire.beginTransmission(DHT20_ADDRESS);
    Wire.write(0xAC);
    Wire.write(0x33);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) {
        return false;
    }

    delay(85);

    uint8_t bytes[7] = {0};
    int received = Wire.requestFrom(static_cast<int>(DHT20_ADDRESS), 7);
    if (received != 7) {
        return false;
    }

    for (int i = 0; i < 7; ++i) {
        bytes[i] = Wire.read();
    }

    // Bit 7 means the sensor is still busy.
    if ((bytes[0] & 0x80) != 0) {
        return false;
    }

    uint32_t rawHumidity =
        (static_cast<uint32_t>(bytes[1]) << 12) |
        (static_cast<uint32_t>(bytes[2]) << 4) |
        (static_cast<uint32_t>(bytes[3]) >> 4);

    uint32_t rawTemperature =
        (static_cast<uint32_t>(bytes[3] & 0x0F) << 16) |
        (static_cast<uint32_t>(bytes[4]) << 8) |
        static_cast<uint32_t>(bytes[5]);

    humidity = (static_cast<float>(rawHumidity) * 100.0f) / 1048576.0f;
    temperatureC = (static_cast<float>(rawTemperature) * 200.0f) / 1048576.0f - 50.0f;

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

            // Gyroscope: 104 Hz, 250 dps. Not used yet, but enabled for
            // future motion analysis.
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
    if (!lsm6dsoAvailable) return false;

    Wire.beginTransmission(lsm6dsoAddress);
    Wire.write(LSM6DSO_OUTX_L_A);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (Wire.requestFrom(static_cast<int>(lsm6dsoAddress), 6) != 6) {
        return false;
    }

    uint8_t raw[6];
    for (int i = 0; i < 6; ++i) {
        raw[i] = Wire.read();
    }

    int16_t xRaw = static_cast<int16_t>((raw[1] << 8) | raw[0]);
    int16_t yRaw = static_cast<int16_t>((raw[3] << 8) | raw[2]);
    int16_t zRaw = static_cast<int16_t>((raw[5] << 8) | raw[4]);

    float xG = xRaw * LSM6DSO_G_PER_LSB;
    float yG = yRaw * LSM6DSO_G_PER_LSB;
    float zG = zRaw * LSM6DSO_G_PER_LSB;

    accelerationG = sqrtf(xG * xG + yG * yG + zG * zG);
    return true;
}
}  // namespace

void initSensors() {
    Wire.begin(SDA_PIN, SCL_PIN, I2C_FREQUENCY_HZ);
    delay(50);

    Serial.print("I2C initialized (SDA=");
    Serial.print(SDA_PIN);
    Serial.print(", SCL=");
    Serial.print(SCL_PIN);
    Serial.println(").");

    scanI2CBus();

    dht20Available = initDHT20();
    lsm6dsoAvailable = initLSM6DSO();

    if (!dht20Available || !lsm6dsoAvailable) {
        Serial.println("One or more sensors are unavailable; missing values will be uploaded as null.");
    }
}

SensorData readSensors() {
    SensorData data = {NAN, NAN, NAN, false, false};

    data.environmentValid = readDHT20(data.temperatureC, data.humidity);
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
        Serial.println("Temperature/Humidity: unavailable");
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
