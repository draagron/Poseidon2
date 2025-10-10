/**
 * @file ESP32OneWireSensors.cpp
 * @brief Implementation of ESP32 1-wire sensor HAL adapter
 *
 * @see ESP32OneWireSensors.h for detailed documentation
 */

#include "ESP32OneWireSensors.h"

// Device family codes for 1-wire devices
#define DS2438_FAMILY 0x26  // Smart Battery Monitor
#define DS18B20_FAMILY 0x28  // Temperature sensor (if needed)

ESP32OneWireSensors::ESP32OneWireSensors(uint8_t pin)
    : busPin(pin),
      initialized(false),
      hasSaildriveSensor(false),
      hasBatteryASensor(false),
      hasBatteryBSensor(false),
      hasShorePowerSensor(false) {
    // Initialize device addresses to zero
    memset(&saildriveAddr, 0, sizeof(DeviceAddress));
    memset(&batteryAAddr, 0, sizeof(DeviceAddress));
    memset(&batteryBAddr, 0, sizeof(DeviceAddress));
    memset(&shorePowerAddr, 0, sizeof(DeviceAddress));

    // Create OneWire bus instance
    oneWire = new OneWire(busPin);
}

ESP32OneWireSensors::~ESP32OneWireSensors() {
    if (oneWire != nullptr) {
        delete oneWire;
        oneWire = nullptr;
    }
}

bool ESP32OneWireSensors::initialize() {
    if (initialized) {
        return true;  // Already initialized
    }

    if (oneWire == nullptr) {
        return false;  // Failed to create OneWire instance
    }

    // Reset the 1-wire bus
    if (!oneWire->reset()) {
        // No devices present on bus
        return false;
    }

    // Enumerate all devices on the bus
    int deviceCount = enumerateDevices();

    // Log device discovery results
    Serial.printf("[1-Wire] Found %d device(s) on GPIO %d\n", deviceCount, busPin);

    if (hasSaildriveSensor) {
        Serial.println("[1-Wire] Saildrive sensor detected");
    }
    if (hasBatteryASensor) {
        Serial.println("[1-Wire] Battery A monitor detected");
    }
    if (hasBatteryBSensor) {
        Serial.println("[1-Wire] Battery B monitor detected");
    }
    if (hasShorePowerSensor) {
        Serial.println("[1-Wire] Shore power sensor detected");
    }

    initialized = true;
    return true;
}

int ESP32OneWireSensors::enumerateDevices() {
    uint8_t addr[8];
    int deviceCount = 0;

    // Reset search
    oneWire->reset_search();

    // Search for all devices on the bus
    while (oneWire->search(addr)) {
        // Validate CRC of device address
        if (!OneWire::crc8(addr, 7) == addr[7]) {
            Serial.println("[1-Wire] CRC mismatch on device address");
            continue;
        }

        deviceCount++;

        // Identify device by family code
        uint8_t familyCode = addr[0];

        // For now, assign devices in discovery order
        // TODO: Update with actual device addresses from hardware installation
        if (familyCode == DS2438_FAMILY) {
            if (!hasSaildriveSensor) {
                copyAddress(saildriveAddr, addr);
                hasSaildriveSensor = true;
            } else if (!hasBatteryASensor) {
                copyAddress(batteryAAddr, addr);
                hasBatteryASensor = true;
            } else if (!hasBatteryBSensor) {
                copyAddress(batteryBAddr, addr);
                hasBatteryBSensor = true;
            } else if (!hasShorePowerSensor) {
                copyAddress(shorePowerAddr, addr);
                hasShorePowerSensor = true;
            }
        }
    }

    return deviceCount;
}

bool ESP32OneWireSensors::readSaildriveStatus(SaildriveData& data) {
    if (!initialized || !hasSaildriveSensor) {
        data.available = false;
        return false;
    }

    bool state = false;
    if (readDigitalSensor(saildriveAddr, state)) {
        data.saildriveEngaged = state;
        data.available = true;
        data.lastUpdate = millis();
        return true;
    }

    data.available = false;
    return false;
}

bool ESP32OneWireSensors::readBatteryA(BatteryMonitorData& data) {
    if (!initialized || !hasBatteryASensor) {
        data.available = false;
        return false;
    }

    return readBatteryMonitor(batteryAAddr, data);
}

bool ESP32OneWireSensors::readBatteryB(BatteryMonitorData& data) {
    if (!initialized || !hasBatteryBSensor) {
        data.available = false;
        return false;
    }

    return readBatteryMonitor(batteryBAddr, data);
}

bool ESP32OneWireSensors::readShorePower(ShorePowerData& data) {
    if (!initialized || !hasShorePowerSensor) {
        data.available = false;
        return false;
    }

    bool connected = false;
    double power = 0.0;

    // Read connection status (digital)
    if (!readDigitalSensor(shorePowerAddr, connected)) {
        data.available = false;
        return false;
    }

    // Read power consumption (analog) - only if connected
    if (connected) {
        if (!readPowerConsumption(shorePowerAddr, power)) {
            data.available = false;
            return false;
        }
    }

    data.shorePowerOn = connected;
    data.power = power;
    data.available = true;
    data.lastUpdate = millis();

    return true;
}

bool ESP32OneWireSensors::isBusHealthy() {
    if (!initialized || oneWire == nullptr) {
        return false;
    }

    // Quick bus health check - try to reset
    return oneWire->reset();
}

// Private helper methods

bool ESP32OneWireSensors::readDigitalSensor(const DeviceAddress& addr, bool& state) {
    if (!isValidAddress(addr)) {
        return false;
    }

    // Reset the bus
    if (!oneWire->reset()) {
        return false;
    }

    // Select the device
    oneWire->select(addr.addr);

    // TODO: Implement actual DS2438 digital read commands
    // For now, return a placeholder value
    // This will be updated during hardware integration phase
    state = false;  // Placeholder

    return true;  // Stub implementation
}

bool ESP32OneWireSensors::readBatteryMonitor(const DeviceAddress& addr, BatteryMonitorData& data) {
    if (!isValidAddress(addr)) {
        return false;
    }

    // Reset the bus
    if (!oneWire->reset()) {
        return false;
    }

    // Select the device
    oneWire->select(addr.addr);

    // TODO: Implement actual DS2438 battery monitor read commands
    // DS2438 provides: voltage (VAD), current (ISENSE), temperature
    // For now, return placeholder values
    // This will be updated during hardware integration phase

    data.voltage = 12.6;  // Placeholder
    data.amperage = 0.0;  // Placeholder
    data.stateOfCharge = 85.0;  // Placeholder
    data.shoreChargerOn = false;  // Placeholder
    data.engineChargerOn = false;  // Placeholder
    data.available = true;

    return true;  // Stub implementation
}

bool ESP32OneWireSensors::readPowerConsumption(const DeviceAddress& addr, double& power) {
    if (!isValidAddress(addr)) {
        return false;
    }

    // TODO: Implement actual power consumption read
    // This will be updated during hardware integration phase
    power = 0.0;  // Placeholder

    return true;  // Stub implementation
}

bool ESP32OneWireSensors::validateCRC(const uint8_t* data, uint8_t len) {
    if (len < 2) {
        return false;
    }

    uint8_t crc = OneWire::crc8(data, len - 1);
    return (crc == data[len - 1]);
}

void ESP32OneWireSensors::copyAddress(DeviceAddress& dest, const uint8_t* src) {
    memcpy(dest.addr, src, 8);
}

bool ESP32OneWireSensors::isValidAddress(const DeviceAddress& addr) {
    // Check if address is all zeros (uninitialized)
    for (int i = 0; i < 8; i++) {
        if (addr.addr[i] != 0) {
            return true;
        }
    }
    return false;
}
