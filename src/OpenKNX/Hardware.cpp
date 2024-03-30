#include "OpenKNX/Facade.h"

#ifdef ARDUINO_ARCH_RP2040
    #include "LittleFS.h"
#endif

#if defined(ARDUINO_ARCH_RP2040) && defined(USE_TP_RX_QUEUE) && defined(USE_KNX_DMA_UART) && defined(USE_KNX_DMA_IRQ)
void __time_critical_func(processKnxRxISR)()
{
    uart_get_hw(KNX_DMA_UART)->icr = UART_UARTICR_RTIC_BITS | UART_UARTICR_RXIC_BITS;
    knx.bau().getDataLinkLayer()->processRxISR();
}
// bool __time_critical_func(processKnxRxTimer)(repeating_timer *t)
// {
//   if(!knx.platform().uartAvailable()) return true;
//     knx.bau().getDataLinkLayer()->processRxISR();
//     return true;
// }
// struct repeating_timer repeatingTimer;
#endif

namespace OpenKNX
{
    void Hardware::init()
    {
        // initFlash();

#ifdef ARDUINO_ARCH_RP2040
        adc_init();
        adc_set_temp_sensor_enabled(true);

        initFilesystem();
#endif
    }

    void Hardware::initLeds()
    {
        openknx.progLed.init(PROG_LED_PIN, PROG_LED_PIN_ACTIVE_ON);
#ifdef INFO1_LED_PIN
        openknx.info1Led.init(INFO1_LED_PIN, INFO1_LED_PIN_ACTIVE_ON);
#endif
#ifdef INFO2_LED_PIN
        openknx.info2Led.init(INFO2_LED_PIN, INFO2_LED_PIN_ACTIVE_ON);
#endif
#ifdef INFO3_LED_PIN
        openknx.info3Led.init(INFO3_LED_PIN, INFO3_LED_PIN_ACTIVE_ON);
#endif
    }

    void Hardware::initButtons()
    {
#ifdef PROG_BUTTON_PIN
        pinMode(PROG_BUTTON_PIN, INPUT_PULLUP);
        attachInterrupt(
            digitalPinToInterrupt(PROG_BUTTON_PIN),
            []() -> void { openknx.progButton.change(!digitalRead(PROG_BUTTON_PIN)); }, CHANGE);
#endif

#ifdef FUNC1_BUTTON_PIN
        pinMode(FUNC1_BUTTON_PIN, INPUT_PULLUP);
        attachInterrupt(
            digitalPinToInterrupt(FUNC1_BUTTON_PIN),
            []() -> void { openknx.func1Button.change(!digitalRead(FUNC1_BUTTON_PIN)); }, CHANGE);
#endif

#ifdef FUNC2_BUTTON_PIN
        pinMode(FUNC2_BUTTON_PIN, INPUT_PULLUP);
        attachInterrupt(
            digitalPinToInterrupt(FUNC2_BUTTON_PIN),
            []() -> void { openknx.func2Button.change(!digitalRead(FUNC2_BUTTON_PIN)); }, CHANGE);
#endif

#ifdef FUNC3_BUTTON_PIN
        pinMode(FUNC3_BUTTON_PIN, INPUT_PULLUP);
        attachInterrupt(
            digitalPinToInterrupt(FUNC3_BUTTON_PIN),
            []() -> void { openknx.func3Button.change(!digitalRead(FUNC3_BUTTON_PIN)); }, CHANGE);
#endif
    }

    
    void Hardware::initKnxRxISR()
    {
#if defined(ARDUINO_ARCH_RP2040) && defined(USE_TP_RX_QUEUE) && defined(USE_KNX_DMA_UART) && defined(USE_KNX_DMA_IRQ)
    // alarm_pool_add_repeating_timer_ms(openknx.timerInterrupt.alarmPool(), -1, processKnxRxTimer, NULL, &repeatingTimer);
    irq_set_exclusive_handler(KNX_DMA_UART_IRQ, processKnxRxISR);
    irq_set_enabled(KNX_DMA_UART_IRQ, true);
    uart_set_irq_enables(KNX_DMA_UART, true, false);
#endif
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

#ifdef ARDUINO_ARCH_RP2040
    void Hardware::initFilesystem()
    {
        // normal
        LittleFSConfig cfg;
        // Default is already auto format
        cfg.setAutoFormat(true);
        LittleFS.setConfig(cfg);

        if (!LittleFS.begin())
        {
            fatalError(FATAL_INIT_FILESYSTEM, "Unable to initalize filesystem");
        }
    }
#endif

    void Hardware::fatalError(uint8_t code, const char* message)
    {
        logError("FatalError", "Code: %d (%s)", code, message);
        logIndentUp();
#ifdef INFO1_LED_PIN
        openknx.info1Led.on();
#endif
        openknx.progLed.errorCode(code);

#if MASK_VERSION == 0x07B0
        TpUartDataLinkLayer* ddl = knx.bau().getDataLinkLayer();
        ddl->stop(true);
    #ifdef NCN5120
        ddl->powerControl(false);
    #endif
#endif
        logIndentDown();

        while (true)
        {
#ifdef OPENKNX_WATCHDOG
            openknx.watchdog.deactivate();
#endif
            delay(2000);
            // Repeat error message
            logError("FatalError", "Code: %d (%s)", code, message);
        }
    }

    float Hardware::cpuTemperature()
    {
#if defined(ARDUINO_ARCH_RP2040)
        adc_select_input(4);
        const float conversionFactor = 3.3f / (1 << 12);
        float adc = (float)adc_read() * conversionFactor;
        return 27.0f - (adc - 0.706f) / 0.001721f; // Conversion from Datasheet
#else
        return 0.0f;
#endif
    }
} // namespace OpenKNX
