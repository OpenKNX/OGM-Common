#pragma once

#include <cstdint>
#include "knx.h"
#include <hardware.h>

// #ifndef BOARD_ENDUSER
// // Board specific definietions
// // #define BOARD_MASIFI
// // ################################################
// // ### Board Configuration
// // ################################################
// #ifdef BOARD_DEVEL
// #define PROG_LED_PIN 26
// #define PROG_LED_PIN_ACTIVE_ON LOW
// #define PROG_BUTTON_PIN 10
// #define PROG_BUTTON_PIN_INTERRUPT_ON RISING
// #define SAVE_INTERRUPT_PIN 17
// #define BUZZER_PIN 18
// #define I2C_EEPROM_DEVICE_ADDRESSS 0xFF // Address of 24LC256 eeprom chip
// #endif
// #ifdef BOARD_ENOCEAN
// #ifndef PROG_LED_PIN
// #define PROG_LED_PIN 10
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #endif
// #ifndef PROG_BUTTON_PIN
// #define PROG_BUTTON_PIN 8
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #endif
// #define SAVE_INTERRUPT_PIN A9
// // #define INFO_LED_PIN 38
// // #define INFO_LED_PIN_ACTIVE_ON HIGH
// // #define COUNT_LOG_CHANNEL 80
// // Buzzer
// // #define BUZZER_PIN 9
// // #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #define NO_I2C
// #endif
// #ifdef BOARD_MASIFI_V1
// #define PROG_LED_PIN 13
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 11
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN 8
// #define INFO_LED_PIN 38
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define COUNT_1WIRE_BUSMASTER 1
// #define COUNT_1WIRE_CHANNEL 30
// // #define COUNT_PM_CHANNEL 6
// // #define COUNT_LOG_CHANNEL 80
// // Buzzer
// #define BUZZER_PIN 9
// #define I2C_1WIRE_DEVICE_ADDRESSS 0x1A  // Address of DS2482 1-Wire-Busmaster chip
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #define I2C_RGBLED_DEVICE_ADDRESS 0x60  // Address of PCA9632 RBGW-LED-Driver
// #endif
// #ifdef BOARD_MASIFI_V2
// #define PROG_LED_PIN 13
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 11
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN 8
// #define INFO_LED_PIN 38
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define COUNT_1WIRE_BUSMASTER 1
// #define COUNT_1WIRE_CHANNEL 30
// // #define COUNT_PM_CHANNEL 6
// // #define COUNT_LOG_CHANNEL 99
// // Buzzer
// #define BUZZER_PIN 9
// #define I2C_1WIRE_DEVICE_ADDRESSS 0x18 // Address of DS2484 1-Wire-Busmaster chip
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #define I2C_RGBLED_DEVICE_ADDRESS 0x60  // Address of PCA9632 RBGW-LED-Driver
// #endif
// #ifdef BOARD_MASIFI_V3
// #define PROG_LED_PIN 13
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 11
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN A2 // 8
// #define INFO_LED_PIN 38
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define COUNT_1WIRE_BUSMASTER 1
// #define COUNT_1WIRE_CHANNEL 30
// // #define COUNT_PM_CHANNEL 6
// // #define COUNT_LOG_CHANNEL 80
// // Buzzer
// #define BUZZER_PIN 9
// #define I2C_1WIRE_DEVICE_ADDRESSS 0x18  // Address of DS2484 1-Wire-Busmaster chip
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #define I2C_RGBLED_DEVICE_ADDRESS 0x60  // Address of PCA9632 RBGW-LED-Driver
// #endif
// #ifdef BOARD_MASIFI_V31
// #define PROG_LED_PIN 13
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 12
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN A2 // 8
// #define INFO_LED_PIN 38
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define COUNT_1WIRE_BUSMASTER 1
// #define COUNT_1WIRE_CHANNEL 30
// // #define COUNT_PM_CHANNEL 6
// // #define COUNT_LOG_CHANNEL 80
// // Buzzer
// #define BUZZER_PIN 9
// #define I2C_1WIRE_DEVICE_ADDRESSS 0x18  // Address of DS2484 1-Wire-Busmaster chip
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #define I2C_RGBLED_DEVICE_ADDRESS 0x60  // Address of PCA9632 RBGW-LED-Driver
// #endif
// #ifdef BOARD_MASIFI_AUSSEN_V13
// #define PROG_LED_PIN 13
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 12
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN A2 // 8
// #define INFO_LED_PIN 38
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define COUNT_1WIRE_BUSMASTER 1
// #define COUNT_1WIRE_CHANNEL 30
// // #define COUNT_LOG_CHANNEL 80
// // Buzzer
// #define BUZZER_PIN 9
// #define I2C_1WIRE_DEVICE_ADDRESSS 0x18  // Address of DS2484 1-Wire-Busmaster chip
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #define I2C_RGBLED_DEVICE_ADDRESS 0x60  // Address of PCA9632 RBGW-LED-Driver
// #endif
// // Sensormodul auf RP2040 Basis
// #ifdef BOARD_MASIFI_PICO
// #define PROG_LED_PIN 12
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 28
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN D29 // 8
// // #define INFO_LED_PIN 38
// // #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define COUNT_1WIRE_BUSMASTER 1
// #define COUNT_1WIRE_CHANNEL 30
// // #define COUNT_PM_CHANNEL 6
// // #define COUNT_LOG_CHANNEL 99
// #define KNX_UART_RX_PIN 17
// #define KNX_UART_TX_PIN 16
// #define KNX_I2C_SDA_PIN 20
// #define KNX_I2C_SCL_PIN 21
// // Buzzer
// #define BUZZER_PIN 27
// // #define I2C_1WIRE_DEVICE_ADDRESSS 0x18  // Address of DS2484 1-Wire-Busmaster chip
// // #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// // #define I2C_RGBLED_DEVICE_ADDRESS 0x60  // Address of PCA9632 RBGW-LED-Driver
// #endif
// #ifdef BOARD_SIRSYDOM_PIPICO_BCU_CONNECTOR
// #define PROG_LED_PIN 21
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 22
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN 20 // 8
// // #define INFO_LED_PIN 38
// // #define INFO_LED_PIN_ACTIVE_ON HIGH
// // #define COUNT_1WIRE_BUSMASTER 1
// // #define COUNT_1WIRE_CHANNEL 30
// // #define COUNT_PM_CHANNEL 6
// // #define COUNT_LOG_CHANNEL 99
// #define KNX_UART_RX_PIN 1
// #define KNX_UART_TX_PIN 0
// // #define KNX_I2C_SDA_PIN 20
// // #define KNX_I2C_SCL_PIN 21
// // Buzzer
// #define BUZZER_PIN 27
// // #define I2C_1WIRE_DEVICE_ADDRESSS 0x18  // Address of DS2484 1-Wire-Busmaster chip
// // #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// // #define I2C_RGBLED_DEVICE_ADDRESS 0x60  // Address of PCA9632 RBGW-LED-Driver
// #endif
// // HF-Firmware-Test auf RP2040-Sensormodul Basis (reiner Test)
// #ifdef BOARD_MASIFI_PICO_SEN_PM_TEST
// #define PROG_LED_PIN 12
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 28
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN D29 // 8
// // #define INFO_LED_PIN 38
// // #define INFO_LED_PIN_ACTIVE_ON HIGH
// // #define COUNT_1WIRE_BUSMASTER 1
// // #define COUNT_1WIRE_CHANNEL 30
// // #define COUNT_PM_CHANNEL 6
// // #define COUNT_LOG_CHANNEL 99
// #define KNX_UART_RX_PIN 17
// #define KNX_UART_TX_PIN 16
// #define KNX_I2C_SDA_PIN 20
// #define KNX_I2C_SCL_PIN 21
// // Buzzer
// #define BUZZER_PIN 27
// #define PRESENCE_LED_PIN 9
// #define PRESENCE_LED_PIN_ACTIVE_ON HIGH
// #define MOVE_LED_PIN 8
// #define MOVE_LED_PIN_ACTIVE_ON HIGH
// #define I2C_SDA_PIN 2
// #define I2C_SCL_PIN 3
// #define HF_UART_TX_PIN 4
// #define HF_UART_RX_PIN 5
// #define HF_S1_PIN 6
// #define HF_S2_PIN 7
// #define HF_POWER_PIN 14
// // #define I2C_1WIRE_DEVICE_ADDRESSS 0x18  // Address of DS2484 1-Wire-Busmaster chip
// // #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// // #define I2C_RGBLED_DEVICE_ADDRESS 0x60  // Address of PCA9632 RBGW-LED-Driver
// #endif
// #ifdef BOARD_MASIFI_HFPM_DEVEL
// #define PROG_LED_PIN 16
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 17
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define INFO_LED_PIN 10
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define PRESENCE_LED_PIN 9
// #define PRESENCE_LED_PIN_ACTIVE_ON HIGH
// #define MOVE_LED_PIN 8
// #define MOVE_LED_PIN_ACTIVE_ON HIGH
// #define KNX_UART_TX_PIN 12
// #define KNX_UART_RX_PIN 13
// #define I2C_SDA_PIN 2
// #define I2C_SCL_PIN 3
// #define HF_UART_TX_PIN 4
// #define HF_UART_RX_PIN 5
// #define HF_S1_PIN 6
// #define HF_S2_PIN 7
// #define HF_POWER_PIN 14
// #define SAVE_INTERRUPT_PIN 15
// // #define SENSOR_I2C_OPT300x Wire1
// // #define SENSOR_I2C_VEML7700 Wire1
// // #define PIR_PIN 14
// // #define COUNT_PM_CHANNEL 15
// // #define COUNT_LOG_CHANNEL 80
// // Buzzer
// // #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #endif
// #ifdef BOARD_MASIFI_HFPM_DEVEL2
// #define PROG_LED_PIN 18
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 17
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// // #define INFO_LED_PIN 9
// // #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define PRESENCE_LED_PIN 0
// #define PRESENCE_LED_PIN_ACTIVE_ON HIGH
// #define MOVE_LED_PIN 1
// #define MOVE_LED_PIN_ACTIVE_ON HIGH
// #define KNX_UART_TX_PIN 12
// #define KNX_UART_RX_PIN 13
// #define I2C_SDA_PIN 26 //I2C1
// #define I2C_SCL_PIN 27 //I2C1
// #define HF_UART_TX_PIN 4
// #define HF_UART_RX_PIN 5
// #define HF_S1_PIN 2
// #define HF_S2_PIN 3
// #define HF_POWER_PIN 10
// #define SAVE_INTERRUPT_PIN 11
// // #define SENSOR_I2C_OPT300x Wire1
// // #define SENSOR_I2C_VEML7700 Wire1
// #endif
// #ifdef BOARD_MASIFI_PM
// #define PROG_LED_PIN 13
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 11
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN 0
// #define INFO_LED_PIN 26
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define PRESENCE_LED_PIN 8
// #define PRESENCE_LED_PIN_ACTIVE_ON HIGH
// #define PIR_PIN 14
// // #define COUNT_PM_CHANNEL 15
// // #define COUNT_LOG_CHANNEL 80
// // Buzzer
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #endif
// #ifdef BOARD_PM_V3
// #define PROG_LED_PIN 13
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 11
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN A2 // 8
// #define INFO_LED_PIN 38
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define COUNT_1WIRE_BUSMASTER 1
// #define COUNT_1WIRE_CHANNEL 30
// // #define COUNT_PM_CHANNEL 20
// // #define COUNT_LOG_CHANNEL 80
// // Buzzer
// #define BUZZER_PIN 9
// #define I2C_1WIRE_DEVICE_ADDRESSS 0x18  // Address of DS2484 1-Wire-Busmaster chip
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #define I2C_RGBLED_DEVICE_ADDRESS 0x60  // Address of PCA9632 RBGW-LED-Driver
// #endif
// #ifdef BOARD_PM_V31
// #define PROG_LED_PIN 13
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN 12
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define SAVE_INTERRUPT_PIN A2 // 8
// #define INFO_LED_PIN 38
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define COUNT_1WIRE_BUSMASTER 1
// #define COUNT_1WIRE_CHANNEL 30
// // #define COUNT_PM_CHANNEL 20
// // #define COUNT_LOG_CHANNEL 80
// // Buzzer
// #define BUZZER_PIN 9
// #define I2C_1WIRE_DEVICE_ADDRESSS 0x18  // Address of DS2484 1-Wire-Busmaster chip
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #define I2C_RGBLED_DEVICE_ADDRESS 0x60  // Address of PCA9632 RBGW-LED-Driver
// #endif
// #ifdef BOARD_MASIFI_ONEWIRE
// #define PROG_LED_PIN 26
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN A1
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define INFO_LED_PIN 25
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define COUNT_1WIRE_BUSMASTER 3
// #define COUNT_1WIRE_CHANNEL 90
// // #define COUNT_LOG_CHANNEL 80
// #define I2C_1WIRE_DEVICE_ADDRESSS 0x18 // Address of first DS2482 1-Wire-Busmaster chip, used are 0x19, 0x1A, 0x1B
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #endif
// #ifdef BOARD_MASIFI_ONEWIRE_ITSYBITSY_M0
// #define PROG_LED_PIN 9
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN A1
// #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
// #define INFO_LED_PIN 13
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define COUNT_1WIRE_BUSMASTER 3
// #define COUNT_1WIRE_CHANNEL 90
// // #define COUNT_LOG_CHANNEL 80
// #define I2C_1WIRE_DEVICE_ADDRESSS 0x18 // Address of first DS2482 1-Wire-Busmaster chip, used are 0x19, 0x1A, 0x1B
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #endif
// // ModbusGateway from Masifi, SAMD hardware
// #ifdef BOARD_MASIFI_MODBUS_SAMD
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN A1
// #define INFO_LED_PIN 13
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #endif
// // ModbusGateway from Masifi, RP2040 hardware
// #ifdef BOARD_MASIFI_MODBUS_RP2040
// #define PROG_LED_PIN_ACTIVE_ON HIGH
// #define PROG_BUTTON_PIN A1
// #define INFO_LED_PIN 13
// #define INFO_LED_PIN_ACTIVE_ON HIGH
// #define KNX_UART_TX_PIN 12
// #define KNX_UART_RX_PIN 13
// #endif
// #endif
// // board independent definitions

// fatal error codes
#define FATAL_I2C_BUSY                3  // I2C busy during startup
#define FATAL_LOG_WRONG_CHANNEL_COUNT 4  // knxprod contains more channels than logic supports
#define FATAL_SENS_UNKNOWN            5  // unknown or unsupported sensor
#define FATAL_SCHEDULE_MAX_CALLBACKS  6  // Too many callbacks in scheduler

// // EEPROM Support
// #define I2C_EEPROM_DEVICE_ADDRESSS 0x50 // Address of 24LC256 eeprom chip
// #define SAVE_BUFFER_START_PAGE     0    // All stored KO data begin at this page and takes 37 pages, 
//                                         // allow 3 pages boundary, so next store should start at page 40


#define BOARD_HW_EEPROM  0x01
#define BOARD_HW_LED     0x02
#define BOARD_HW_ONEWIRE 0x04
#define BOARD_HW_NCN5130 0x08

struct sSensorInfo
{
    float lastValue;
    float lastSentValue;
    uint32_t sendDelay;
    uint32_t readDelay;
};

struct sActorInfo
{
    uint8_t lastOutputValue;
    uint8_t lastInputValue;
    uint32_t sendDelay;
    uint32_t readDelay;
};

union uData
{
    sSensorInfo sensor;
    sActorInfo actor;
};

// call this BEFORE Wire.begin()
// it clears I2C Bus, calls Wire.begin() and checks which board hardware is available
bool boardCheck();
bool checkUartExistence();
bool initUart();
uint8_t sendUartCommand(const char* iInfo, uint8_t iCmd, uint8_t iResp, uint8_t iLen = 0);

// just work after boardCheck
bool boardWithOneWire();
bool boardWithLed();
bool boardWithEEPROM();
bool boardWithNCN5130();

void ledInfo(bool iOn);
void ledProg(bool iOn);
// Turn off 5V rail from NCN5130 to save power for EEPROM write during knx save operation
void deactivatePowerRail();
// Turn on 5V rail from NCN5130 in case SAVE-Interrupt was false positive
void activatePowerRail();

void fatalError(uint8_t iErrorCode, const char *iErrorText = 0);

uint8_t clearI2cBus();
