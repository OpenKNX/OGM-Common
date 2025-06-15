#pragma once

#include "Arduino.h"
#include "Wire.h"



#define TCA6408_OK                        0x00
#define TCA6408_PIN_ERROR                 0x81
#define TCA6408_I2C_ERROR                 0x82
#define TCA6408_VALUE_ERROR               0x83

#define TCA6408_INVALID_READ              -100


#if !defined(TCA6408_PIN_NAMES)
#define TCA6408_PIN_NAMES

  #define TCA_P00         0
  #define TCA_P01         1
  #define TCA_P02         2
  #define TCA_P03         3
  #define TCA_P04         4
  #define TCA_P05         5
  #define TCA_P06         6
  #define TCA_P07         7

#endif


class TCA6408
{
public:
  TCA6408(uint8_t address, TwoWire *wire = &Wire);

  bool     begin();
  bool     isConnected();
  uint8_t  getAddress();


  //  1 PIN INTERFACE
  //  pin    = 0..15
  //  mode  = INPUT, OUTPUT       (INPUT_PULLUP is not supported)
  //  value = LOW, HIGH
  bool     pinMode1(uint8_t pin, uint8_t mode);
  bool     write1(uint8_t pin, uint8_t value);
  uint8_t  read1(uint8_t pin);
  bool     setPolarity(uint8_t pin, uint8_t value);    //  input pins only.
  uint8_t  getPolarity(uint8_t pin);


  //  8 PIN INTERFACE
  //  port  = 0..1
  //  mask  = bit pattern
  bool     pinMode8(uint8_t mask);
  bool     write8(uint8_t mask);
  int      read8();
  bool     setPolarity8(uint8_t value);
  uint8_t  getPolarity8();


  int      lastError();
  uint8_t  getType();


protected:
  bool     writeRegister(uint8_t reg, uint8_t value);
  uint8_t  readRegister(uint8_t reg);

  uint8_t  _address;
  TwoWire* _wire;
  uint8_t  _error;
  uint8_t  _type;
};
