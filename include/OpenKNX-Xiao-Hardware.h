#pragma once
/**
 * OpenKNX Hardware Definition Header File
 *
 * File: OpenKNXiao-Hardware.h
 * Hardware: OpenKNXiao RP2040 / SAMD / ESP Based Boards
 * Responsible: OpenKNX - Erkan Çolak
 *
 * Defines hardware IO pins and functionalities for OpenKNXiao RP2040 / SAMD platform.
 * Includes pin assignments for LEDs, buttons, serial communication, I2C interfaces,
 * and other peripherals. Ensures compatibility with various application boards and
 * firmware features.
 *
 * Configurations are categorized by hardware versions and features:
 * - OpenKNXiao RP2040 ( Current Versions: V1 )
 * - OpenKNXiao SAMD ( Current Versions: V1 )
 * - OpenKNXiao ESP32 ( Current Versions: V1 )
 * - Wi-Fi
 * - Meter support, WS2812/NeoPixel, etc.
 *
 * Each configuration is guarded by preprocessor directives to enable/disable features.
 * More info about the Hardware visit: https://github.com/OpenKNX/OpenKNXiao
 *
 * Section: Product configurations / use cases
 *          Describes the available configurations for the OpenKNXiao RP2040 / SAMD  / ESP hardware.
 * Section: Firmware Features (FwF) based IO and Pin Definitions
 *          Defines the pin assignments for the firmware features.
 * Section: Hardware specific Pin Definitions
 *          Defines the pin assignments for the separate hardware specific configurations.
 * Section: Common Hardware (CHW) Pin Definitions
 *          Defines the pin assignments for the common hardware features.
 * Section: FwF and CHW based Pin Definitions
 *          Defines the pin assignments for the firmware features and common hardware features.
 *
 * ATTENTION:
 *    Do not include this file directly.
 *    It will be included by the OpenKNXHardware.h file.
 *
 */

/**
 * Section: Product configurations / use cases
 * OpenKNXiao RP2040 / SAMD / ESP
 */

// OpenKNXiao KNeoPix RP2040 V1
// https://github.com/OpenKNX/OpenKNX-KNeoPiX
#ifdef OKNXHW_OPENKNXIAO_KNEOPIX_RP2040_V1
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-OpenKNXiao-KNeoPix-RP2040-V1"
    #endif
    #define OKNXHW_OPENKNXIAO_RP2040_V1_COMMON
    #define OKNXHW_OPENKNXIAO_RP2040_V1_TERMINAL
    #define OKNXHW_OPENKNXIAO_RP2040_V1_TERMINAL_NEOPIX
    #define OKNXHW_OPENKNXIAO_RP2040_V1_LED1
    #define OKNXHW_OPENKNXIAO_RP2040_V1_LED2
    #define OKNXHW_OPENKNXIAO_RP2040_V1_NEOPIXEL
#endif

// OpenKNXiao Mini RP2040 V1
// https://github.com/OpenKNX/OpenKNXiao-Mini
#ifdef OKNXHW_OPENKNXIAO_RP2040_MINI_V1
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-OpenKNXiao-Mini-RP2040-V1"
    #endif
    #define OKNXHW_OPENKNXIAO_RP2040_V1_COMMON
    #define OKNXHW_OPENKNXIAO_RP2040_V1_TERMINAL
    #define OKNXHW_OPENKNXIAO_RP2040_V1_TERMINAL_MINI
    #define OKNXHW_OPENKNXIAO_RP2040_V1_LED1
    #define OKNXHW_OPENKNXIAO_RP2040_V1_LED2
    #define OKNXHW_OPENKNXIAO_RP2040_V1_NEOPIXEL
#endif

// OpenKNXiao KNeoPix RP2040 V1 (Meter)
// https://github.com/OpenKNX/OpenKNX-KNeoPiX
#ifdef OKNXHW_OPENKNXIAO_KNEOPIX_RP2040_V1_METER
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-OpenKNXiao-KNeoPix-RP2040-V1"
    #endif
    #define OKNXHW_OPENKNXIAO_RP2040_V1_COMMON
    #define OKNXHW_OPENKNXIAO_RP2040_V1_TERMINAL
    #define OKNXHW_OPENKNXIAO_RP2040_V1_TERMINAL_NEOPIX
    #define OKNXHW_OPENKNXIAO_RP2040_V1_LED1
    #define OKNXHW_OPENKNXIAO_RP2040_V1_LED2
    #define OKNXHW_OPENKNXIAO_RP2040_V1_NEOPIXEL
    #define OKNXHW_OPENKNXIAO_RP2040_V1_METER
#endif

// OpenKNXiao Mini RP2040 V1 (Meter)
// https://github.com/OpenKNX/OpenKNXiao-Mini
#ifdef OKNXHW_OPENKNXIAO_RP2040_MINI_V1_METER
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-OpenKNXiao-Mini-RP2040-V1"
    #endif
    #define OKNXHW_OPENKNXIAO_RP2040_V1_COMMON
    #define OKNXHW_OPENKNXIAO_RP2040_V1_TERMINAL
    #define OKNXHW_OPENKNXIAO_RP2040_V1_TERMINAL_MINI
    #define OKNXHW_OPENKNXIAO_RP2040_V1_LED1
    #define OKNXHW_OPENKNXIAO_RP2040_V1_LED2
    #define OKNXHW_OPENKNXIAO_RP2040_V1_NEOPIXEL
    #define OKNXHW_OPENKNXIAO_RP2040_V1_METER
#endif

// OpenKNXiao KNeoPix SAMD21 V1
// https://github.com/OpenKNX/OpenKNX-KNeoPiX
#ifdef OKNXHW_OPENKNXIAO_KNEOPIX_SAMD21_V1
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-OpenKNXiao-KNeoPix-SAMD21-V1"
    #endif
    #define OKNXHW_OPENKNXIAO_SAMD21_V1_COMMON
    #define OKNXHW_OPENKNXIAO_SAMD21_V1_TERMINAL
    #define OKNXHW_OPENKNXIAO_SAMD21_V1_TERMINAL_NEOPIX
    #define OKNXHW_OPENKNXIAO_SAMD21_V1_LED1
    #define OKNXHW_OPENKNXIAO_SAMD21_V1_LED2
#endif

// OpenKNXiao Mini SAMD21 V1
// https://github.com/OpenKNX/OpenKNXiao-Mini
#ifdef OKNXHW_OPENKNXIAO_SAMD21_MINI_V1
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-OpenKNXiao-Mini-SAMD21-V1"
    #endif
    #define OKNXHW_OPENKNXIAO_SAMD21_V1_COMMON
    #define OKNXHW_OPENKNXIAO_SAMD21_V1_TERMINAL
    #define OKNXHW_OPENKNXIAO_SAMD21_V1_TERMINAL_MINI
    #define OKNXHW_OPENKNXIAO_SAMD21_V1_LED1
    #define OKNXHW_OPENKNXIAO_SAMD21_V1_LED2
#endif

/**
 * Section: Firmware Features (FwF) based IO and Pin Definitions
 * OpenKNXiao RP2040 / SAMD / ESP
 */

// OpenKNXiao RP2040 V1 FwF: Meter Support
#ifdef OKNXHW_OPENKNXIAO_RP2040_V1_METER
    // Default pins for the Meter Sensors typ Rx/Tx
    #define OKNXHW_OPENKNXIAO_MSENS_1_SDA0_TX_PIN 2  // GPIO2  | SPI0 TX  | UART1 RTS | I2C1 SDA | PWM1 B
    #define OKNXHW_OPENKNXIAO_MSENS_1_SCL0_RX_PIN 3  // GPIO3  | SPI0 RX  | UART1 TX  | I2C1 SCL | PWM2 B
    #define OKNXHW_OPENKNXIAO_MSENS_2_SDA1_TX_PIN 8  // GPIO8  | SPI1 RX  | UART1 TX  | I2C0 SDA | PWM4 A
    #define OKNXHW_OPENKNXIAO_MSENS_2_SCL1_RX_PIN 9  // GPIO9  | SPI1 TX  | UART1 RX  | I2C0 SCL | PWM4 B
#endif

/**
 * Section: Hardware specific Pin Definitions
 * OpenKNXiao RP2040 / SAMD / ESP
 */

// OpenKNXiao RP2040 V1 LED1
#ifdef OKNXHW_OPENKNXIAO_RP2040_V1_LED1
    #define INFO1_LED_PIN 16 // Build-In LED GREEN (RED:GPIO17 | GREEN:GPIO16 | BLUE:GPIO25)
    #define INFO1_LED_PIN_ACTIVE_ON HIGH
#endif

// OpenKNXiao RP2040 V1 LED2
#ifdef OKNXHW_OPENKNXIAO_RP2040_V1_LED2
    #define INFO2_LED_PIN 25 // Build-In LED BLUE (RED:GPIO17 | GREEN:GPIO16 | BLUE:GPIO25)
    #define INFO2_LED_PIN_ACTIVE_ON HIGH
#endif

// OpenKNXiao RP2040 V1 RGB LED (NeoPixel LED)
#ifdef OKNXHW_OPENKNXIAO_RP2040_V1_NEOPIXEL
    #define NEOPIX_LED_POWER_PIN 11   // NeoPixel LED Power Pin
    #define NEOPIX_LED_PIN 12         // Build-In RGB NeoPixel LED
    #define NEOPIX_LED_COUNT 1        // Number of NeoPixel LEDs
    #define NEOPIX_LED_TYPE NEO_GRB   // NeoPixel LED Type
    #define NEOPIX_LED_ORDER NEO_GRB  // NeoPixel LED Order
    #define NEOPIX_LED_ACTIVE_ON HIGH // Active on HIGH
    #define NEOPIX_LED_SPEED 800000   // NeoPixel LED Speed (800 KHz / NEO_KHZ800)
#endif

/**
 * Section: Common Hardware (CHW) Pin Definitions
 * OpenKNXiao RP2040
 */

// OpenKNXiao RP2040 CHW Pins
#ifdef OKNXHW_OPENKNXIAO_RP2040_V1_COMMON
    #define PROG_LED_PIN 17                      // Build-In LED RED (RED:GPIO17 | GREEN:GPIO16 | BLUE:GPIO25)
    #define PROG_LED_PIN_ACTIVE_ON HIGH          // Active on HIGH
    #define PROG_BUTTON_PIN 0                    // GPIO0 | UART0 RX | I2C1 SDA | PWM0 A
    #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING // Interrupt on FALLING
    #define KNX_SERIAL Serial1                   // Serial1 for UART1
    #define KNX_UART_RX_PIN 6                    // GPIO6 | SPI1 TX  | UART1 RX  | I2C0 SCL | PWM4 B
    #define KNX_UART_TX_PIN 7                    // GPIO7 | SPI1 RX  | UART1 TX  | I2C0 SDA | PWM4 A

    // Possible pins for the OpenKNXiao RP2040 V1
    #define KNXIAO_RP2040_PIN1 1  // GPIO1  | SPI0 CSn | UART1 RTS | I2C1 SCL | PWM1 A
    #define KNXIAO_RP2040_PIN2 2  // GPIO2  | SPI0 TX  | UART1 RTS | I2C1 SDA | PWM1 B
    #define KNXIAO_RP2040_PIN3 3  // GPIO3  | SPI0 RX  | UART1 TX  | I2C1 SCL | PWM2 B
    #define KNXIAO_RP2040_PIN4 4  // GPIO4  | SPI0 RX  | UART1 TX  | I2C0 SDA | PWM2 A
    #define KNXIAO_RP2040_PIN5 5  // GPIO5  | SPI0 TX  | UART1 RX  | I2C0 SCL | PWM3 A
    #define KNXIAO_RP2040_PIN6 8  // GPIO8  | SPI1 RX  | UART1 TX  | I2C0 SDA | PWM4 A
    #define KNXIAO_RP2040_PIN7 9  // GPIO9  | SPI1 TX  | UART1 RX  | I2C0 SCL | PWM4 B
    #define KNXIAO_RP2040_PIN8 10 // GPIO10 | SPI1 SCK | UART1 CTS | I2C1 SDA | PWM5 A
#endif

// OpenKNXiao RP2040 V1 Terminal Connections
#ifdef OKNXHW_OPENKNXIAO_RP2040_V1_TERMINAL
    #define KNXIAO_RP2040_TERM_PIN1 KNXIAO_RP2040_PIN1 // GPIO1  | SPI0 CSn | UART1 RTS | I2C1 SCL | PWM1 A
    #define KNXIAO_RP2040_TERM_PIN2 KNXIAO_RP2040_PIN2 // GPIO2  | SPI0 TX  | UART1 RTS | I2C1 SDA | PWM1 B
    #define KNXIAO_RP2040_TERM_PIN3 KNXIAO_RP2040_PIN3 // GPIO3  | SPI0 RX  | UART1 TX  | I2C1 SCL | PWM2 B

    #define KNXIAO_RP2040_TERM_PIN6 KNXIAO_RP2040_PIN6 // GPIO8  | SPI1 RX  | UART1 TX  | I2C0 SDA | PWM4 A
    #define KNXIAO_RP2040_TERM_PIN7 KNXIAO_RP2040_PIN7 // GPIO9  | SPI1 TX  | UART1 RX  | I2C0 SCL | PWM4 B
    #define KNXIAO_RP2040_TERM_PIN8 KNXIAO_RP2040_PIN8 // GPIO10 | SPI1 SCK | UART1 CTS | I2C1 SDA | PWM5 A
#endif

// OpenKNXiao RP2040 V1 Terminal Mini Connections
#ifdef OKNXHW_OPENKNXIAO_RP2040_V1_TERMINAL_MINI
    #define KNXIAO_RP2040_TERM_PIN4 KNXIAO_RP2040_PIN4 // GPIO4  | SPI0 RX  | UART1 TX  | I2C0 SDA | PWM2 A
    #define KNXIAO_RP2040_TERM_PIN5 KNXIAO_RP2040_PIN5 // GPIO5  | SPI0 TX  | UART1 RX  | I2C0 SCL | PWM3 A
#endif

// OpenKNXiao RP2040 V1 Terminal Isolated Connections
#ifdef OKNXHW_OPENKNXIAO_RP2040_TERMINAL_NEOPIXEL
    #define KNXIAO_RP2040_TERM_PIN_NEOPIN1 KNXIAO_RP2040_PIN4 // GPIO4  | SPI0 RX  | UART1 TX  | I2C0 SDA | PWM2 A
    #define KNXIAO_RP2040_TERM_PIN_NEOPIN2 KNXIAO_RP2040_PIN5 // GPIO5  | SPI0 TX  | UART1 RX  | I2C0 SCL | PWM3 A
#endif

/**
 * Section: Hardware specific Pin Definitions
 * OpenKNXiao SAMD21
 */

// OpenKNXiao SAMD V1 LED1
#ifdef OKNXHW_OPENKNXIAO_SAMD21_V1_LED1
    #define INFO1_LED_PIN 11 // Build-In 1st LED BLUE
    #define INFO1_LED_PIN_ACTIVE_ON HIGH
#endif

// OpenKNXiao SAMD V1 LED2
#ifdef OKNXHW_OPENKNXIAO_SAMD21_V1_LED2
    #define INFO2_LED_PIN 12 // Build-In 2nd LED also BLUE
    #define INFO2_LED_PIN_ACTIVE_ON HIGH
#endif

/**
 * Section: Common Hardware (CHW) Pin Definitions
 * OpenKNXiao SAMD21
 */

// OpenKNXiao SAMD21 CHW Pins
#ifdef OKNXHW_OPENKNXIAO_SAMD21_V1_COMMON
    #define PROG_LED_PIN 13                      // Build-In LED YELLOW
    #define PROG_LED_PIN_ACTIVE_ON HIGH          // Active on HIGH
    #define PROG_BUTTON_PIN 0                    // GPIO0 | UART0 RX | I2C1 SDA | PWM0 A
    #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING // Interrupt on FALLING
    #define KNX_SERIAL Serial1                   // Serial1 for UART1
    #define KNX_UART_RX_PIN 6                    // GPIO6 | SPI1 TX  | UART1 RX  | I2C0 SCL | PWM4 B
    #define KNX_UART_TX_PIN 7                    // GPIO7 | SPI1 RX  | UART1 TX  | I2C0 SDA | PWM4 A

    // Possible pins for the OpenKNXiao SAMD21 V1
    #define KNXIAO_SAMD21_PIN1 1  // GPIO1  | SPI0 CSn | UART1 RTS | I2C1 SCL | PWM1 A
    #define KNXIAO_SAMD21_PIN2 2  // GPIO2  | SPI0 TX  | UART1 RTS | I2C1 SDA | PWM1 B
    #define KNXIAO_SAMD21_PIN3 3  // GPIO3  | SPI0 RX  | UART1 TX  | I2C1 SCL | PWM2 B
    #define KNXIAO_SAMD21_PIN4 4  // GPIO4  | SPI0 RX  | UART1 TX  | I2C0 SDA | PWM2 A
    #define KNXIAO_SAMD21_PIN5 5  // GPIO5  | SPI0 TX  | UART1 RX  | I2C0 SCL | PWM3 A
    #define KNXIAO_SAMD21_PIN6 8  // GPIO8  | SPI1 RX  | UART1 TX  | I2C0 SDA | PWM4 A
    #define KNXIAO_SAMD21_PIN7 9  // GPIO9  | SPI1 TX  | UART1 RX  | I2C0 SCL | PWM4 B
    #define KNXIAO_SAMD21_PIN8 10 // GPIO10 | SPI1 SCK | UART1 CTS | I2C1 SDA | PWM5 A
#endif

// OpenKNXiao SAMD21 V1 Terminal Connections
#ifdef OKNXHW_OPENKNXIAO_SAMD21_V1_TERMINAL
    #define KNXIAO_SAMD21_TERM_PIN1 KNXIAO_SAMD21_PIN1 // GPIO1  | SPI0 CSn | UART1 RTS | I2C1 SCL | PWM1 A
    #define KNXIAO_SAMD21_TERM_PIN2 KNXIAO_SAMD21_PIN2 // GPIO2  | SPI0 TX  | UART1 RTS | I2C1 SDA | PWM1 B
    #define KNXIAO_SAMD21_TERM_PIN3 KNXIAO_SAMD21_PIN3 // GPIO3  | SPI0 RX  | UART1 TX  | I2C1 SCL | PWM2 B

    #define KNXIAO_SAMD21_TERM_PIN6 KNXIAO_SAMD21_PIN6 // GPIO8  | SPI1 RX  | UART1 TX  | I2C0 SDA | PWM4 A
    #define KNXIAO_SAMD21_TERM_PIN7 KNXIAO_SAMD21_PIN7 // GPIO9  | SPI1 TX  | UART1 RX  | I2C0 SCL | PWM4 B
    #define KNXIAO_SAMD21_TERM_PIN8 KNXIAO_SAMD21_PIN8 // GPIO10 | SPI1 SCK | UART1 CTS | I2C1 SDA | PWM5 A
#endif

// OpenKNXiao SAMD21 V1 Terminal Mini Connections
#ifdef OKNXHW_OPENKNXIAO_SAMD21_TERMINAL_MINI
    #define KNXIAO_SAMD21_TERM_PIN4 KNXIAO_SAMD21_PIN4 // GPIO4  | SPI0 RX  | UART1 TX  | I2C0 SDA | PWM2 A
    #define KNXIAO_SAMD21_TERM_PIN5 KNXIAO_SAMD21_PIN5 // GPIO5  | SPI0 TX  | UART1 RX  | I2C0 SCL | PWM3 A
#endif