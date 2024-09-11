// This file defines hardware properties of several reusable OpenKNX hardware
// It is meant to be included in the project-specific xyzHardware.h


// REG1-Eth
// https://github.com/OpenKNX/OpenKNX/wiki/REG1-Eth

    #ifdef OKNXHW_REG1_ETH
        #define OKNXHW_REG1_CONTROLLER2040

        #define ETH_SPI_INTERFACE SPI1 // SPI or SPI1, depends on the pins
        #define PIN_ETH_MISO (28)
        #define PIN_ETH_MOSI (27)
        #define PIN_ETH_SCK (26)
        #define PIN_ETH_SS (29)

        #define PIN_SD_SS (16)
        #define PIN_ETH_INT (17)
        #define PIN_ETH_RES (18)
    #endif

    #ifdef OKNXHW_REG1_ETH_V1
        #define OKNXHW_REG1_BASE_V1

        #define ETH_SPI_INTERFACE SPI1 // SPI or SPI1, depends on the pins
        #define PIN_ETH_MISO (28)
        #define PIN_ETH_MOSI (27)
        #define PIN_ETH_SCK (26)
        #define PIN_ETH_SS (29)

        #define PIN_SD_SS (16)
        #define PIN_ETH_INT (17)
        #define PIN_ETH_RES (18)

    #endif

// REG1-Base
// https://github.com/OpenKNX/OpenKNX/wiki/REG1-Base
    #ifdef OKNXHW_REG1_BASE_V1
        #define OKNXHW_REG1_CONTROLLER2040_V1

        #define INFO1_LED_PIN 6
        #define INFO1_LED_PIN_ACTIVE_ON HIGH

        #define INFO2_LED_PIN 24
        #define INFO2_LED_PIN_ACTIVE_ON HIGH

        #define INFO3_LED_PIN 22
        #define INFO3_LED_PIN_ACTIVE_ON HIGH

        #define FUNC1_BUTTON_PIN 7

    #endif

    #ifdef OKNXHW_REG1_BASE_V0
        #define OKNXHW_REG1_CONTROLLER2040
    #endif

// REG1-Base
// https://github.com/OpenKNX/OpenKNX/wiki/REG1-Base-IP
    #ifdef OKNXHW_REG1_BASE_IP
        #define OKNXHW_REG1_IPCONTROLLER2040

        #define PROG_LED_PIN 25
        #define PROG_LED_PIN_ACTIVE_ON HIGH
        #define PROG_BUTTON_PIN 23
        #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING

        #define INFO1_LED_PIN 6
        #define INFO1_LED_PIN_ACTIVE_ON HIGH

        #define INFO2_LED_PIN 24
        #define INFO2_LED_PIN_ACTIVE_ON HIGH

        #define INFO3_LED_PIN 22
        #define INFO3_LED_PIN_ACTIVE_ON HIGH

        #define FUNC1_BUTTON_PIN 7

    #endif

// SEN-REG1-Multi
// https://github.com/OpenKNX/OpenKNX/wiki/SEN-REG1-Multi
    #ifdef OKNXHW_SEN_REG1_MULTI
        #define HARDWARE_NAME "OpenKNX-SEN-REG1-Multi"
        #define OKNXHW_REG1_CONTROLLER2040_V1

        //#define INFO1_LED_PIN 6
        //#define INFO1_LED_PIN_ACTIVE_ON HIGH

        //#define INFO2_LED_PIN 24
        //#define INFO2_LED_PIN_ACTIVE_ON HIGH

        //#define INFO3_LED_PIN 22
        //#define INFO3_LED_PIN_ACTIVE_ON HIGH

        //#define FUNC1_BUTTON_PIN 7

        #define OKNXHW_REG1_APP_SEN_MULTI

    #endif

// SA-REG1-4xSELV
// https://github.com/OpenKNX/OpenKNX/wiki/SA-REG1-4xSELV
    #ifdef OKNXHW_SA_REG1_4XSELV
        #define HARDWARE_NAME "OpenKNX-SA-REG1-4xSELV"
        #define OKNXHW_REG1_CONTROLLER2040_V1

        #define OKNXHW_REG1_APP_SA_4XSELV

        //#define INFO1_LED_PIN 6
        //#define INFO1_LED_PIN_ACTIVE_ON HIGH

        //#define INFO2_LED_PIN 24
        //#define INFO2_LED_PIN_ACTIVE_ON HIGH

        //#define INFO3_LED_PIN 22
        //#define INFO3_LED_PIN_ACTIVE_ON HIGH

        //#define FUNC1_BUTTON_PIN 7
    #endif

// PiPico-BCU-Connector
// https://github.com/OpenKNX/OpenKNX/wiki/PiPico-BCU-Connector

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

// REG1-Controller2040
// https://github.com/OpenKNX/OpenKNX/wiki/REG1-Controller2040

    #ifdef OKNXHW_REG1_CONTROLLER2040 // V00.01 - V00.89
        #define PROG_LED_PIN 2
        #define PROG_LED_PIN_ACTIVE_ON HIGH
        #define PROG_BUTTON_PIN 7
        #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
        #define SAVE_INTERRUPT_PIN 6
        #define INFO_LED_PIN 3
        #define INFO_LED_PIN_ACTIVE_ON HIGH
        #define KNX_SERIAL Serial1
        #define KNX_UART_RX_PIN 1
        #define KNX_UART_TX_PIN 0
    #endif

    #ifdef OKNXHW_REG1_CONTROLLER2040_V1 // V00.90 - V01.89
        #ifndef HARDWARE_NAME
            #define HARDWARE_NAME "OpenKNX-REG1-Controller2040-V1"
        #endif
        #define PROG_LED_PIN 25
        #define PROG_LED_PIN_ACTIVE_ON HIGH
        #define PROG_BUTTON_PIN 23
        #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
        #define SAVE_INTERRUPT_PIN 3
        #define KNX_SERIAL Serial1
        #define KNX_UART_RX_PIN 1
        #define KNX_UART_TX_PIN 0

        #define REG1_APP_PIN1 29
        #define REG1_APP_PIN2 28
        #define REG1_APP_PIN3 27
        #define REG1_APP_PIN4 26
        #define REG1_APP_PIN5 18
        #define REG1_APP_PIN6 17
        #define REG1_APP_PIN7 16
    #endif

// REG1-IpController2040
// https://github.com/OpenKNX/OpenKNX/wiki/REG1-IpController2040

    #ifdef OKNXHW_REG1_IPCONTROLLER2040
    // #define PROG_LED_PIN 2
    // #define PROG_LED_PIN_ACTIVE_ON HIGH
    // #define PROG_BUTTON_PIN 7
    // #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
    // #define SAVE_INTERRUPT_PIN 6
    // #define INFO_LED_PIN 3
    // #define INFO_LED_PIN_ACTIVE_ON HIGH
    // #define KNX_UART_RX_PIN -1
    // #define KNX_UART_TX_PIN -1

        #define ETH_SPI_INTERFACE SPI // SPI or SPI1, depends on the pins
        #define PIN_ETH_MISO (0)
        #define PIN_ETH_MOSI (3)
        #define PIN_ETH_SCK (2)
        #define PIN_ETH_SS (1)
        #define PIN_ETH_INT (5)
        #define PIN_ETH_RES (4)
    #endif

// REG1-ControllerESP
// https://github.com/OpenKNX/OpenKNX/wiki/REG1-ControllerESP
     #ifdef OKNXHW_REG1_CONTROLLERESP
        #ifndef HARDWARE_NAME
            #define HARDWARE_NAME "OpenKNX-REG1-ControllerESP"
        #endif

        #ifdef USE_RGBLED
            #define RGBLED_PIN 4
            #define RGBLED_NUM 4
            #define PROG_LED_PIN 0
            #define INFO1_LED_PIN 1
            #define INFO2_LED_PIN 2
            #define INFO3_LED_PIN 3
        #else
            #define PROG_LED_PIN 4
            #define PROG_LED_PIN_ACTIVE_ON HIGH
        #endif


        #define PROG_BUTTON_PIN 39
        #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING

        #define SAVE_INTERRUPT_PIN 36
        #define KNX_UART_RX_PIN 37
        #define KNX_UART_TX_PIN 14

        #define FUNC1_BUTTON_PIN 38
        #define FUNC2_BUTTON_PIN 34
        #define FUNC3_BUTTON_PIN 35

        #define ETH_PHY_TYPE  ETH_PHY_LAN8720   // type of PHY used, needed for IDF
        #define ETH_PHY_ADDR  0                 // PHYs I2C address
        #define ETH_PHY_MDC   33                
        #define ETH_PHY_MDIO  32
        #define ETH_PHY_POWER 2                 // enable / disable the PHYs clock
        #define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN

        #define REG1_APP_PIN1 12
        #define REG1_APP_PIN2 15
        #define REG1_APP_PIN3 13
        #define REG1_APP_PIN4 5
        #define REG1_APP_PIN5 8
        #define REG1_APP_PIN6 7
        #define REG1_APP_PIN7 20

        // informative pin usage - Do not use for other purposes
        // RMII: 
        // ETH_PHY_CLK_INPUT_PIN 0
        // ETH_PHY_RXD0_PIN 25
        // ETH_PHY_RXD1_PIN 26
        // ETH_PHY_RX_DV_CRS_PIN 27
        // ETH_PHY_TX_EN_PIN 21
        // ETH_PHY_TXD0_PIN 19
        // ETH_PHY_TXD1_PIN 22
        //
        // USB:
        // USB_UART_TX_PIN 1
        // USB_UART_RX_PIN 3
    #endif

// REG1-App-SEN-Multi
// https://github.com/OpenKNX/OpenKNX/wiki/REG1-App-SEN-Multi
    #ifdef OKNXHW_REG1_APP_SEN_MULTI
        #define OKNXHW_REG1_APP_SEN_MULTI_SENSOR1_SDA_TX_PIN  REG1_APP_PIN2
        #define OKNXHW_REG1_APP_SEN_MULTI_SENSOR1_SCL_RX_PIN  REG1_APP_PIN1
        #define OKNXHW_REG1_APP_SEN_MULTI_SENSOR2_SDA_TX_PIN  REG1_APP_PIN4
        #define OKNXHW_REG1_APP_SEN_MULTI_SENSOR2_SCL_RX_PIN  REG1_APP_PIN3
        #define OKNXHW_REG1_APP_SEN_MULTI_BINARY_INPUT1_PIN   REG1_APP_PIN7
        #define OKNXHW_REG1_APP_SEN_MULTI_BINARY_INPUT2_PIN   REG1_APP_PIN6
        #define OKNXHW_REG1_APP_SEN_MULTI_BINARY_INPUT3_PIN   REG1_APP_PIN5
        #define OKNXHW_REG1_APP_SEN_MULTI_BINARY_INPUT_ONLEVEL  HIGH
    #endif

// REG1-App-GW-RF2G4
// https://github.com/OpenKNX/OpenKNX/wiki/REG1-App-GW-RF2G4
    #ifdef OKNXHW_REG1_APP_GW_RF2G4
        #define OKNXHW_REG1_APP_GW_RF2G4_CS       REG1_APP_PIN1
        #define OKNXHW_REG1_APP_GW_RF2G4_MISO     REG1_APP_PIN2
        #define OKNXHW_REG1_APP_GW_RF2G4_MOSI     REG1_APP_PIN3
        #define OKNXHW_REG1_APP_GW_RF2G4_CLK      REG1_APP_PIN4
        #define OKNXHW_REG1_APP_GW_RF2G4_INT      REG1_APP_PIN5
        #define OKNXHW_REG1_APP_GW_RF2G4_ENABLE   REG1_APP_PIN6
    #endif

// REG1-App-SA-4xSELV
// https://github.com/OpenKNX/OpenKNX/wiki/REG1-App-SA-4xSELV
    #ifdef OKNXHW_REG1_APP_SA_4XSELV
        #define OKNXHW_REG1_APP_SA_4xSELV_TCA_SDA   REG1_APP_PIN2
        #define OKNXHW_REG1_APP_SA_4xSELV_TCA_SCL   REG1_APP_PIN1
        #define OKNXHW_REG1_APP_SA_4xSELV_TCA_ADDR  0x20
        #define OKNXHW_REG1_APP_SA_4xSELV_TCA_RES   REG1_APP_PIN4
        #define OKNXHW_REG1_APP_SA_4xSELV_TCA_TYPE  OPENKNX_GPIO_T_TCA6408

        #define OPENKNX_SWA_SET_ACTIVE_ON HIGH
        #define OPENKNX_SWA_RESET_ACTIVE_ON HIGH
        #define OPENKNX_SWA_BISTABLE_IMPULSE_LENGTH 50
        #define V12_ADC_PIN REG1_APP_PIN3
    #endif

// UP1-Controller2040
// https://github.com/OpenKNX/OpenKNX/wiki/UP1-Controller2040

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

// REG2-Pi-Pico (Base) V1
// https://github.com/OpenKNX/OpenKNX-Pi-Pico-REG2
    #ifdef OKNXHW_REG2_PIPICO_W_ETH_V1
        #define OKNXHW_REG2_PIPICO_W_V1
        #define ETH_SPI_INTERFACE SPI // SPI or SPI1, depends on the pins
        #define PIN_ETH_MISO (16)     // ETH_MISO  - SPI0 RX UART0 TX I2C0 SDA PWM0_A SIO PIO0 PIO1
        #define PIN_ETH_SS (17)       // ETH_CS    - SPI0 CSn UART0 RX I2C0 SCL PWM0_B SIO PIO0 PIO1
        #define PIN_ETH_SCK (18)      // ETH_SCK   - SPI0 SCK UART0 CTS I2C1 SDA PWM1_A SIO PIO0 PIO1
        #define PIN_ETH_MOSI (19)     // ETH_MOSI  - SPI0 TX UART0 RTS I2C1 SCL PWM1_B SIO PIO0 PIO1
        #define PIN_ETH_INT (26)      // ETH_RES   - SPI1 SCK UART1 CTS I2C1 SDA PWM5_A SIO PIO0 PIO1
        #define PIN_ETH_RES (27)      // ETH_INT   - SPI1 TX UART1 RTS I2C1 SCL PWM5_B SIO PIO0 PIO1
    #endif

    #ifdef OKNXHW_REG2_PIPICO_ETH_V1
        #define OKNXHW_REG2_PIPICO_V1
        #define ETH_SPI_INTERFACE SPI // SPI or SPI1, depends on the pins
        #define PIN_ETH_MISO (16)     // ETH_MISO  - SPI0 RX UART0 TX I2C0 SDA PWM0_A SIO PIO0 PIO1
        #define PIN_ETH_SS (17)       // ETH_CS    - SPI0 CSn UART0 RX I2C0 SCL PWM0_B SIO PIO0 PIO1
        #define PIN_ETH_SCK (18)      // ETH_SCK   - SPI0 SCK UART0 CTS I2C1 SDA PWM1_A SIO PIO0 PIO1
        #define PIN_ETH_MOSI (19)     // ETH_MOSI  - SPI0 TX UART0 RTS I2C1 SCL PWM1_B SIO PIO0 PIO1
        #define PIN_ETH_INT (26)      // ETH_RES   - SPI1 SCK UART1 CTS I2C1 SDA PWM5_A SIO PIO0 PIO1
        #define PIN_ETH_RES (27)      // ETH_INT   - SPI1 TX UART1 RTS I2C1 SCL PWM5_B SIO PIO0 PIO1
    #endif

    #ifdef OKNXHW_REG2_PIPICO_W_V1
        #define OKNXHW_REG2_PIPICO_V1_BASE
        #define INFO1_LED_PIN 32 // PiPicoW Onboard LED
        #define INFO1_LED_PIN_ACTIVE_ON HIGH
        #define INFO2_LED_PIN 3
        #define INFO2_LED_PIN_ACTIVE_ON HIGH
        #define SAVE_INTERRUPT_PIN 21
    #endif

    #ifdef OKNXHW_REG2_PIPICO_V1
        #define OKNXHW_REG2_PIPICO_V1_BASE
        #define INFO1_LED_PIN 25 // PiPico Onboard LED
        #define INFO1_LED_PIN_ACTIVE_ON HIGH
        #define INFO2_LED_PIN 3
        #define INFO2_LED_PIN_ACTIVE_ON HIGH
        #define SAVE_INTERRUPT_PIN 21
    #endif

    #ifdef OKNXHW_REG2_PIPICO_V1_BASE
        #define PROG_LED_PIN 2
        #define PROG_LED_PIN_ACTIVE_ON HIGH
        #define PROG_BUTTON_PIN 20
        #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
        #define KNX_SERIAL Serial1
        #define KNX_UART_RX_PIN 1
        #define KNX_UART_TX_PIN 0
    #endif

// OpenKNXiao (Base) V1
    #ifdef OKNXHW_OPENKNXIAO_KNEOPIX_V1
        #define OKNXHW_OPENKNXIAO_RP2040_BASE_V1
        #define INFO1_LED_PIN 16 // Build-In LED GREEN
        #define INFO1_LED_PIN_ACTIVE_ON HIGH
        #define INFO2_LED_PIN 25 // Build-In LED BLUE
        #define INFO2_LED_PIN_ACTIVE_ON HIGH
    #endif
    
    #ifdef OKNXHW_OPENKNXIAO_MINI_V1
        #define OKNXHW_OPENKNXIAO_RP2040_BASE_V1
        #define INFO1_LED_PIN 16 // Build-In LED GREEN
        #define INFO1_LED_PIN_ACTIVE_ON HIGH
        #define INFO2_LED_PIN 25 // Build-In LED BLUE
        #define INFO2_LED_PIN_ACTIVE_ON HIGH
    #endif

    #ifdef OKNXHW_OPENKNXIAO_RP2040_BASE_V1
        #define PROG_LED_PIN 12
        #define PROG_LED_PIN_ACTIVE_ON HIGH
        #define PROG_BUTTON_PIN 0
        #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING
        #define KNX_SERIAL Serial1
        #define KNX_UART_RX_PIN 6
        #define KNX_UART_TX_PIN 7
    #endif