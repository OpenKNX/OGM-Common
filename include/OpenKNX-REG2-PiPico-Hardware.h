#pragma once
/**
 * OpenKNX Hardware Definition Header File
 *
 * File: OpenKNX-REG2-PiPico-Hardware.h
 * Hardware: REG2-Pi-Pico Based Boards (V1)
 * Responsible: OpenKNX - Erkan Çolak
 *
 * Defines hardware IO pins and functionalities for OpenKNX REG2-Pi-Pico platform.
 * Includes pin assignments for LEDs, buttons, serial communication, I2C interfaces,
 * and other peripherals. Ensures compatibility with various application boards and
 * firmware features.
 *
 * Configurations are categorized by hardware versions and features:
 * - REG2-Pi-Pico ( Current Versions: V1 )
 * - Wi-Fi
 * - Ethernet
 * - Device Display, Meter support, RTC, WS2812, etc.
 *
 * Each configuration is guarded by preprocessor directives to enable/disable features.
 * More info about the Hardware visit: https://github.com/OpenKNX/OpenKNX-Pi-Pico-REG2
 *
 * Section: Product configurations / use cases
 *          Describes the available configurations for the OpenKNX REG2-Pi-Pico hardware.
 * Section: Firmware Features (FwF) based IO and Pin Definitions
 *          Defines the pin assignments for the firmware features.
 * Section: Hardware specific Pin Definitions
 *          Defines the pin assignments for the seperate hardware specific configurations.
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
 * OpenKNX REG2-Pi-Pico
 */

// REG2-Pi-Pico V1
#ifdef OKNXHW_REG2_PIPICO_V1
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-REG2-Pi-Pico-V1"
    #endif
    #ifndef PRODUCTION_NAME
        #define PRODUCTION_NAME "OpenKNX REG2 PiPico V1"
    #endif
    #define OKNXHW_REG2_PIPICO_V1_COMMON // Common pins for all REG2-Pi-Pico
    #define OKNXHW_REG2_PIPICO_V1_LED1   // LED1
    #define OKNXHW_REG2_PIPICO_V1_LED2   // LED2
    #define OKNXHW_REG2_PIPICO_V1_LED3   // LED3
#endif

// REG2-Pi-Pico V1 (Device Display + Meter)
#ifdef OKNXHW_REG2_PIPICO_V1_DD_METER
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-REG2-Pi-Pico-V1"
    #endif
    #ifndef PRODUCTION_NAME
        #define PRODUCTION_NAME "OpenKNX REG2 PiPico V1 - Device Display + Meter"
    #endif
    #define OKNXHW_REG2_PIPICO_V1_COMMON // Common pins for all REG2-Pi-Pico
    #define OKNXHW_REG2_PIPICO_V1_LED1   // LED1
    #define OKNXHW_REG2_PIPICO_V1_LED2   // LED2
    #define OKNXHW_REG2_PIPICO_V1_LED3   // LED3
    #define OKNXHW_REG2_DEVICE_DISPLAY   // Device Display Support
    #define OKNXHW_REG2_METER            // Meter Support
#endif

// REG2-Pi-Pico V1 ETH App
#ifdef OKNXHW_REG2_PIPICO_ETH_V1
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-REG2-Pi-Pico-Eth-V1"
    #endif
    #ifndef PRODUCTION_NAME
        #define PRODUCTION_NAME "OpenKNX REG2 PiPico V1 - Ethernet"
    #endif
    #define OKNXHW_REG2_PIPICO_V1
    #define OKNXHW_REG2_PIPICO_V1_COMMON
    #define OKNXHW_REG2_PIPICO_V1_LED1
    #define OKNXHW_REG2_PIPICO_V1_LED2
    #define OKNXHW_REG2_PIPICO_V1_LED3
    #define OKNXHW_REG2_PIPICO_APP_ETH
#endif

// REG2-Pi-Pico V1 ETH App (Device Display + Meter)
#ifdef OKNXHW_REG2_PIPICO_ETH_V1_DD_METER
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-REG2-Pi-Pico-Eth-V1"
    #endif
    #ifndef PRODUCTION_NAME
        #define PRODUCTION_NAME "OpenKNX REG2 PiPico V1 - Ethernet + Device Display + Meter"
    #endif
    #define OKNXHW_REG2_PIPICO_V1
    #define OKNXHW_REG2_PIPICO_V1_COMMON
    #define OKNXHW_REG2_PIPICO_V1_LED1
    #define OKNXHW_REG2_PIPICO_V1_LED2
    #define OKNXHW_REG2_PIPICO_V1_LED3
    #define OKNXHW_REG2_PIPICO_APP_ETH
    #define OKNXHW_REG2_DEVICE_DISPLAY
    #define OKNXHW_REG2_METER
#endif

/**
 * Section: Product configurations / use cases
 * OpenKNX REG2-Pi-Pico Wi-Fi
 */

// REG2-Pi-Pico Wi-Fi V1
#ifdef OKNXHW_REG2_PIPICO_W_V1
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-REG2-Pi-Pico-W-V1"
    #endif
    #ifndef PRODUCTION_NAME
        #define PRODUCTION_NAME "OpenKNX REG2 PiPico WiFi V1"
    #endif

    #define OKNXHW_REG2_PIPICO_V1_COMMON
    #define OKNXHW_REG2_PIPICO_W_V1_LED1
    #define OKNXHW_REG2_PIPICO_V1_LED2
    #define OKNXHW_REG2_PIPICO_V1_LED3
#endif

// REG2-Pi-Pico Wi-Fi V1 (Device Display + Meter)
#ifdef OKNXHW_REG2_PIPICO_W_V1_DD_METER
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-REG2-Pi-Pico-W-V1"
    #endif
    #ifndef PRODUCTION_NAME
        #define PRODUCTION_NAME "OpenKNX REG2 PiPico WiFi V1 - Device Display + Meter"
    #endif
    #define OKNXHW_REG2_PIPICO_V1_COMMON
    #define OKNXHW_REG2_PIPICO_W_V1_LED1
    #define OKNXHW_REG2_PIPICO_V1_LED2
    #define OKNXHW_REG2_PIPICO_V1_LED3
    #define OKNXHW_REG2_DEVICE_DISPLAY
    #define OKNXHW_REG2_METER
#endif

// REG2-Pi-Pico WiFi V1 ETH App
#ifdef OKNXHW_REG2_PIPICO_W_ETH_V1
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-REG2-Pi-Pico-W-Eth-V1"
    #endif
    #ifndef PRODUCTION_NAME
        #define PRODUCTION_NAME "OpenKNX REG2 PiPico WiFi V1 - Ethernet"
    #endif
    #define OKNXHW_REG2_PIPICO_W_V1
    #define OKNXHW_REG2_PIPICO_V1_COMMON
    #define OKNXHW_REG2_PIPICO_W_V1_LED1
    #define OKNXHW_REG2_PIPICO_V1_LED2
    #define OKNXHW_REG2_PIPICO_V1_LED3
    #define OKNXHW_REG2_PIPICO_APP_ETH
#endif

// REG2-Pi-Pico WiFi V1 ETH App (Device Display + Meter)
#ifdef OKNXHW_REG2_PIPICO_W_ETH_V1_DD_METER
    #ifndef HARDWARE_NAME
        #define HARDWARE_NAME "OpenKNX-REG2-Pi-Pico-W-Eth-V1"
    #endif
    #ifndef PRODUCTION_NAME
        #define PRODUCTION_NAME "OpenKNX REG2 PiPico WiFi V1 - Ethernet + Device Display + Meter"
    #endif
    #define OKNXHW_REG2_PIPICO_W_V1
    #define OKNXHW_REG2_PIPICO_V1_COMMON
    #define OKNXHW_REG2_PIPICO_W_V1_LED1
    #define OKNXHW_REG2_PIPICO_V1_LED2
    #define OKNXHW_REG2_PIPICO_V1_LED3
    #define OKNXHW_REG2_PIPICO_APP_ETH
    #define OKNXHW_REG2_DEVICE_DISPLAY
    #define OKNXHW_REG2_METER
#endif

/**
 * Section: Firmware Features (FwF) based IO and Pin Definitions
 * OpenKNX REG2-Pi-Pico
 */

// REG2-Pi-Pico FwF: Device Display Support
#ifdef OKNXHW_REG2_DEVICE_DISPLAY
    // Default pins for the I2C bus to connect the hardware display
    #define OKNXHW_REG2_HWDISPLAY_I2C_0_1 1  // 0: I2C0, 1: I2C1
    #define OKNXHW_REG2_HWDISPLAY_I2C_SDA 26 // GPIO26 | SPI1 SCK | UART0 CTS | I2C1 SDA | PWM5 A | ADC0
    #define OKNXHW_REG2_HWDISPLAY_I2C_SCL 27 // GPIO27 | SPI1 TX  | UART0 RX  | I2C0 SCL | PWM6 B | ADC1
#endif                                       // REG2-Pi-Pico FwF: Device Display Support

// REG2-Pi-Pico FwF: Meter Support
#ifdef OKNXHW_REG2_METER
    // Default pins for the Meter Sensors typ Rx/Tx
    #define OKNXHW_REG2_MSENS_1_SDA0_TX_PIN 4 // GPIO4  | SPI0 RX  | UART1 TX  | I2C0 SDA | PWM2 A
    #define OKNXHW_REG2_MSENS_1_SCL0_RX_PIN 5 // GPIO5  | SPI0 TX  | UART1 RX  | I2C0 SCL | PWM2 B
    #define OKNXHW_REG2_MSENS_2_SDA1_TX_PIN 6 // GPIO6  | SPI0 SCK | UART1 CTS | I2C1 SDA | PWM3 A
    #define OKNXHW_REG2_MSENS_2_SCL1_RX_PIN 7 // GPIO7  | SPI0 CSn | UART1 RTS | I2C1 SCL | PWM3 B
#endif                                        // REG2-Pi-Pico (Device Display + Meter)

/**
 * Section: Hardware specific Pin Definitions
 * OpenKNX REG2-Pi-Pico
 */

// REG2-Pi-Pico V1: Info1 LED
#ifdef OKNXHW_REG2_PIPICO_V1_LED1
    #define INFO1_LED_PIN 25 // PiPico Onboard LED
    #define INFO1_LED_PIN_ACTIVE_ON HIGH
#endif

// REG2-Pi-Pico Wifi V1: Info1 LED
#ifdef OKNXHW_REG2_PIPICO_W_V1_LED1
    #define INFO1_LED_PIN 32 // PiPicoW Onboard LED
    #define INFO1_LED_PIN_ACTIVE_ON HIGH
#endif

// REG2-Pi-Pico V1: Info2 LED
#ifdef OKNXHW_REG2_PIPICO_V1_LED2
    #define INFO2_LED_PIN 3
    #define INFO2_LED_PIN_ACTIVE_ON HIGH
#endif
// REG2-Pi-Pico V1: Info3 LED
#ifdef OKNXHW_REG2_PIPICO_V1_LED3
    #define INFO3_LED_PIN 21
    #define INFO3_LED_PIN_ACTIVE_ON HIGH
#endif

// REG2-Pi-Pico V1: Interrupt Pin
#ifdef OKNXHW_REG2_PIPICO_V1_SAVE_INTERRUPT
    #define SAVE_INTERRUPT_PIN 21
#endif

/**
 * Section: Common Hardware (CHW) Pin Definitions
 * REG2-Pi-Pico V1
 */
// REG2-Pi-Pico V1 CHW Pins
#ifdef OKNXHW_REG2_PIPICO_V1_COMMON
    #define PROG_LED_PIN 2
    #define PROG_LED_PIN_ACTIVE_ON HIGH
    #define PROG_BUTTON_PIN 20
    #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
    #define KNX_SERIAL Serial1
    #define KNX_UART_RX_PIN 1
    #define KNX_UART_TX_PIN 0

    // Application board
    #define REG2_APP_PIN1 19 // GPIO19 | SPI0 TX  | UART0 RTS | I2C1 SCL | PWM1 B
    #define REG2_APP_PIN2 28 // GPIO28 | SPI1 RX  | UART0 TX  | I2C0 SDA | PWM6 A | ADC2
    #define REG2_APP_PIN3 27 // GPIO27 | SPI1 TX  | UART0 RX  | I2C0 SCL | PWM6 B | ADC1
    #define REG2_APP_PIN4 26 // GPIO26 | SPI1 SCK | UART0 CTS | I2C1 SDA | PWM5 A | ADC0
    #define REG2_APP_PIN5 18 // GPIO18 | SPI0 SCK | UART0 CTS | I2C1 SDA | PWM1 A
    #define REG2_APP_PIN6 17 // GPIO17 | SPI0 CSn | UART0 RX  | I2C0 SCL | PWM0 B
    #define REG2_APP_PIN7 16 // GPIO16 | SPI0 RX  | UART0 TX  | I2C0 SDA | PWM0 A

    // Application board extended pins
    #define REG2_APP_PIN8 10  // GPIO10 | SPI1 SCK | UART1 CTS | I2C1 SDA | PWM5 A
    #define REG2_APP_PIN9 11  // GPIO11 | SPI1 TX  | UART1 RTS | I2C1 SCL | PWM7 B
    #define REG2_APP_PIN10 12 // GPIO12 | SPI1 RX  | UART0 TX  | I2C0 SDA | PWM6 A
    #define REG2_APP_PIN11 13 // GPIO13 | SPI1 RX  | UART0 TX  | I2C0 SDA | PWM6 B
    #define REG2_APP_PIN12 14 // GPIO14 | SPI1 SCK | UART0 CTS | I2C1 SDA | PWM7 A
    #define REG2_APP_PIN13 15 // GPIO15 | SPI1 TX  | UART0 RTS | I2C1 SCL | PWM7 B

    // Terminal board
    #define REG2_TERM_PIN10 22 // GPIO22 | SPI0 SCK | UART1 CTS | I2C1 SDA | PWM3 A
    #define REG2_TERM_PIN9 9   // GPIO9  | SPI1 TX  | UART1 RX  | I2C0 SCL | PWM4 B
    #define REG2_TERM_PIN8 8   // GPIO8  | SPI1 RX  | UART1 TX  | I2C0 SDA | PWM4 A
    #define REG2_TERM_PIN7 7   // GPIO7  | SPI0 CSn | UART1 RTS | I2C1 SCL | PWM3 B
    #define REG2_TERM_PIN6 6   // GPIO6  | SPI0 SCK | UART1 CTS | I2C1 SDA | PWM3 A
    #define REG2_TERM_PIN5 5   // GPIO5  | SPI0 TX  | UART1 RX  | I2C0 SCL | PWM2 B
    #define REG2_TERM_PIN4 4   // GPIO4  | SPI0 RX  | UART1 TX  | I2C0 SDA | PWM2 A

#endif

/**
 * Section: FwF and CHW based Pin Definitions
 * OpenKNX REG2-Pi-Pico
 */

// REG2-Pi-Pico (WiFi) ETH App and FwF
#if defined OKNXHW_REG2_PIPICO_APP_ETH
    #define ETH_SPI_INTERFACE SPI        // SPI or SPI1, depends on the pins
    #define PIN_ETH_MISO (REG2_APP_PIN7) // ETH_MISO - GPIO16 SPI0 RX UART0 TX I2C0 SDA PWM0_A SIO PIO0 PIO1
    #define PIN_ETH_SS (REG2_APP_PIN6)   // ETH_CS   - GPIO17 SPI0 CSn UART0 RX I2C0 SCL PWM0_B SIO PIO0 PIO1
    #define PIN_ETH_SCK (REG2_APP_PIN5)  // ETH_SCK  - GPIO18 SPI0 SCK UART0 CTS I2C1 SDA PWM1_A SIO PIO0 PIO1
    #define PIN_ETH_MOSI (REG2_APP_PIN1) // ETH_MOSI - GPIO19 SPI0 TX UART0 RTS I2C1 SCL PWM1_B SIO PIO0 PIO1
    #define PIN_ETH_INT (REG2_APP_PIN4)  // ETH_RES  - GPIO26 SPI1 SCK UART0 CTS I2C1 SDA PWM5_A SIO PIO0 PIO1
    #define PIN_ETH_RES (REG2_APP_PIN3)  // ETH_INT  - GPIO27 SPI1 TX UART0 RTS I2C1 SCL PWM5_B SIO PIO0 PIO1
#endif