# OpenKNX I2C Wrapper - Usage Guide

Practical examples for using the OpenKNX I2C Wrapper with PIO and Hardware I2C.

**Part of OpenKNX Common Library**

---

## Table of Contents

1. [Basic Configuration](#basic-configuration)
2. [Standalone PIO I2C](#standalone-pio-i2c)
3. [OpenKNX Integration](#openknx-integration)
4. [Multiple I2C Buses](#multiple-i2c-buses)
5. [Device-Specific Examples](#device-specific-examples)
6. [Error Handling](#error-handling)
7. [Performance Optimization](#performance-optimization)
8. [Debugging and Testing](#debugging-and-testing)

---

## Basic Configuration

### Hardware Configuration File

Create or update your hardware configuration file (e.g., `PiPico.h`):

```cpp
// PiPico.h or similar hardware configuration
#pragma once

// ============================================================
// PIO I2C Bus 1 (for Display) - Maximum 2 PIO buses total
// ============================================================
#define OPENKNX_WIRE_PIO
#define OPENKNX_WIRE_PIO_SDA_PIN 26
#define OPENKNX_WIRE_PIO_SCL_PIN 27  // Must be SDA + 1!
#define OPENKNX_WIRE_PIO_CLOCK   400000

// ============================================================
// PIO I2C Bus 2 (for Sensors) - Maximum 2 PIO buses total
// ============================================================
#define OPENKNX_WIRE1_PIO
#define OPENKNX_WIRE1_PIO_SDA_PIN 4
#define OPENKNX_WIRE1_PIO_SCL_PIN 5  // Must be SDA + 1!
#define OPENKNX_WIRE1_PIO_CLOCK   100000

// ============================================================
// Hardware I2C Bus 1 (optional)
// ============================================================
#define OPENKNX_WIRE
#define OPENKNX_WIRE_SDA_PIN 20
#define OPENKNX_WIRE_SCL_PIN 21
#define OPENKNX_WIRE_CLOCK   400000

// ============================================================
// Hardware I2C Bus 2 (optional)
// ============================================================
#define OPENKNX_WIRE1
#define OPENKNX_WIRE1_SDA_PIN 6
#define OPENKNX_WIRE1_SCL_PIN 7
#define OPENKNX_WIRE1_CLOCK   100000

// ============================================================
// Optional Features
// ============================================================

// Scan all buses on initialization
#define OPENKNX_I2C_SCAN_ON_INIT

// Enable benchmark suite
// #define OPENKNX_I2C_BENCHMARK
```

### PlatformIO Configuration

```ini
[env:pico]
platform = raspberrypi
board = pico
framework = arduino

lib_deps = 
    OpenKNX-Common

build_flags = 
    -D OPENKNX_I2C_SCAN_ON_INIT
    ; -D OPENKNX_I2C_BENCHMARK
```

---

## Standalone PIO I2C

### Simple Example without OpenKNX Framework

```cpp
#include <Arduino.h>
#include "OpenKNX/I2C/PIOI2CWire.h"

// Create PIO I2C instance
OpenKNX::I2C::PIOI2CWire myI2C(26, 27, 400000);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize I2C
    myI2C.begin();
    
    Serial.println("PIO I2C initialized");
    
    // Scan for devices
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        myI2C.beginTransmission(addr);
        uint8_t error = myI2C.endTransmission();
        
        if (error == 0) {
            Serial.printf("Device found at 0x%02X\n", addr);
        }
    }
}

void loop() {
    // Write to device 0x3C
    myI2C.beginTransmission(0x3C);
    myI2C.write(0x00);  // Command
    myI2C.write(0xAF);  // Data
    uint8_t error = myI2C.endTransmission();
    
    if (error == 0) {
        Serial.println("Write successful");
    } else {
        Serial.println("Write failed");
    }
    
    delay(1000);
}
```

### Conditional Compilation

```cpp
#include <Arduino.h>

// Conditional include
#ifdef USE_PIO_I2C
    #include "OpenKNX/I2C/PIOI2CWire.h"
    OpenKNX::I2C::PIOI2CWire myI2C(26, 27, 400000);
    #define MY_I2C myI2C
#else
    #define MY_I2C Wire1
#endif

void setup() {
    Serial.begin(115200);
    
    #ifndef USE_PIO_I2C
        Wire1.setSDA(26);
        Wire1.setSCL(27);
    #endif
    
    MY_I2C.begin();
    MY_I2C.setClock(400000);
    
    Serial.println("I2C ready");
}

void loop() {
    MY_I2C.beginTransmission(0x3C);
    MY_I2C.write(0x00);
    MY_I2C.endTransmission();
    
    delay(100);
}
```

---

## OpenKNX Integration

### Using in OpenKNX Projects

```cpp
#include "OpenKNX.h"

void setup() {
    // OpenKNX automatically initializes all I2C buses
    openknx.init();
    
    // I2C buses are now available:
    // - WIRE_PIO (if defined)
    // - WIRE1_PIO (if defined)
    // - WIRE (if defined)
    // - WIRE1 (if defined)
}

void loop() {
    openknx.loop();
}
```

### Using in OpenKNX Modules

```cpp
// In an OpenKNX module
#include "OpenKNX.h"

class MyModule : public OpenKNX::Module {
public:
    void setup() override {
        // Use WIRE_PIO for display
        WIRE_PIO.beginTransmission(0x3C);
        WIRE_PIO.write(displayData);
        WIRE_PIO.endTransmission();
    }
    
    void loop() override {
        // Use WIRE1_PIO for sensors
        WIRE1_PIO.beginTransmission(0x48);
        WIRE1_PIO.write(sensorCommand);
        WIRE1_PIO.endTransmission();
    }
};
```

### Hardware-Specific Configuration

In hardware config (e.g., `PiPico.h`):

```cpp
// Device Display I2C
#ifdef OKNXHW_REG2_DEVICE_DISPLAY
    #define OKNXHW_DEVICE_DISPLAY_I2C_SDA 26
    #define OKNXHW_DEVICE_DISPLAY_I2C_SCL 27
    #define OKNXHW_DEVICE_DISPLAY_I2C_CLOCK 400000
    
    // Use PIO I2C for display
    #define OPENKNX_WIRE_PIO
    #define OPENKNX_WIRE_PIO_SDA_PIN OKNXHW_DEVICE_DISPLAY_I2C_SDA
    #define OPENKNX_WIRE_PIO_SCL_PIN OKNXHW_DEVICE_DISPLAY_I2C_SCL
    #define OPENKNX_WIRE_PIO_CLOCK   OKNXHW_DEVICE_DISPLAY_I2C_CLOCK
    
    // Alias for display
    #define OKNXHW_REG2_HWDISPLAY_I2C_INST WIRE_PIO
#endif

// GPIO Expander I2C
#ifdef OKNXHW_REG2_GPIO_EXPANDER
    #define OPENKNX_WIRE1_PIO
    #define OPENKNX_WIRE1_PIO_SDA_PIN 4
    #define OPENKNX_WIRE1_PIO_SCL_PIN 5
    #define OPENKNX_WIRE1_PIO_CLOCK   100000
    
    #define OPENKNX_GPIO_WIRE WIRE1_PIO
#endif
```

---

## Multiple I2C Buses

### Four-Bus Configuration (Maximum)

```cpp
// hardware.h
// Maximum: 2 PIO + 2 Hardware = 4 buses total

#define OPENKNX_WIRE_PIO        // PIO Bus 1
#define OPENKNX_WIRE_PIO_SDA_PIN 26
#define OPENKNX_WIRE_PIO_SCL_PIN 27
#define OPENKNX_WIRE_PIO_CLOCK   400000

#define OPENKNX_WIRE1_PIO       // PIO Bus 2
#define OPENKNX_WIRE1_PIO_SDA_PIN 4
#define OPENKNX_WIRE1_PIO_SCL_PIN 5
#define OPENKNX_WIRE1_PIO_CLOCK   100000

#define OPENKNX_WIRE            // Hardware Bus 1
#define OPENKNX_WIRE_SDA_PIN 20
#define OPENKNX_WIRE_SCL_PIN 21
#define OPENKNX_WIRE_CLOCK   400000

#define OPENKNX_WIRE1           // Hardware Bus 2
#define OPENKNX_WIRE1_SDA_PIN 6
#define OPENKNX_WIRE1_SCL_PIN 7
#define OPENKNX_WIRE1_CLOCK   100000
```

### Using Different Buses

```cpp
void setup() {
    openknx.init();
    
    // Display on WIRE_PIO (400 kHz)
    WIRE_PIO.beginTransmission(0x3C);
    WIRE_PIO.write(displayData);
    WIRE_PIO.endTransmission();
    
    // GPIO Expander on WIRE1_PIO (100 kHz)
    WIRE1_PIO.beginTransmission(0x18);
    WIRE1_PIO.write(gpioData);
    WIRE1_PIO.endTransmission();
    
    // Sensor on WIRE (Hardware I2C)
    WIRE.beginTransmission(0x48);
    WIRE.write(sensorCmd);
    WIRE.endTransmission();
    
    // RTC on WIRE1 (Hardware I2C)
    WIRE1.beginTransmission(0x68);
    WIRE1.write(rtcCmd);
    WIRE1.endTransmission();
}
```

### Runtime Bus Selection

```cpp
TwoWire* selectBus(const char* deviceType) {
    if (strcmp(deviceType, "display") == 0) {
        return &WIRE_PIO;
    } else if (strcmp(deviceType, "sensor") == 0) {
        return &WIRE1_PIO;
    } else if (strcmp(deviceType, "rtc") == 0) {
        return &WIRE;
    }
    return nullptr;
}

void writeToDevice(const char* deviceType, uint8_t addr, uint8_t data) {
    TwoWire* bus = selectBus(deviceType);
    if (bus) {
        bus->beginTransmission(addr);
        bus->write(data);
        bus->endTransmission();
    }
}
```

---

## Device-Specific Examples

### SSD1306 OLED Display

```cpp
void initDisplay() {
    WIRE_PIO.setClock(400000);
    
    // Display OFF
    sendCommand(0xAE);
    
    // Clock divide ratio
    sendCommand(0xD5);
    sendCommand(0x80);
    
    // Multiplex ratio
    sendCommand(0xA8);
    sendCommand(0x3F);
    
    // Display offset
    sendCommand(0xD3);
    sendCommand(0x00);
    
    // Display ON
    sendCommand(0xAF);
}

void sendCommand(uint8_t cmd) {
    WIRE_PIO.beginTransmission(0x3C);
    WIRE_PIO.write(0x00);  // Control byte: Command
    WIRE_PIO.write(cmd);
    WIRE_PIO.endTransmission();
}

void sendData(uint8_t* data, size_t len) {
    WIRE_PIO.beginTransmission(0x3C);
    WIRE_PIO.write(0x40);  // Control byte: Data
    WIRE_PIO.write(data, len);
    WIRE_PIO.endTransmission();
}

void updateDisplay(uint8_t* frameBuffer) {
    // 128x64 Display = 1024 bytes
    const size_t chunkSize = 32;
    
    for (size_t i = 0; i < 1024; i += chunkSize) {
        sendData(frameBuffer + i, chunkSize);
    }
}
```

### PCA9557 GPIO Expander

```cpp
class PCA9557 {
private:
    TwoWire* _wire;
    uint8_t _addr;
    
    enum Register {
        INPUT_PORT = 0x00,
        OUTPUT_PORT = 0x01,
        POLARITY_INV = 0x02,
        CONFIG = 0x03
    };
    
public:
    PCA9557(TwoWire* wire, uint8_t addr = 0x18) 
        : _wire(wire), _addr(addr) {}
    
    void begin() {
        // Configure all pins as output
        writeRegister(CONFIG, 0x00);
    }
    
    void writeOutputs(uint8_t value) {
        writeRegister(OUTPUT_PORT, value);
    }
    
    uint8_t readInputs() {
        return readRegister(INPUT_PORT);
    }
    
private:
    void writeRegister(uint8_t reg, uint8_t value) {
        _wire->beginTransmission(_addr);
        _wire->write(reg);
        _wire->write(value);
        _wire->endTransmission();
    }
    
    uint8_t readRegister(uint8_t reg) {
        // Write register address (without STOP)
        _wire->beginTransmission(_addr);
        _wire->write(reg);
        _wire->endTransmission(false);  // Repeated Start!
        
        // Read value
        _wire->requestFrom(_addr, (size_t)1);
        return _wire->read();
    }
};

// Usage:
PCA9557 gpioExpander(&WIRE1_PIO);

void setup() {
    openknx.init();
    gpioExpander.begin();
    gpioExpander.writeOutputs(0xFF);  // All High
}

void loop() {
    uint8_t inputs = gpioExpander.readInputs();
    Serial.printf("Inputs: 0x%02X\n", inputs);
    delay(100);
}
```

### DS3231 RTC

```cpp
class DS3231 {
private:
    TwoWire* _wire;
    uint8_t _addr = 0x68;
    
public:
    DS3231(TwoWire* wire) : _wire(wire) {}
    
    void setTime(uint8_t hour, uint8_t minute, uint8_t second) {
        _wire->beginTransmission(_addr);
        _wire->write(0x00);  // Start at seconds register
        _wire->write(decToBcd(second));
        _wire->write(decToBcd(minute));
        _wire->write(decToBcd(hour));
        _wire->endTransmission();
    }
    
    void getTime(uint8_t& hour, uint8_t& minute, uint8_t& second) {
        // Set register pointer
        _wire->beginTransmission(_addr);
        _wire->write(0x00);
        _wire->endTransmission(false);  // Repeated Start
        
        // Read 3 bytes
        _wire->requestFrom(_addr, (size_t)3);
        second = bcdToDec(_wire->read());
        minute = bcdToDec(_wire->read());
        hour = bcdToDec(_wire->read());
    }
    
    float getTemperature() {
        // Set register pointer
        _wire->beginTransmission(_addr);
        _wire->write(0x11);  // Temperature register
        _wire->endTransmission(false);
        
        // Read 2 bytes
        _wire->requestFrom(_addr, (size_t)2);
        uint8_t msb = _wire->read();
        uint8_t lsb = _wire->read();
        
        return (float)msb + (float)(lsb >> 6) * 0.25f;
    }
    
private:
    uint8_t decToBcd(uint8_t val) {
        return ((val / 10) << 4) | (val % 10);
    }
    
    uint8_t bcdToDec(uint8_t val) {
        return ((val >> 4) * 10) + (val & 0x0F);
    }
};

// Usage:
DS3231 rtc(&WIRE);

void setup() {
    openknx.init();
    rtc.setTime(14, 30, 0);  // 14:30:00
}

void loop() {
    uint8_t h, m, s;
    rtc.getTime(h, m, s);
    
    float temp = rtc.getTemperature();
    
    Serial.printf("Time: %02d:%02d:%02d, Temp: %.2f°C\n", h, m, s, temp);
    delay(1000);
}
```

### BME280 Environmental Sensor

```cpp
class BME280 {
private:
    TwoWire* _wire;
    uint8_t _addr;
    
public:
    BME280(TwoWire* wire, uint8_t addr = 0x76) 
        : _wire(wire), _addr(addr) {}
    
    bool begin() {
        // Check chip ID
        uint8_t id = readRegister(0xD0);
        if (id != 0x60) {
            return false;
        }
        
        // Soft reset
        writeRegister(0xE0, 0xB6);
        delay(10);
        
        // Configure: Normal mode, oversampling x1
        writeRegister(0xF2, 0x01);  // Humidity
        writeRegister(0xF4, 0x27);  // Temp & Pressure
        
        return true;
    }
    
    float readTemperature() {
        // Read raw temperature (0xFA-0xFC)
        _wire->beginTransmission(_addr);
        _wire->write(0xFA);
        _wire->endTransmission(false);
        
        _wire->requestFrom(_addr, (size_t)3);
        uint32_t raw = 0;
        raw |= (uint32_t)_wire->read() << 12;
        raw |= (uint32_t)_wire->read() << 4;
        raw |= (uint32_t)_wire->read() >> 4;
        
        // Apply compensation (simplified)
        return (float)raw / 5120.0f - 40.0f;
    }
    
private:
    uint8_t readRegister(uint8_t reg) {
        _wire->beginTransmission(_addr);
        _wire->write(reg);
        _wire->endTransmission(false);
        
        _wire->requestFrom(_addr, (size_t)1);
        return _wire->read();
    }
    
    void writeRegister(uint8_t reg, uint8_t value) {
        _wire->beginTransmission(_addr);
        _wire->write(reg);
        _wire->write(value);
        _wire->endTransmission();
    }
};

// Usage:
BME280 sensor(&WIRE1_PIO);

void setup() {
    openknx.init();
    
    if (sensor.begin()) {
        Serial.println("BME280 initialized");
    } else {
        Serial.println("BME280 not found!");
    }
}

void loop() {
    float temp = sensor.readTemperature();
    Serial.printf("Temperature: %.2f°C\n", temp);
    delay(1000);
}
```

---

## Error Handling

### Basic Error Checking

```cpp
void writeToDevice(uint8_t addr, uint8_t data) {
    WIRE_PIO.beginTransmission(addr);
    WIRE_PIO.write(data);
    uint8_t error = WIRE_PIO.endTransmission();
    
    switch(error) {
        case 0:
            Serial.println("Success");
            break;
        case 1:
            Serial.println("Data too long");
            break;
        case 2:
            Serial.println("NACK on address");
            break;
        case 3:
            Serial.println("NACK on data");
            break;
        case 4:
            Serial.println("Other error");
            break;
        default:
            Serial.println("Unknown error");
    }
}
```

### Retry Mechanism

```cpp
bool writeWithRetry(uint8_t addr, uint8_t* data, size_t len, uint8_t retries = 3) {
    for (uint8_t i = 0; i < retries; i++) {
        WIRE_PIO.beginTransmission(addr);
        WIRE_PIO.write(data, len);
        uint8_t error = WIRE_PIO.endTransmission();
        
        if (error == 0) {
            return true;  // Success
        }
        
        Serial.printf("Attempt %d failed, retrying...\n", i + 1);
        delay(10);
    }
    
    Serial.println("All retries failed");
    return false;
}
```

### Bus Reset on Error

```cpp
void resetBusOnError() {
    WIRE_PIO.end();
    delay(100);
    WIRE_PIO.begin();
    WIRE_PIO.setClock(400000);
    Serial.println("Bus reset performed");
}

void robustWrite(uint8_t addr, uint8_t data) {
    WIRE_PIO.beginTransmission(addr);
    WIRE_PIO.write(data);
    uint8_t error = WIRE_PIO.endTransmission();
    
    if (error != 0) {
        Serial.println("Error detected, resetting bus");
        resetBusOnError();
        
        // Retry after reset
        WIRE_PIO.beginTransmission(addr);
        WIRE_PIO.write(data);
        error = WIRE_PIO.endTransmission();
        
        if (error == 0) {
            Serial.println("Success after reset");
        } else {
            Serial.println("Failed even after reset");
        }
    }
}
```

---

## Performance Optimization

### Speed Optimization

```cpp
void optimizeForDisplay() {
    // Most displays support 400 kHz
    WIRE_PIO.setClock(400000);
}

void optimizeForSensors() {
    // Sensors often more conservative
    WIRE1_PIO.setClock(100000);
}

void adaptiveSpeed() {
    // Start slow for stability
    WIRE_PIO.setClock(100000);
    
    // Test higher speed
    WIRE_PIO.setClock(400000);
    
    uint8_t errors = 0;
    for (int i = 0; i < 10; i++) {
        WIRE_PIO.beginTransmission(0x3C);
        WIRE_PIO.write(0x00);
        if (WIRE_PIO.endTransmission() != 0) {
            errors++;
        }
    }
    
    if (errors > 0) {
        Serial.println("High speed unstable, reverting to 100kHz");
        WIRE_PIO.setClock(100000);
    } else {
        Serial.println("400kHz stable!");
    }
}
```

### Batch Operations

```cpp
void updateDisplayFast(uint8_t* buffer, size_t size) {
    const size_t chunkSize = 32;  // Optimal chunk size
    
    for (size_t i = 0; i < size; i += chunkSize) {
        size_t remaining = size - i;
        size_t currentChunk = (remaining < chunkSize) ? remaining : chunkSize;
        
        WIRE_PIO.beginTransmission(0x3C);
        WIRE_PIO.write(0x40);  // Data mode
        WIRE_PIO.write(buffer + i, currentChunk);
        WIRE_PIO.endTransmission();
    }
}
```

---

## Debugging and Testing

### Bus Scanner

```cpp
void scanI2CBus(TwoWire& wire, const char* name) {
    Serial.printf("\nScanning %s...\n", name);
    Serial.println("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
    
    uint8_t found = 0;
    
    for (uint8_t addr = 0; addr < 0x80; addr++) {
        if (addr % 16 == 0) {
            Serial.printf("%02X: ", addr);
        }
        
        wire.beginTransmission(addr);
        uint8_t error = wire.endTransmission();
        
        if (error == 0) {
            Serial.printf(" %02X", addr);
            found++;
        } else {
            Serial.print(" --");
        }
        
        if (addr % 16 == 15) {
            Serial.println();
        }
    }
    
    Serial.printf("\nFound %d device(s)\n", found);
}

void setup() {
    Serial.begin(115200);
    openknx.init();
    
    scanI2CBus(WIRE_PIO, "WIRE_PIO");
    scanI2CBus(WIRE1_PIO, "WIRE1_PIO");
}
```

### Run Benchmarks

```cpp
#ifdef OPENKNX_I2C_BENCHMARK
#include "OpenKNX/I2C/Benchmark.h"

void runTests() {
    Serial.begin(115200);
    delay(2000);
    
    // Test all buses
    OpenKNX::I2C::runAllBenchmarks();
    
    // Or test single bus
    OpenKNX::I2C::runBenchmark(WIRE_PIO, "WIRE_PIO", 0x3C);
}
#endif
```

### Detailed Diagnostics

```cpp
void diagnoseI2C() {
    Serial.println("\n=== I2C Diagnostics ===\n");
    
#ifdef OPENKNX_WIRE_PIO
    Serial.println("WIRE_PIO:");
    Serial.printf("  SDA: GPIO %d\n", OPENKNX_WIRE_PIO_SDA_PIN);
    Serial.printf("  SCL: GPIO %d\n", OPENKNX_WIRE_PIO_SCL_PIN);
    Serial.printf("  Clock: %d Hz\n", OPENKNX_WIRE_PIO_CLOCK);
    
    // Test communication
    WIRE_PIO.beginTransmission(0x3C);
    uint8_t err = WIRE_PIO.endTransmission();
    Serial.printf("  Test (0x3C): %s\n", err == 0 ? "OK" : "FAIL");
#endif

#ifdef OPENKNX_WIRE1_PIO
    Serial.println("\nWIRE1_PIO:");
    Serial.printf("  SDA: GPIO %d\n", OPENKNX_WIRE1_PIO_SDA_PIN);
    Serial.printf("  SCL: GPIO %d\n", OPENKNX_WIRE1_PIO_SCL_PIN);
    Serial.printf("  Clock: %d Hz\n", OPENKNX_WIRE1_PIO_CLOCK);
    
    WIRE1_PIO.beginTransmission(0x18);
    uint8_t err = WIRE1_PIO.endTransmission();
    Serial.printf("  Test (0x18): %s\n", err == 0 ? "OK" : "FAIL");
#endif
}
```

---

## Best Practices

### 1. Pin Selection (PIO I2C Only)
```cpp
// ✓ Correct: Adjacent pins
#define SDA_PIN 26
#define SCL_PIN 27  // SDA + 1

// ✗ Wrong: Non-adjacent pins
#define SDA_PIN 26
#define SCL_PIN 28  // Not SDA + 1
```

### 2. Pull-up Resistors
- Always use external 4.7kΩ pull-ups
- Internal pull-ups (~45kΩ) are too weak
- Place resistors close to master device

### 3. Speed Selection
```cpp
// Standard Mode (safe for all devices)
wire.setClock(100000);

// Fast Mode (most modern devices)
wire.setClock(400000);

// Experimental (test first!)
wire.setClock(800000);
```

### 4. Error Handling
```cpp
// Always check return values
uint8_t error = wire.endTransmission();
if (error != 0) {
    // Handle error
}
```

### 5. Repeated Start
```cpp
// For register-based communication
wire.beginTransmission(addr);
wire.write(reg);
wire.endTransmission(false);  // false = NO STOP
wire.requestFrom(addr, len);
```

### 6. Bus Assignment Strategy
```cpp
// Assign critical devices to separate buses
WIRE_PIO   → Display (400 kHz, PIO)
WIRE1_PIO  → GPIO Expander (100 kHz, PIO)
WIRE       → Sensors (400 kHz, Hardware)
WIRE1      → RTC (100 kHz, Hardware)
```

---

For more examples, see the Benchmark suite and hardware configurations in the OpenKNX Common Library.