#pragma once
/**
 * OpenKNX Hardware definition header file
 * 
 * File: misc.h
 * Hardware: OpenKNX Miscellanous Hardware
 * Responsible: OpenKNX
 *
 * Defines hardware IO pins and functionalities for the OpenKNX Miscellanous Hardware.
 * Includes pin assignments for LEDs, buttons, serial communication, and other peripherals.
 * Ensures compatibility with various application boards and firmware features.
 *
 * More info about the Hardware visit: https://www.OpenKNX.de
 *
 * ATTENTION:
 *    Do not include this file directly.
 *    It will be included by the OpenKNXHardware.h file.
 */

// PiPico-BCU-Connector
#ifdef OKNXHW_PIPICO_BCU_CONNECTOR
    #define PROG_LED_PIN 21
    #define PROG_LED_PIN_ACTIVE_ON HIGH
    #define PROG_BUTTON_PIN 22
    #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
    #define SAVE_INTERRUPT_PIN 20
    #define KNX_SERIAL Serial1
    #define KNX_UART_RX_PIN 1
    #define KNX_UART_TX_PIN 0
#endif