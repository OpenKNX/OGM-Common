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

// Constructor
PCA95XX::PCA95XX( TwoWire *wirePort, uint8_t address, pca95xx_devices_e device, uint8_t numberOfGpio)
{
    _i2cPort = wirePort;
    _lastValidInput = 0xFF; // Default: all inputs HIGH (pullup logic)
    _cachedOutput = 0x00;   // Default: all outputs LOW
    
    switch (device)
    {
        default: // Default is the PCA9554
            _deviceAddress = ( address != 255 ) ? (PCA95XX_Address_t)address : PCA9554_ADDRESS_20;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 8;
            _deviceType = PCA95XX_PCA9554;
            break;

        case PCA95XX_PCA9534:
            _deviceAddress = ( address != 255 ) ? (PCA95XX_Address_t)address : PCA9534_ADDRESS_20;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 8;
            _deviceType = PCA95XX_PCA9534;
            break;

        case PCA95XX_PCA9536:
            _deviceAddress = ( address != 255 ) ? (PCA95XX_Address_t)address : PCA9536_ADDRESS;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 4;
            _deviceType = PCA95XX_PCA9536;
            break;

        case PCA95XX_PCA9537:
            _deviceAddress = ( address != 255 ) ? (PCA95XX_Address_t)address : PCA9537_ADDRESS;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 4;
            _deviceType = PCA95XX_PCA9537;
            break;

        case PCA95XX_PCA9554:
            _deviceAddress = ( address != 255 ) ? (PCA95XX_Address_t)address : PCA9554_ADDRESS_20;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 8;
            _deviceType = PCA95XX_PCA9554;
            break;

        case PCA95XX_PCA9556:
            _deviceAddress = ( address != 255 ) ? (PCA95XX_Address_t)address : PCA9556_ADDRESS_18;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 8;
            _deviceType = PCA95XX_PCA9556;
            break;

        case PCA95XX_PCA9557:
            _deviceAddress = ( address != 255 ) ? (PCA95XX_Address_t)address : PCA9557_ADDRESS_18;
            _numberOfGpio = (numberOfGpio != 255) ? numberOfGpio : 8;
            _deviceType = PCA95XX_PCA9557;
            break;
    }
}

bool PCA95XX::begin()
{
    if (!isConnected())
        return false;
    
    // Initialize cache with actual INPUT register value (prevent ghost button events on boot)
    uint8_t inputReg = 0;
    if (readI2CRegister(&inputReg, PCA95XX_REGISTER_INPUT_PORT) == PCA95XX_ERROR_SUCCESS)
    {
        _lastValidInput = inputReg;
    }
    
    // Initialize OUTPUT cache with actual register value (for consistent state)
    uint8_t outputReg = 0;
    if (readI2CRegister(&outputReg, PCA95XX_REGISTER_OUTPUT_PORT) == PCA95XX_ERROR_SUCCESS)
    {
        _cachedOutput = outputReg;
    }
    
    return true;
}

bool PCA95XX::isConnected(void)
{
    _i2cPort->beginTransmission((uint8_t)_deviceAddress);
    return (_i2cPort->endTransmission() == 0);
}

PCA95XX_error_t PCA95XX::pinMode(uint8_t pin, uint8_t mode)
{
    PCA95XX_error_t err;
    uint8_t cfgRegister = 0;

    if (pin >= _numberOfGpio)
        return PCA95XX_ERROR_UNDEFINED;

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

    // **Atomic Cached Modify-Write** (NO Read from chip!)
    // Thread-safe between ISR calls - atomic update prevents race conditions
    // when multiple LEDs write simultaneously
    
#ifdef ARDUINO_ARCH_RP2040
    // RP2040: Use atomic section for cache update
    uint32_t irqState = save_and_disable_interrupts();
#endif
    
    const uint8_t mask = 1 << pin;
    uint8_t newOutput = (_cachedOutput & ~mask) | ((value != 0) << pin);
    _cachedOutput = newOutput;  // Update cache atomically
    
#ifdef ARDUINO_ARCH_RP2040
    restore_interrupts(irqState);
#endif
    
    // Write to chip (may be queued if ASYNC_QUEUE enabled)
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
    _i2cPort->beginTransmission((uint8_t)_deviceAddress);
    _i2cPort->write(startRegister);
    for (int i = 0; i < len; i++)
    {
        _i2cPort->write(src[i]);
    }
    if (_i2cPort->endTransmission(true) != 0)
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
