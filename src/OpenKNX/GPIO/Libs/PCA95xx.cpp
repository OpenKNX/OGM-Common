/**
 * OpenKNX I2C I/O Expander Library for NXP PCA95xx series
 *
 * Based on the original SparkFun I2C Expander Arduino Library
 * (https://github.com/sparkfunX/SparkFun_I2C_Expander_Arduino_Library)
 *
 * Supports PCA/TCA series I/O expanders:
 * - PCA9534/TCA9534: 8-bit with INT
 * - PCA9536/TCA9536: 4-bit with optional INT
 * - PCA9537/TCA9537: 4-bit with INT/RST
 * - PCA9554/TCA9554: 8-bit with INT
 * - PCA9556/TCA9556: 8-bit with RST
 * - PCA9557/TCA9557: 8-bit with RST
 *
 * Modified by Erkan Çolar 2024
 * Original work by SparkFun Electronics (2018-2024)
 *
 * License: GNU General Public License
 */

#include "PCA95xx.h"

#include "OpenKNX/I2C/Wire1Lock.h" // shared Wire1 (display-shared bus) mutex; no-op on RP2040

// Constructor
PCA95XX::PCA95XX(TwoWire *wirePort, uint8_t address, pca95xx_devices_e device, uint8_t numberOfGpio)
{
    _i2cPort = wirePort;
    _lastValidInput = 0xFF; // Default: all inputs HIGH (pullup logic)
    _cachedOutput = 0x00;   // Default: all outputs LOW

    switch (device)
    {
        default: // Default is the PCA9554
            _deviceAddress = (address != 255) ? (PCA95XX_Address_t)address : PCA9554_ADDRESS_20;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 8;
            _deviceType = PCA95XX_PCA9554;
            break;

        case PCA95XX_PCA9534:
            _deviceAddress = (address != 255) ? (PCA95XX_Address_t)address : PCA9534_ADDRESS_20;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 8;
            _deviceType = PCA95XX_PCA9534;
            break;

        case PCA95XX_PCA9536:
            _deviceAddress = (address != 255) ? (PCA95XX_Address_t)address : PCA9536_ADDRESS;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 4;
            _deviceType = PCA95XX_PCA9536;
            break;

        case PCA95XX_PCA9537:
            _deviceAddress = (address != 255) ? (PCA95XX_Address_t)address : PCA9537_ADDRESS;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 4;
            _deviceType = PCA95XX_PCA9537;
            break;

        case PCA95XX_PCA9554:
            _deviceAddress = (address != 255) ? (PCA95XX_Address_t)address : PCA9554_ADDRESS_20;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 8;
            _deviceType = PCA95XX_PCA9554;
            break;

        case PCA95XX_PCA9556:
            _deviceAddress = (address != 255) ? (PCA95XX_Address_t)address : PCA9556_ADDRESS_18;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 8;
            _deviceType = PCA95XX_PCA9556;
            break;

        case PCA95XX_PCA9557:
            _deviceAddress = (address != 255) ? (PCA95XX_Address_t)address : PCA9557_ADDRESS_18;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 8;
            _deviceType = PCA95XX_PCA9557;
            break;
    }
}

bool PCA95XX::begin()
{
    if (!isConnected())
        return false;

#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
    // Async queue mode: init must use blocking I2C so the chip is configured before any queued op runs.
    #if defined(ARDUINO_ARCH_RP2040)
    OpenKNX::I2C::PIOI2CWire *pioWire = dynamic_cast<OpenKNX::I2C::PIOI2CWire *>(_i2cPort);
    if (pioWire && pioWire->getInstance())
    {
        uint8_t reg = PCA95XX_REGISTER_INPUT_PORT;
        uint8_t inputReg = 0;
        if (pioWire->getInstance()->write_blocking(_deviceAddress, &reg, 1, true) >= 0 &&
            pioWire->getInstance()->read_blocking(_deviceAddress, &inputReg, 1, false) >= 0)
        {
            _lastValidInput = inputReg;
        }

        // Seed the output cache; afterwards _cachedOutput tracks intended state, no more chip reads
        reg = PCA95XX_REGISTER_OUTPUT_PORT;
        uint8_t outputReg = 0;
        if (pioWire->getInstance()->write_blocking(_deviceAddress, &reg, 1, true) >= 0 &&
            pioWire->getInstance()->read_blocking(_deviceAddress, &outputReg, 1, false) >= 0)
        {
            _cachedOutput = outputReg;
        }

        // Read CONFIG register for boot verification
        reg = PCA95XX_REGISTER_CONFIGURATION;
        uint8_t configReg = 0xFF;
        if (pioWire->getInstance()->write_blocking(_deviceAddress, &reg, 1, true) >= 0 &&
            pioWire->getInstance()->read_blocking(_deviceAddress, &configReg, 1, false) >= 0)
        {
            Serial.printf("PCA95xx::begin[ASYNC]: CONFIG=0x%02X OUTPUT=0x%02X (0=OUTPUT, 1=INPUT) addr=0x%02X\n",
                          configReg, outputReg, _deviceAddress);
        }
    }
    #endif
#else
    // Seed input cache to prevent ghost button events on boot
    uint8_t inputReg = 0;
    if (readI2CRegister(&inputReg, PCA95XX_REGISTER_INPUT_PORT) == PCA95XX_ERROR_SUCCESS)
    {
        _lastValidInput = inputReg;
    }

    uint8_t outputReg = 0;
    if (readI2CRegister(&outputReg, PCA95XX_REGISTER_OUTPUT_PORT) == PCA95XX_ERROR_SUCCESS)
    {
        _cachedOutput = outputReg;
    }

    // Read CONFIGURATION register for boot verification
    uint8_t configReg = 0;
    if (readI2CRegister(&configReg, PCA95XX_REGISTER_CONFIGURATION) == PCA95XX_ERROR_SUCCESS)
    {
        Serial.printf("PCA95xx::begin: CONFIG=0x%02X OUTPUT=0x%02X (0=OUTPUT, 1=INPUT)\n",
                      configReg, outputReg);
    }
#endif

    return true;
}

bool PCA95XX::isConnected(void)
{
    // Wire1 is shared with the OLED display; serialize this probe against a concurrent
    // display/LED flush (ESP32 50Hz flush task). No-op on RP2040.
    OPENKNX_WIRE1_LOCK();
    _i2cPort->beginTransmission((uint8_t)_deviceAddress);
    return (_i2cPort->endTransmission() == 0);
}

PCA95XX_error_t PCA95XX::pinMode(uint8_t pin, uint8_t mode)
{
    PCA95XX_error_t err;
    uint8_t cfgRegister = 0;

    if (pin >= _numberOfGpio)
        return PCA95XX_ERROR_UNDEFINED;

#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
    // Async queue mode: pinMode must be blocking, LED init calls it too early for queued writes.
    #if defined(ARDUINO_ARCH_RP2040)
    OpenKNX::I2C::PIOI2CWire *pioWire = dynamic_cast<OpenKNX::I2C::PIOI2CWire *>(_i2cPort);
    if (pioWire && pioWire->getInstance())
    {
        uint8_t reg = PCA95XX_REGISTER_CONFIGURATION;
        if (pioWire->getInstance()->write_blocking(_deviceAddress, &reg, 1, true) < 0 ||
            pioWire->getInstance()->read_blocking(_deviceAddress, &cfgRegister, 1, false) < 0)
        {
            return PCA95XX_ERROR_READ;
        }

        cfgRegister &= ~(1 << pin);
        if (mode == INPUT)
        {
            cfgRegister |= (1 << pin);
        }

        uint8_t buf[2] = {PCA95XX_REGISTER_CONFIGURATION, cfgRegister};
        if (pioWire->getInstance()->write_blocking(_deviceAddress, buf, 2, false) < 0)
        {
            return PCA95XX_ERROR_WRITE;
        }

        return PCA95XX_ERROR_SUCCESS;
    }
    #endif
#endif

    err = readI2CRegister(&cfgRegister, PCA95XX_REGISTER_CONFIGURATION);
    if (err != PCA95XX_ERROR_SUCCESS)
    {
        return err;
    }
    cfgRegister &= ~(1 << pin);
    if (mode == INPUT)
    {
        cfgRegister |= (1 << pin);
    }
    return writeI2CRegister(cfgRegister, PCA95XX_REGISTER_CONFIGURATION);
}

// Safe writing of a pin
PCA95XX_error_t PCA95XX::write(uint8_t pin, uint8_t value)
{
    if (pin >= _numberOfGpio)
        return PCA95XX_ERROR_UNDEFINED;

    // Critical section guards only the RMW of the _cachedOutput shadow so concurrent single-pin
    // updates don't clobber each other's bits.
    const uint8_t mask = 1 << pin;
#ifdef ARDUINO_ARCH_RP2040
    uint32_t irqState = save_and_disable_interrupts();
#endif
    uint8_t newOutput = (_cachedOutput & ~mask) | ((value != 0) << pin);
    _cachedOutput = newOutput;
#ifdef ARDUINO_ARCH_RP2040
    restore_interrupts(irqState);
#endif

    // Blocking I2C transaction runs with interrupts enabled: its completion path is IRQ-driven, so
    // holding IRQs off across it deadlocked the display-shared bus. Race-free because write() is only
    // reached from the core0 main loop (GPIO::flushPendingI2C); the ISR only sets pending flags.
    return writeI2CRegister(newOutput, PCA95XX_REGISTER_OUTPUT_PORT);
}

// digitalWrite overload
PCA95XX_error_t PCA95XX::digitalWrite(uint8_t pin, uint8_t value)
{
    return write(pin, value);
}

// Safe reading of input register
PCA95XX_error_t PCA95XX::getInputRegister(uint8_t *destination)
{
    PCA95XX_error_t err;
    uint8_t inputRegister = 0;

    err = readI2CRegister(&inputRegister, PCA95XX_REGISTER_INPUT_PORT);
    if (err != PCA95XX_ERROR_SUCCESS)
        return err;

    *destination = inputRegister;
    return (err);
}

// Unsafe overload
uint8_t PCA95XX::getInputRegister()
{
    uint8_t val;
    if (getInputRegister(&val) == PCA95XX_ERROR_SUCCESS)
        return val;
    return 0; // Unsafe
}

// Safe reading of a pin
PCA95XX_error_t PCA95XX::read(uint8_t *destination, uint8_t pin)
{
    PCA95XX_error_t err;
    uint8_t inputRegister = 0;

    if (pin >= _numberOfGpio)
        return PCA95XX_ERROR_UNDEFINED;

    err = readI2CRegister(&inputRegister, PCA95XX_REGISTER_INPUT_PORT);
    if (err != PCA95XX_ERROR_SUCCESS)
    {
        // On I2C error: return cached value from last successful read
        inputRegister = _lastValidInput;
    }
    else
    {
        // Success: cache this value for error recovery
        _lastValidInput = inputRegister;
    }

    *destination = (inputRegister & (1 << pin)) >> pin;
    return (err);
}

// Unsafe overload - returns last valid value on error (safer than 0)
uint8_t PCA95XX::read(uint8_t pin)
{
    uint8_t val;
    read(&val, pin); // Always returns a value (cached on error)
    return val;
}

// Safe reading of a pin
PCA95XX_error_t PCA95XX::digitalRead(uint8_t *destination, uint8_t pin)
{
    return (read(destination, pin));
}

// Unsafe overload - returns last valid value on error (safer than 0)
uint8_t PCA95XX::digitalRead(uint8_t pin)
{
    return read(pin); // Uses cached value on I2C error
}

// Invert the polarity of a pin
PCA95XX_error_t PCA95XX::invert(uint8_t pin, PCA95XX_invert_t inversion)
{
    if (pin >= _numberOfGpio)
        return PCA95XX_ERROR_UNDEFINED;

    uint8_t invertRegister;
    PCA95XX_error_t err = readI2CRegister(&invertRegister, PCA95XX_REGISTER_POLARITY_INVERSION);
    if (err != PCA95XX_ERROR_SUCCESS)
        return err;

    const uint8_t mask = 1 << pin;
    invertRegister = (invertRegister & ~mask) | ((inversion == PCA95XX_INVERT) << pin);

    return writeI2CRegister(invertRegister, PCA95XX_REGISTER_POLARITY_INVERSION);
}

// Revert the polarity of a pin
PCA95XX_error_t PCA95XX::revert(uint8_t pin)
{
    return invert(pin, PCA95XX_RETAIN);
}

// Private functions
PCA95XX_error_t PCA95XX::readI2CBuffer(uint8_t *dest, PCA95XX_REGISTER_t startRegister, uint16_t len)
{
    if (_deviceAddress == PCA95XX_ADDRESS_INVALID)
    {
        return PCA95XX_ERROR_INVALID_ADDRESS;
    }

    // One register read = one atomic Wire1 transaction; serializes against the display-shared bus.
    // Taken once here, callers do not hold the lock. No-op on RP2040.
    OPENKNX_WIRE1_LOCK();

    _i2cPort->beginTransmission((uint8_t)_deviceAddress);
    _i2cPort->write(startRegister);
    if (_i2cPort->endTransmission(false) != 0)
    {
        return PCA95XX_ERROR_READ;
    }

    _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)len);
    for (int i = 0; i < len; i++)
    {
        dest[i] = _i2cPort->read();
    }

    return PCA95XX_ERROR_SUCCESS;
}

PCA95XX_error_t PCA95XX::writeI2CBuffer(uint8_t *src, PCA95XX_REGISTER_t startRegister, uint16_t len)
{
    if (_deviceAddress == PCA95XX_ADDRESS_INVALID)
    {
        return PCA95XX_ERROR_INVALID_ADDRESS;
    }

    // One register write = one atomic Wire1 transaction; serializes against the display-shared bus.
    // Taken once here, callers do not hold it. No-op on RP2040.
    OPENKNX_WIRE1_LOCK();

    _i2cPort->beginTransmission((uint8_t)_deviceAddress);
    _i2cPort->write(startRegister);
    for (int i = 0; i < len; i++)
    {
        _i2cPort->write(src[i]);
    }
    uint8_t result = _i2cPort->endTransmission(true);

    if (result != 0)
    {
        return PCA95XX_ERROR_WRITE;
    }
    return PCA95XX_ERROR_SUCCESS;
}

PCA95XX_error_t PCA95XX::readI2CRegister(uint8_t *dest, PCA95XX_REGISTER_t registerAddress)
{
    PCA95XX_error_t result = readI2CBuffer(dest, registerAddress, 1);

    // Interrupt Errata: User must change command byte to something besides 00h after a Read operation
    if (_deviceType == PCA95XX_PCA9554 && registerAddress == PCA95XX_REGISTER_INPUT_PORT)
    {
        // If we've just read the INPUT Port register, we must change command byte
        writeI2CBuffer(0, PCA95XX_REGISTER_OUTPUT_PORT, 0); // Write the command register, but no data
    }
    return result;
}

PCA95XX_error_t PCA95XX::writeI2CRegister(uint8_t data, PCA95XX_REGISTER_t registerAddress)
{
    PCA95XX_error_t result = writeI2CBuffer(&data, registerAddress, 1);

    // Interrupt Errata: User must change command byte to something besides 00h after a Read operation
    if (_deviceType == PCA95XX_PCA9554 && registerAddress == PCA95XX_REGISTER_INPUT_PORT)
    {
        // If we've just read the INPUT Port register, we must change command byte
        writeI2CBuffer(0, PCA95XX_REGISTER_OUTPUT_PORT, 0); // Write the command register, but no data
    }
    return result;
}
