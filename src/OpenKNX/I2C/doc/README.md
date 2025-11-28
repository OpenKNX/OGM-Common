# OpenKNX I²C Wire Wrapper - PIO I²C Implementation

## Overview

The **OpenKNX  I²C Wire Wrapper** provides a unified  I²C interface for OpenKNX projects on Raspberry Pi Pico (RP2040/RP2350). It seamlessly integrates both **Hardware  I²C** and **PIO-based  I²C**, offering flexible multi-bus configurations with automatic initialization and optional diagnostics.

**This is part of the OpenKNX Common Library** - no external dependencies required.

**Key Innovation:** PIO-based  I²C implementation that serves as a **100% Wire-compatible** alternative, enabling free GPIO selection while maintaining full API compatibility.

Based on the official Raspberry Pi Pico SDK example: [pico-examples/pio/i2c](https://github.com/raspberrypi/pico-examples/tree/master/pio/i2c)

---

## Architecture

```
OpenKNX::I2C::WireWrapper (in OpenKNX Common Library)
├── PIO I²C (WIRE_PIO/WIRE1_PIO)
│   ├── Uses PIO0 exclusively
│   ├── Maximum 2 PIO I²C buses (limited by PIO0 state machines)
│   ├── Free GPIO selection (adjacent pins required)
│   └── 100% Wire API compatible
│
└── Hardware I²C (WIRE/WIRE1)
    ├── Fixed hardware pins
    ├── Maximum 2 Hardware I²C buses
    └── Standard Arduino Wire API

Total: Maximum 4 I²C buses (2 PIO + 2 Hardware)
```

**Global Bus Instances:**
- `WIRE_PIO` - PIO-based I²C bus #1 (if enabled)
- `WIRE1_PIO` - PIO-based I²C bus #2 (if enabled)
- `WIRE` - Hardware I²C bus #1 (alias to Wire, if enabled)
- `WIRE1` - Hardware I²C bus #2 (alias to Wire1, if enabled)

---

## Key Features

## Advantages over Hardware I²C

| **<span style="color:WHITE">Feature</span>** | **<span style="color:WHITE">Hardware I²C</span>** | **<span style="color:WHITE">PIO I²C</span>** |
|---------|--------------|---------|
| Pin selection | Fixed (I2C0/I2C1) | **<span style="color:GREEN">Any adjacent pins</span>** |
| PIO usage | No | Yes (1 State Machine) |
| CPU independent | Partially | **<span style="color:GREEN">Completely (PIO)** |
| Wire compatible | Yes | Yes |
| Repeated Start | Yes | Yes |

### PIO I²C Implementation
- **<span style="color:WHITE">100% Wire API compatible</span>** - Drop-in replacement without code changes
- **<span style="color:WHITE">Free pin selection</span>** - Any adjacent GPIO pair (SCL = SDA + 1)
- **<span style="color:WHITE">Dedicated PIO0</span>** - Uses PIO0 exclusively for OpenKNX Common
- **<span style="color:WHITE">Up to 2 PIO buses</span>** - Limited by available PIO0 state machines
- **<span style="color:WHITE">Hardware independent</span>** - Hardware I²C remains available
- **<span style="color:WHITE">Fast Mode</span>** - 100 kHz Standard / 400 kHz Fast Mode tested
- **<span style="color:WHITE">Repeated Start</span>** - Full I²C protocol support
- **<span style="color:WHITE">CPU independent</span>** - PIO handles timing autonomously
- **<span style="color:WHITE">RP2040 & RP2350</span>** - Both platforms supported

### Unified Wire Wrapper
- **<span style="color:WHITE">Centralized management</span>** - Single initialization for all buses
- **<span style="color:WHITE">Automatic configuration</span>** - Via compile-time defines
- **<span style="color:WHITE">Bus scanning</span>** - Built-in I²C device detection
- **<span style="color:WHITE">Optional benchmarking</span>** - Performance analysis tools
- **<span style="color:WHITE">Error handling</span>** - Robust timeout and recovery mechanisms
- **<span style="color:WHITE">Part of OpenKNX Common</span>** - No external library dependencies

---

## Bus Limitations

| Bus Type | Maximum Count | Instance Names | Notes |
|----------|---------------|----------------|-------|
| **PIO I²C** | 2 | WIRE_PIO, WIRE1_PIO | Automatic PIO/SM assignment |
| **Hardware I²C** | 2 | WIRE, WIRE1 | RP2040/RP2350 hardware |
| **Total** | **4** | - | 2 PIO + 2 Hardware |

**Important:** OpenKNX Common Library uses 2 PIO I²C buses by default to preserve PIO resources for other applications.

**PIO I²C Resource Allocation:**
- **Automatic Assignment (Default):** PIO instance and state machine are automatically allocated by the SDK
- **Deterministic Behavior:** Resources are assigned based on initialization order in OpenKNX Wire Wrapper
- **First-Come-First-Serve:** First PIO I²C bus gets first available PIO/SM, second bus gets next available
- Each PIO I²C bus requires 1 state machine on any available PIO (pio0, pio1, or pio2)

**Hardware Capabilities:**
- **RP2040:** 2 PIO instances (PIO0, PIO1), 8 state machines total (4 per PIO)
- **RP2350:** 3 PIO instances (PIO0, PIO1, PIO2), 12 state machines total (4 per PIO)

**OpenKNX Usage:**
- **Default Configuration:** 2 PIO I²C buses to leave resources available for other PIO applications
- **Theoretical Maximum:** Up to 8 PIO I²C buses possible on RP2040, 12 on RP2350
- **PIO Assignment:** Automatically uses any available PIO instance (not limited to a specific PIO)
- **Initialization Order:** Defines resource allocation (WIRE_PIO → first free, WIRE1_PIO → second free)

**Resource Assignment Example:**
```
Init Order:     Typical Assignment:
WIRE_PIO    →   First available PIO/SM (e.g., PIO0/SM0)
WIRE1_PIO   →   Next available PIO/SM (e.g., PIO0/SM1)
WIRE        →   Hardware I2C0
WIRE1       →   Hardware I2C1
```

**Note:** The exact PIO/SM assignment depends on available resources at initialization time. Use `get_pio()` and `get_sm()` methods to verify actual allocation.

---

## Requirements

- **Hardware:** Raspberry Pi Pico (RP2040) or Pico 2 (RP2350)
- **SDK:** Pico SDK >= 1.5.0 (RP2040) or >= 2.0.0 (RP2350)
- **Framework:** PlatformIO with Arduino-Pico or Pico SDK
- **Library:** OpenKNX Common Library (included)
- **External:** Pull-up resistors on SDA/SCL (typically 4.7kΩ)

---

## Installation

### Part of OpenKNX Common Library

The I²C Wire wrapper is **integrated into OpenKNX Common Library** - no separate installation needed.

```
OpenKNX/
├── OFM-Common/
│   ├── src/
│   │   └── OpenKNX/
│   │       └── I2C/
│   │           ├── Doc/
│   │           |    ├── i2c.pio    # Original I²C PIO
│   │           |    ├── README.md  # Documentation
│   │           |    └── Usage.md   # Usage examples
│   │           ├── Pio/
│   │           |    ├── pio_i2c.h    # Low-level PIO I²C
│   │           |    ├── pio_i2c.cpp  # PIO I²C driver
│   │           |    └── i2c.pio.h    # PIO program definitions
│   │           ├── Test/
│   │           |    └── Benchmark.h  # Optional benchmark/test suite
│   │           ├── WireWrapper.h   # Main wire wrapper interface
│   │           ├── WireWrapper.cpp # Wire Wrapper implementation
│   │           ├── PIOI2CWire.h    # PIO I²C Wire class
│   │           └── PIOI2CWire.cpp  # PIO I²C implementation
│   └── ...
└── ...
```

### Include in Your Project

```cpp
#include "OpenKNX.h"

// Wire Wrapper is automatically available via openknx.i2c
void setup() {
    openknx.init();  // Initializes all I²C buses
}
```

---

## Configuration

### Hardware Configuration

Define I²C buses in your hardware configuration file (e.g., `PiPico.h`):

```cpp
// ============================================================
// PIO I2C Configuration (Maximum 2 buses)
// ============================================================

// PIO I2C Bus 1 (e.g., for Display)
#define OPENKNX_WIRE_PIO
#define OPENKNX_WIRE_PIO_SDA_PIN 26
#define OPENKNX_WIRE_PIO_SCL_PIN 27  // Must be SDA + 1!
#define OPENKNX_WIRE_PIO_CLOCK   400000

// PIO I2C Bus 2 (e.g., for GPIO Expander)
#define OPENKNX_WIRE1_PIO
#define OPENKNX_WIRE1_PIO_SDA_PIN 4
#define OPENKNX_WIRE1_PIO_SCL_PIN 5  // Must be SDA + 1!
#define OPENKNX_WIRE1_PIO_CLOCK   100000

// ============================================================
// Hardware I2C Configuration (Maximum 2 buses)
// ============================================================

// Hardware I2C Bus 1 (e.g., for Sensors)
#define OPENKNX_WIRE
#define OPENKNX_WIRE_SDA_PIN 20
#define OPENKNX_WIRE_SCL_PIN 21
#define OPENKNX_WIRE_CLOCK   400000

// Hardware I2C Bus 2 (e.g., for RTC)
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

### Basic Usage

```cpp
#include "OpenKNX.h"

void setup() {
    // Initialize all configured I2C buses
    openknx.init();
    
    // Use WIRE_PIO like standard Wire
    WIRE_PIO.beginTransmission(0x3C);
    WIRE_PIO.write(0x00);
    WIRE_PIO.write(0xAF);
    WIRE_PIO.endTransmission();
    
    // Use WIRE1_PIO for other devices
    WIRE1_PIO.beginTransmission(0x18);
    WIRE1_PIO.write(data);
    WIRE1_PIO.endTransmission();
}

void loop() {
    openknx.loop();
}
```

---

## Usage Examples

### Basic Communication

```cpp
void setup() {
    openknx.init();
    
    // Write to device
    WIRE_PIO.beginTransmission(0x3C);
    WIRE_PIO.write(0x00);  // Command byte
    WIRE_PIO.write(0xAF);  // Data byte
    uint8_t error = WIRE_PIO.endTransmission();
    
    if (error == 0) {
        Serial.println("Write successful");
    }
}
```

### Read with Repeated Start

```cpp
uint8_t readRegister(uint8_t addr, uint8_t reg) {
    // Write register address
    WIRE_PIO.beginTransmission(addr);
    WIRE_PIO.write(reg);
    WIRE_PIO.endTransmission(false);  // NO STOP - Repeated Start!
    
    // Read value
    WIRE_PIO.requestFrom(addr, (size_t)1);
    return WIRE_PIO.read();
}
```

### Using All 4 Buses

```cpp
void setup() {
    openknx.init();
    
    // PIO I2C Bus 1 - Display
    WIRE_PIO.beginTransmission(0x3C);
    WIRE_PIO.write(displayData);
    WIRE_PIO.endTransmission();
    
    // PIO I2C Bus 2 - GPIO Expander
    WIRE1_PIO.beginTransmission(0x18);
    WIRE1_PIO.write(gpioData);
    WIRE1_PIO.endTransmission();
    
    // Hardware I2C Bus 1 - Sensor
    WIRE.beginTransmission(0x48);
    WIRE.write(sensorData);
    WIRE.endTransmission();
    
    // Hardware I2C Bus 2 - RTC
    WIRE1.beginTransmission(0x68);
    WIRE1.write(rtcData);
    WIRE1.endTransmission();
}
```

### Device Scanning

```cpp
void setup() {
    openknx.init();
    
    // Scan all configured buses
    openknx.i2c.scanAllBuses();
    
    // Or scan individual bus
    openknx.i2c.scanBus(WIRE_PIO, "WIRE_PIO");
}
```

---

## Complete Wire API Reference

### Initialization
```cpp
void begin()                    // Start I2C bus
void end()                      // Stop I2C bus
void setClock(uint32_t hz)      // Set speed (100000 or 400000)
bool setSDA(pin_size_t pin)     // Set SDA pin (before begin)
bool setSCL(pin_size_t pin)     // Set SCL pin (before begin)
```

### Write Operations
```cpp
void beginTransmission(uint8_t addr)       // Start transaction
size_t write(uint8_t data)                 // Write single byte
size_t write(uint8_t* data, size_t len)    // Write array
uint8_t endTransmission(bool stop = true)  // End (0=OK, 4=NAK)
```

### Read Operations
```cpp
size_t requestFrom(uint8_t addr, size_t len, bool stop = true)
int available()                 // Bytes in buffer
int read()                      // Read next byte (-1 if empty)
void flush()                    // Clear buffers
```

### Helper Functions (PIO I²C only)
```cpp
bool ping(uint8_t addr)         // Device detection (read-based)
bool pingw(uint8_t addr)        // Device detection (write-based)
int ReadBlocking(uint8_t addr, uint8_t* data, size_t len)
int WriteBlocking(uint8_t addr, uint8_t* data, size_t len)
```

---

## Pin Requirements

### Critical Constraint for PIO I²C

**SDA and SCL must be adjacent GPIOs!**
- SCL must be exactly SDA + 1
- This is a hardware requirement of the PIO implementation
- Hardware I²C has no such restriction

### Valid Pin Combinations

| SDA  | SCL  | Status | Notes |
|------|------|--------|-------|
| GP0  | GP1  | ✓ OK   | Valid for PIO I²C |
| GP4  | GP5  | ✓ OK   | Valid for PIO I²C |
| GP26 | GP27 | ✓ OK   | Valid for PIO I²C |
| GP0  | GP2  | ✗ FAIL | SCL not SDA + 1 |
| GP5  | GP4  | ✗ FAIL | Wrong order |

### External Pull-ups Required

Both SDA and SCL need external pull-up resistors:
- **Recommended:** 4.7kΩ to 3.3V
- **Range:** 2.2kΩ - 10kΩ depending on bus capacitance
- **Important:** Internal pull-ups are too weak for reliable I²C operation

---

## Benchmarking

Enable comprehensive performance analysis by defining `OPENKNX_I2C_BENCHMARK`.

### Run Benchmarks

```cpp
#include "OpenKNX.h"

void setup() {
    Serial.begin(115200);
    openknx.init();
    
    // Run all benchmarks on all configured buses
    OpenKNX::I2C::runAllBenchmarks();
}
```

---

## Performance Comparison

| Feature | Hardware  I²C | PIO  I²C |
|---------|--------------|---------|
| **Pin Selection** | Fixed (I2C0/I2C1) | Any adjacent pair |
| **Max Speed** | 1 MHz | 400-800 kHz tested |
| **CPU Load** | Low | Very Low (PIO) |
| **Buses Available** | 2 | 2 (*up to 8 or 12)|
| **Wire Compatible** | Yes | Yes (100%) |
| **Repeated Start** | Yes | Yes |
| **Clock Stretching** | Yes | Yes |

\* The number of PIO-based I²C buses is theoretically higher:
RP2040 provides up to 8 state machines (2 PIO blocks × 4 SMs), RP2350 even up to 12. In this project, we deliberately limited it to 2 to conserve resources and reduce complexity.

---

## Bus Configuration Strategies

### Strategy 1: All PIO I²C (2 buses)
```cpp
#define OPENKNX_WIRE_PIO        // Display
#define OPENKNX_WIRE1_PIO       // GPIO Expander
```
**Pros:** Maximum pin flexibility  
**Cons:** Only 2 buses total

### Strategy 2: All Hardware I²C (2 buses)
```cpp
#define OPENKNX_WIRE            // Sensor
#define OPENKNX_WIRE1           // RTC
```
**Pros:** PIO resources available for other uses  
**Cons:** Fixed pin locations

### Strategy 3: Mixed (4 buses) - Recommended
```cpp
#define OPENKNX_WIRE_PIO        // Display (free pins)
#define OPENKNX_WIRE1_PIO       // GPIO Expander (free pins)
#define OPENKNX_WIRE            // Sensor (hardware pins)
#define OPENKNX_WIRE1           // RTC (hardware pins)
```
**Pros:** Maximum flexibility, 4 buses total  
**Cons:** More complex configuration

---

## Tested I²C Devices

Successfully tested with:
- **SSD1306** OLED Display (0x3C) @ 400 kHz
- **PCA9557** I/O Expander (0x18) with Repeated Start
- **DS3231** RTC modules (0x68)
- **24C256** EEPROM
- **BME280** Environmental sensor (0x76/0x77)
- Generic I²C sensors and peripherals

---

## Troubleshooting

### Compilation Error: "SCL pin must be SDA pin + 1"

**Cause:** PIO I²C requires adjacent pins

**Solution:**
```cpp
// Wrong:
#define OPENKNX_WIRE_PIO_SDA_PIN 26
#define OPENKNX_WIRE_PIO_SCL_PIN 28  // ✗ Not adjacent!

// Correct:
#define OPENKNX_WIRE_PIO_SDA_PIN 26
#define OPENKNX_WIRE_PIO_SCL_PIN 27  // ✓ SDA + 1
```

### Device Not Found

**Solutions:**
1. Verify external pull-up resistors (4.7kΩ)
2. Check pin configuration for PIO I²C
3. Test with hardware Wire for comparison
4. Verify device address
5. Check 3.3V power supply

### Unstable Communication

**Solutions:**
1. Reduce clock speed: `setClock(100000)`
2. Shorten I²C traces (max 30cm)
3. Use stronger pull-ups (2.2kΩ instead of 10kΩ)
4. Add 100nF capacitors near devices

---

## Technical Details

### PIO Resource Usage

**The OpenKNX Common Library uses up to 2 PIO state machines for I²C (if both buses are defined):**

- Each PIO I²C bus uses 1 state machine
- State machines are not statically assigned
- The system dynamically selects the next available one at runtime

This ensures flexibility and keeps remaining PIO resources available for other use cases.


#
## Memory Usage Per Bus

| Component      | Size         |
|----------------|--------------|
| TX Buffer      | 256 bytes*   |
| RX Buffer      | 256 bytes*   |
| Instance Data  | ~24 bytes    |
| **Total per bus** | **~536 bytes** |


\* Buffer sizes are optimized for the I²C display use case to reduce CPU load and interrupt frequency. They can be adjusted later if needed — even down to 16 bytes to match the hardware I²C FIFOs. This flexibility allows balancing performance and memory usage depending on the application.

### I²C Protocol Support

- START/STOP conditions
- Repeated START
- ACK/NAK handling
- 7-bit addressing
- Clock stretching
- **<span style="color:RED">Not Supported: 10-bit addressing</span>**
- **<span style="color:RED">Not Supported: Slave mode</span>**
- **<span style="color:RED">Not Supported: Multi-master arbitration</span>**

---

## Migration Guide

### From Hardware Wire to PIO I²C

**Step 1:** Update hardware configuration
```cpp
// Old:
Wire1.setSDA(26);
Wire1.setSCL(27);
Wire1.begin();

// New:
#define OPENKNX_WIRE_PIO
#define OPENKNX_WIRE_PIO_SDA_PIN 26
#define OPENKNX_WIRE_PIO_SCL_PIN 27
#define OPENKNX_WIRE_PIO_CLOCK 400000
// Auto-initialized by openknx.init()
```

**Step 2:** Change bus reference
```cpp
// Old:
Wire1.beginTransmission(0x3C);

// New:
WIRE_PIO.beginTransmission(0x3C);
// API is identical!
```

---

## Best Practices

### 1. Pin Selection (PIO I²C)
```cpp
// ✓ Correct
#define SDA_PIN 26
#define SCL_PIN 27  // SDA + 1

// ✗ Wrong
#define SDA_PIN 26
#define SCL_PIN 28  // Not adjacent
```

### 2. Pull-up Resistors

### PIO I²C Hardware Connection
```
 +-----------------+
 | RP2040 / RP2350 |
 |                 |
 | SDA ---- GPx ---+--+---+----+
 | SCL ---- GPx+1 -+--+---+----+
 +-----------------+  |   |    |
                      |   |    |
                 +----+ +----+ +----+
                 |Dev1| |Dev2| |Dev3|
                 +----+ +----+ +----+
                      |
              +-------+-------+
              | Pull-up 4.7kΩ |
              +-------+-------+
                      |
                     3.3V
```
**Notes:**
- SDA and SCL must be adjacent GPIOs (SCL = SDA + 1).
- External pull-up resistors (typically 4.7kΩ to 3.3V) are **required**.
- Internal pull-ups are too weak for reliable I²C operation.
- One set of pull-ups is enough for the entire bus.


### 3. Speed Selection
```cpp
setClock(100000);  // Conservative
setClock(400000);  // Standard Fast Mode
setClock(800000);  // Experimental
```

### 4. Error Handling
```cpp
uint8_t error = wire.endTransmission();
if (error != 0) {
    // Handle error
}
```

---

## License

Part of OpenKNX Common Library

### BSD-3-Clause License
PIO I²C core based on Raspberry Pi Pico SDK examples.  
Copyright (c) 2020 Raspberry Pi (Trading) Ltd.

### GNU GPL v3.0
OpenKNX Wire Wrapper and PIO to Wire modifications.  
Copyright (c) 2025 Erkan Çolak - OpenKNX

---

## References

- [OpenKNX Project](https://github.com/OpenKNX)
- [OpenKNX Common Library](https://github.com/OpenKNX/OGM-Common)
- [Raspberry Pi Pico SDK Examples](https://github.com/raspberrypi/pico-examples/tree/master/pio/i2c)
- [RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
- [RP2350 Datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)

---