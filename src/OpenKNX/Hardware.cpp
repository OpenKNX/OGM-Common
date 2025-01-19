#include "OpenKNX/Facade.h"

#if OPENKNX_LITTLE_FS
    #include "LittleFS.h"
#endif

#if defined(USE_TP_RX_QUEUE) && (MASK_VERSION == 0x07B0 || MASK_VERSION == 0x091A)
    #if defined(ARDUINO_ARCH_RP2040) && defined(USE_KNX_DMA_UART) && defined(USE_KNX_DMA_IRQ)
void __time_critical_func(processKnxRxISR)()
{
    uart_get_hw(KNX_DMA_UART)->icr = UART_UARTICR_RTIC_BITS | UART_UARTICR_RXIC_BITS;
        #if MASK_VERSION == 0x07B0
    knx.bau().getDataLinkLayer()->processRxISR();
        #elif MASK_VERSION == 0x091A
    knx.bau().getSecondaryDataLinkLayer()->processRxISR();
        #endif
}
    #endif
    #if defined(ARDUINO_ARCH_ESP32)
void IRAM_ATTR processKnxRxTimer(void* pvParameters)
{
    const TickType_t xFrequency = pdMS_TO_TICKS(1);
    // if (!knx.platform().uartAvailable()) return;
    while (1)
    {
        #if MASK_VERSION == 0x091A
        knx.bau().getSecondaryDataLinkLayer()->processRxISR();
        #else
        knx.bau().getDataLinkLayer()->processRxISR();
        #endif
        vTaskDelay(xFrequency);
    }
}
    #endif
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

    void Hardware::initLeds()
    {
#ifdef OPENKNX_SERIALLED_ENABLE
    #ifndef PROG_LED_COLOR
        #define PROG_LED_COLOR 63, 0, 0
    #endif
        openknx.ledManager.init(OPENKNX_SERIALLED_PIN, 0, OPENKNX_SERIALLED_NUM);
        openknx.progLed.init(PROG_LED_PIN, &(openknx.ledManager), PROG_LED_COLOR);

    #ifdef INFO1_LED_PIN
        #ifndef INFO1_LED_COLOR
            #define INFO1_LED_COLOR 0, 63, 0
        #endif
        openknx.info1Led.init(INFO1_LED_PIN, &(openknx.ledManager), INFO1_LED_COLOR);
    #endif
    #ifdef INFO2_LED_PIN
        #ifndef INFO2_LED_COLOR
            #define INFO2_LED_COLOR 0, 63, 0
        #endif
        openknx.info2Led.init(INFO2_LED_PIN, &(openknx.ledManager), INFO2_LED_COLOR);
    #endif
    #ifdef INFO3_LED_PIN
        #ifndef INFO3_LED_COLOR
            #define INFO3_LED_COLOR 0, 63, 0
        #endif
        openknx.info3Led.init(INFO3_LED_PIN, &(openknx.ledManager), INFO3_LED_COLOR);
    #endif

#else

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
#if defined(USE_TP_RX_QUEUE) && (MASK_VERSION == 0x07B0 || MASK_VERSION == 0x091A)
    #if defined(ARDUINO_ARCH_RP2040) && defined(USE_KNX_DMA_UART) && defined(USE_KNX_DMA_IRQ)
        irq_set_exclusive_handler(KNX_DMA_UART_IRQ, processKnxRxISR);
        irq_set_enabled(KNX_DMA_UART_IRQ, true);
        uart_set_irq_enables(KNX_DMA_UART, true, false);
    #endif

    #ifdef ARDUINO_ARCH_ESP32
        // TimerHandle_t xTimer = xTimerCreate(
        //     "processKnxRx",   // Name des Timers
        //     pdMS_TO_TICKS(1), // Timer-Periode
        //     pdTRUE,           // Wiederholender Timer
        //     (void*)0,         // Benutzerdefinierter Parameter (optional)
        //     processKnxRxTimer // Callback-Funktion
        // );
        // if (xTimerStart(xTimer, 0) != pdPASS)
        // {
        //     logError("Hardware<KnxRx>", "Could not start timer!");
        //     return;
        // }
        xTaskCreatePinnedToCore(
            processKnxRxTimer,         // Task-Funktion
            "KnxRx",                   // Name des Tasks
            2048,                      // Stack-Größe in Worten
            NULL,                      // Parameter
            configMAX_PRIORITIES - 10, // Höchste Priorität
            NULL,                      // Task-Handle (optional)
            0                          // Core 0 auswählen
        );
    #endif
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
