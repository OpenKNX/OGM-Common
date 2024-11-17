#pragma once
/**
 * OpenKNX Hardware definition header file
 * 
 * File: OpenKNX-UP1-Hardware.h
 * Hardware: OpenKNX UP1-Controller2040
 * Responsible: OpenKNX
 *
 * Defines hardware IO pins and functionalities for the OpenKNX PiPico BCU Connector.
 * Includes pin assignments for LEDs, buttons, serial communication, and other peripherals.
 * Ensures compatibility with various application boards and firmware features.
 *
 * More info about the Hardware visit: https://github.com/OpenKNX/OpenKNX/wiki/UP1-Controller2040
 *
 * ATTENTION:
 *    Do not include this file directly.
 *    It will be included by the OpenKNXHardware.h file.
 */

// UP1-Controller2040
#ifdef OKNXHW_UP1_CONTROLLER2040
    #define PROG_LED_PIN 6
    #define PROG_LED_PIN_ACTIVE_ON HIGH
    #define PROG_BUTTON_PIN 7
    #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
    #define SAVE_INTERRUPT_PIN 5
    #define KNX_SERIAL Serial1
    #define KNX_UART_RX_PIN 1
    #define KNX_UART_TX_PIN 0
#endif