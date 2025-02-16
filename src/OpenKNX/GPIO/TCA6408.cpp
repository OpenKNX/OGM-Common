#include "TCA6408.h"


//  REGISTERS
#define TCA6408_INPUT_PORT_REGISTER     0x00    //  read()
#define TCA6408_OUTPUT_PORT_REGISTER    0x01    //  write()
#define TCA6408_POLARITY_REGISTER       0x02    //  get/setPolarity()
#define TCA6408_CONFIGURATION_PORT      0x03    //  pinMode()


TCA6408::TCA6408(uint8_t address, TwoWire *wire)
{
  _address = address;
  _wire    = wire;
  _error   = TCA6408_OK;
  _type    = 55;
}


bool TCA6408::begin()
{
  if ((_address < 0x20) || (_address > 0x21)) return false;
  if (! isConnected()) return false;
  return true;
}


bool TCA6408::isConnected()
{
  _wire->beginTransmission(_address);
  return (_wire->endTransmission() == 0);
}


uint8_t TCA6408::getAddress()
{
  return _address;
}


//////////////////////////////////////////////////////////
//
//  1 PIN INTERFACE
//
bool TCA6408::pinMode1(uint8_t pin, uint8_t mode)   //  pin = 0..7
{
    if (pin > 7)
    {
        _error = TCA6408_PIN_ERROR;
        return false;
    }
    if ( (mode != INPUT) && (mode != OUTPUT) )
    {
        _error = TCA6408_VALUE_ERROR;
        return false;
    }

    uint8_t val = readRegister(TCA6408_CONFIGURATION_PORT);
    uint8_t mask = 1 << pin;
    if (mode == INPUT)  val |= mask;
    if (mode == OUTPUT) val &= ~mask;
    writeRegister(TCA6408_CONFIGURATION_PORT, val);
    _error = TCA6408_OK;
    return true;
}


bool TCA6408::write1(uint8_t pin, uint8_t value)   //  pin = 0..7
{
    if (pin > 7)
    {
        _error = TCA6408_PIN_ERROR;
        return false;
    }

    uint8_t val = readRegister(TCA6408_OUTPUT_PORT_REGISTER);
    uint8_t mask = 1 << pin;
    if (value) val |= mask;
    else val &= ~mask;
    writeRegister(TCA6408_OUTPUT_PORT_REGISTER, val);
    _error = TCA6408_OK;
    return true;
}


uint8_t TCA6408::read1(uint8_t pin)   //  pin = 0..7
{
    if (pin > 7)
    {
        _error = TCA6408_PIN_ERROR;
        return TCA6408_INVALID_READ;
    }

    uint8_t val = readRegister(TCA6408_INPUT_PORT_REGISTER);
    uint8_t mask = 1 << pin;
    _error = TCA6408_OK;
    if (val & mask) return HIGH;
    return LOW;
}


bool TCA6408::setPolarity(uint8_t pin, uint8_t value)   //  pin = 0..7
{
    if (pin > 7)
    {
        _error = TCA6408_PIN_ERROR;
        return false;
    }
    uint8_t val = readRegister(TCA6408_POLARITY_REGISTER);
    uint8_t mask = 1 << pin;
    if (value == HIGH) val |= mask;
    if (value == LOW)  val &= ~mask;
    writeRegister(TCA6408_POLARITY_REGISTER, val);
    _error = TCA6408_OK;
    return true;
}


uint8_t TCA6408::getPolarity(uint8_t pin)
{
    if (pin > 7)
    {
        _error = TCA6408_PIN_ERROR;
        return false;
    }
    _error = TCA6408_OK;
    uint8_t mask = readRegister(TCA6408_POLARITY_REGISTER);
    return (mask >> pin) == 0x01;
}



//////////////////////////////////////////////////////////
//
//  8 PIN INTERFACE
//
bool TCA6408::pinMode8(uint8_t mask)
{
  return writeRegister(TCA6408_CONFIGURATION_PORT, mask);
}


bool TCA6408::write8(uint8_t mask)
{
    return writeRegister(TCA6408_OUTPUT_PORT_REGISTER, mask);
}


int TCA6408::read8()
{
    return readRegister(TCA6408_INPUT_PORT_REGISTER);
}


bool TCA6408::setPolarity8(uint8_t mask)
{
    return writeRegister(TCA6408_POLARITY_REGISTER, mask);
}


uint8_t TCA6408::getPolarity8()
{
    return readRegister(TCA6408_POLARITY_REGISTER);
}


int TCA6408::lastError()
{
  int error = _error;
  _error = TCA6408_OK;  //  reset error after read.
  return error;
}


uint8_t TCA6408::getType()
{
  return _type;
}


////////////////////////////////////////////////////
//
//  PROTECTED
//
bool TCA6408::writeRegister(uint8_t reg, uint8_t value)
{
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _wire->write(value);
  if (_wire->endTransmission() != 0)
  {
    _error = TCA6408_I2C_ERROR;
    return false;
  }
  _error = TCA6408_OK;
  return true;
}


uint8_t TCA6408::readRegister(uint8_t reg)
{
  _wire->beginTransmission(_address);
  _wire->write(reg);
  int rv = _wire->endTransmission();
  if (rv != 0)
  {
    _error = TCA6408_I2C_ERROR;
    return rv;
  }
  else
  {
    _error = TCA6408_OK;
  }
  _wire->requestFrom(_address, (uint8_t)1);
  return _wire->read();
}