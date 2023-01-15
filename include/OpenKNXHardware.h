// This file defines hardware properties of several reusable OpenKNX hardware
// It is meant to be included in the project-specific xyzHardware.h

// PiPico-BCU-Connector
// https://github.com/OpenKNX/OpenKNX/wiki/PiPico-BCU-Connector

#ifdef OKNXHW_PIPICO_BCU_CONNECTOR
#define PROG_LED_PIN 21
#define PROG_LED_PIN_ACTIVE_ON HIGH
#define PROG_BUTTON_PIN 22
#define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
#define SAVE_INTERRUPT_PIN 20
#define KNX_UART_RX_PIN 1
#define KNX_UART_TX_PIN 0
#endif


// REG1-Controller2040
// https://github.com/OpenKNX/OpenKNX/wiki/REG1-Controller2040

#ifdef OKNXHW_REG1_CONTROLLER2040
#define PROG_LED_PIN 2
#define PROG_LED_PIN_ACTIVE_ON HIGH
#define PROG_BUTTON_PIN 7
#define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
#define SAVE_INTERRUPT_PIN 6
#define INFO_LED_PIN 3
#define INFO_LED_PIN_ACTIVE_ON HIGH
#define KNX_UART_RX_PIN 1
#define KNX_UART_TX_PIN 0
#endif


// UP1-Controller2040
// https://github.com/OpenKNX/OpenKNX/wiki/UP1-Controller2040

#ifdef OKNXHW_UP1_CONTROLLER2040
#define PROG_LED_PIN 6
#define PROG_LED_PIN_ACTIVE_ON HIGH
#define PROG_BUTTON_PIN 7
#define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
#define SAVE_INTERRUPT_PIN 5
#define KNX_UART_RX_PIN 1
#define KNX_UART_TX_PIN 0
#endif

