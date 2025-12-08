#include "OpenKNX/Facade.h"

#if OPENKNX_LITTLE_FS
    #include "LittleFS.h"
#endif

namespace OpenKNX
{
    void Hardware::init()
    {
        // initFlash();

#ifdef ARDUINO_ARCH_RP2040
        adc_init();
        adc_set_temp_sensor_enabled(true);
#endif
#if OPENKNX_LITTLE_FS
        initFilesystem();
#endif
    }

    void Hardware::initButtons()
    {
#define ATTACH_BUTTON_INTERRUPT(PIN, MODE, BUTTON)                  \
    openknx.gpio.pinMode(PIN, MODE);                                \
    openknx.gpio.attachInterrupt(                                   \
        digitalPinToInterrupt(PIN),                                 \
        [this](openknx_gpio_number_t pin, bool state) -> void {     \
            BUTTON.change((MODE == INPUT_PULLUP) ? !state : state); \
        },                                                          \
        CHANGE); // Interrupt on change only, since we will detect short, long and double or n clicks

#ifdef PROG_BUTTON_PIN

    #ifndef PROG_BUTTON_PIN_MODE
        #define PROG_BUTTON_PIN_MODE INPUT_PULLUP
    #endif
        ATTACH_BUTTON_INTERRUPT(PROG_BUTTON_PIN, PROG_BUTTON_PIN_MODE, openknx.progButton);
#endif // PROG_BUTTON_PIN

#ifdef FUNC1_BUTTON_PIN
    #ifndef FUNC1_BUTTON_MODE
        #define FUNC1_BUTTON_MODE INPUT_PULLUP
    #endif
        ATTACH_BUTTON_INTERRUPT(FUNC1_BUTTON_PIN, FUNC1_BUTTON_MODE, openknx.func1Button);
#endif // FUNC1_BUTTON_PIN

#ifdef FUNC2_BUTTON_PIN
    #ifndef FUNC2_BUTTON_MODE
        #define FUNC2_BUTTON_MODE INPUT_PULLUP
    #endif
        ATTACH_BUTTON_INTERRUPT(FUNC2_BUTTON_PIN, FUNC2_BUTTON_MODE, openknx.func2Button);
#endif // FUNC2_BUTTON_PIN

#ifdef FUNC3_BUTTON_PIN
    #ifndef FUNC3_BUTTON_MODE
        #define FUNC3_BUTTON_MODE INPUT_PULLUP
    #endif
        ATTACH_BUTTON_INTERRUPT(FUNC3_BUTTON_PIN, FUNC3_BUTTON_MODE, openknx.func3Button);
#endif // FUNC3_BUTTON_PIN
    }

    void Hardware::initFlash()
    {
        logDebug("Hardware<Flash>", "Initialize flash");
        logIndentUp();
#ifdef ARDUINO_ARCH_ESP32
        openknx.openknxFlash.init("openknx");
        openknx.knxFlash.init("knx");
#else
        openknx.openknxFlash.init("openknx", OPENKNX_FLASH_OFFSET, OPENKNX_FLASH_SIZE);
        openknx.knxFlash.init("knx", KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
#endif

#ifdef KNX_FLASH_CALLBACK
        // register callbacks
        knx.platform().registerFlashCallbacks(
            []() -> uint32_t {
                // Size
                return openknx.knxFlash.size();
            },
            []() -> uint8_t* {
                // Read
                return openknx.knxFlash.flashAddress();
            },
            [](uint32_t relativeAddress, uint8_t* buffer, size_t len) -> uint32_t {
                // Write
                return openknx.knxFlash.write(relativeAddress, buffer, len);
            },
            []() -> void {
                // Commit
                return openknx.knxFlash.commit();
            }

        );
#endif
        logIndentDown();
    }

#if OPENKNX_LITTLE_FS
    void Hardware::initFilesystem()
    {

    #if defined(ARDUINO_ARCH_RP2040)
        // normal
        LittleFSConfig cfg;
        // Default is already auto format
        cfg.setAutoFormat(true);
        LittleFS.setConfig(cfg);
        if (!LittleFS.begin())
    #else
        if (!LittleFS.begin(true))
    #endif
        {
            fatalError(FATAL_INIT_FILESYSTEM, "Unable to initalize filesystem");
        }
    }
#endif

    void Hardware::fatalError(uint8_t code, const char* message)
    {
        logError("FatalError", "Code: %d (%s)", code, message);
        logIndentUp();
        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_STATE)->color(Led::Color::Red);
        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_STATE)->off(Led::Capability::MONOCHROME);
        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_STATE)->on(Led::Capability::COLOR);
        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_PROG)->color(Led::Color::Red);
        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_PROG)->errorCode(code);

#if MASK_VERSION == 0x07B0
        TpUartDataLinkLayer* dll = knx.bau().getDataLinkLayer();
        dll->stop(true);
        dll->powerControl(false);
#endif

#if defined(SAVE_POWER_PIN) && SAVE_POWER_PIN >= 0
            logInfo("FatalError", "Shut off aux power with pin %i", SAVE_POWER_PIN);
            openknx.gpio.digitalWrite(SAVE_POWER_PIN, SAVE_POWER_PIN_POWER_OFF);
#endif

        logIndentDown();

#ifdef OPENKNX_WATCHDOG
        openknx.watchdog.deactivate();
#endif
        while (true)
        {
            delay(2000);
            // Repeat error message
            logError("FatalError", "Code: %d (%s)", code, message);
        }
    }

    float Hardware::cpuTemperature()
    {
#if defined(ARDUINO_ARCH_RP2040)
        return analogReadTemp();
#else
        return 0.0f;
#endif
    }

#if MASK_VERSION == 0x07B0 or MASK_VERSION == 0x091A
    void Hardware::initKnxInterface()
    {
    #if defined(ARDUINO_ARCH_ESP32) && defined(KNX_UART_RX_PIN) && defined(KNX_UART_TX_PIN) && defined(KNX_UART_NUM)
        #if KNX_UART_NUM == 0
            #define KNX_UART UART_NUM_0
        #elif KNX_UART_NUM == 1
            #define KNX_UART UART_NUM_1
        #elif KNX_UART_NUM == 2
            #define KNX_UART UART_NUM_2
        #elif KNX_UART_NUM == 3
            #define KNX_UART UART_NUM_3
        #elif KNX_UART_NUM == 4
            #define KNX_UART UART_NUM_4
        #else
            #pragma error "Invalid KNX_UART_NUM defined"
        #endif
        knx.platform().interface(new TPUart::Interface::ESP32(KNX_UART_RX_PIN, KNX_UART_TX_PIN, KNX_UART));
    #elif defined(ARDUINO_ARCH_RP2040) && defined(KNX_UART_RX_PIN) && defined(KNX_UART_TX_PIN) && defined(KNX_UART_NUM)
        #if KNX_UART_NUM == 0
            #define KNX_UART uart0
        #elif KNX_UART_NUM == 1
            #define KNX_UART uart1
        #else
            #pragma error "Invalid KNX_UART_NUM defined"
        #endif
        knx.platform().interface(new TPUart::Interface::RP2040(KNX_UART_RX_PIN, KNX_UART_TX_PIN, KNX_UART, true, true));
    #else
        #pragma GCC error "No valid KNX UART interface defined (KNX_UART_NUM, KNX_UART_RX_PIN, KNX_UART_TX_PIN)"
    #endif
    #if MASK_VERSION == 0x091A
        knx.bau().getSecondaryDataLinkLayer()->getTPUart().registerReceivedFrame(
    #else
        knx.bau().getDataLinkLayer()->getTPUart().registerReceivedFrame(
    #endif
            [](TPUart::Frame& tpFrame) {
                // Process received frame
                if (openknx.console.bcuDebug())
                    openknx.logger.logWithPrefixAndValues("BCU<Debug>", "Received frame: %s", tpFrame.printFrame().c_str());
            });
    }
#endif

} // namespace OpenKNX
