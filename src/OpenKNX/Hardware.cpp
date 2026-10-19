#include "OpenKNX/Facade.h"

#if OPENKNX_LITTLE_FS
    #include "LittleFS.h"
    #ifdef ARDUINO_ARCH_RP2040
        #include <pico_base/pico/ota_command.h> // OTACmdPage: the page the bootloader reads
    #endif
#endif

namespace OpenKNX
{
#if defined(ARDUINO_ARCH_ESP32) && (defined(KNX_FLASH_OFFSET) || defined(OPENKNX_FLASH_OFFSET))
    /**
     * @brief Logs an error when a flashed partition differs from the layout the firmware was built for.
     * @note OTA does not write the partition table, so the device keeps using the partition it has.
     */
    static void checkFlashPartition(const char *id, Flash::Driver &flash, uint32_t offset, uint32_t size)
    {
        if (flash.startOffset() == offset && flash.size() == size)
            return;
        logError("Hardware<Flash>", "%s partition 0x%08X/0x%X differs from build 0x%08X/0x%X, partition table not flashed?",
                 id, (unsigned)flash.startOffset(), (unsigned)flash.size(), (unsigned)offset, (unsigned)size);
    }
#endif

    void Hardware::init()
    {
        // initFlash();

#ifdef ARDUINO_ARCH_RP2040
        adc_init();
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
    #ifdef OPENKNX_FLASH_OFFSET
        checkFlashPartition("openknx", openknx.openknxFlash, OPENKNX_FLASH_OFFSET, OPENKNX_FLASH_SIZE);
    #endif
    #ifdef KNX_FLASH_OFFSET
        checkFlashPartition("knx", openknx.knxFlash, KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
    #endif
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
    #if defined(ARDUINO_ARCH_RP2040)
        reclaimStagedUpdate(); // before any module can open a file of the same name
    #endif
    }
#endif

#if OPENKNX_LITTLE_FS && defined(ARDUINO_ARCH_RP2040)
    static constexpr const char *OTA_STAGED_RECORD = "/ota.staged";
    static constexpr const char *OTA_COMMAND_FILE = "/otacommand.bin";

    void Hardware::noteStagedUpdate(const char *path)
    {
        LittleFS.remove(OTA_STAGED_RECORD);
        if (path == nullptr || path[0] == 0) return;

        // Only a complete, signed command page counts: the core's Updater ignores a failed commit, and a record
        // pointing at an old, already used page would release an image that was never applied.
        File cmd = LittleFS.open(OTA_COMMAND_FILE, "r");
        if (!cmd) return;
        // Sign, count and the first entry up to its file name: 80 bytes instead of the whole page on the stack.
        constexpr size_t nameAt = offsetof(OTACmdPage, cmd) + offsetof(commandEntry, write.filename);
        constexpr size_t nameLen = sizeof(commandEntry::write.filename);
        uint8_t head[nameAt + nameLen] = {};
        const bool valid = cmd.size() == sizeof(OTACmdPage) && cmd.read(head, sizeof(head)) == sizeof(head) &&
                           memcmp(head, "Pico OTA", 8) == 0 && strncmp((const char *)head + nameAt, path, nameLen) == 0;
        cmd.close();
        if (!valid) return;

        File rec = LittleFS.open(OTA_STAGED_RECORD, "w");
        if (!rec) return;
        rec.print(path);
        rec.close();
    }

    /**
     * @brief Give back an image once the bootloader has run the command that named it.
     * @details The record is written only after a complete commit; the bootloader erases the data block of
     *          otacommand.bin only after a complete copy. While the signature is there it retries at the next
     *          boot and needs the image, so nothing is touched.
     */
    void Hardware::reclaimStagedUpdate()
    {
        File rec = LittleFS.open(OTA_STAGED_RECORD, "r");
        if (!rec) return;
        char path[72] = {};
        const bool fits = rec.size() < sizeof(path);
        if (fits) rec.read((uint8_t *)path, sizeof(path) - 1);
        rec.close();
        if (!fits || path[0] == 0)
        {
            LittleFS.remove(OTA_STAGED_RECORD); // cannot name the image reliably: keep it, drop the record
            return;
        }

        File cmd = LittleFS.open(OTA_COMMAND_FILE, "r");
        if (!cmd)
        {
            LittleFS.remove(OTA_STAGED_RECORD);
            return;
        }
        if (cmd.size() != sizeof(OTACmdPage))
        {
            // The bootloader reads exactly one page, so it can never run this one: drop it, keep the image.
            cmd.close();
            LittleFS.remove(OTA_COMMAND_FILE);
            LittleFS.remove(OTA_STAGED_RECORD);
            return;
        }
        char sign[8] = {};
        const bool pending = cmd.read((uint8_t *)sign, sizeof(sign)) == sizeof(sign) && memcmp(sign, "Pico OTA", sizeof(sign)) == 0;
        cmd.close();
        if (pending) return;

        File img = LittleFS.open(path, "r");
        const uint32_t size = img ? (uint32_t)img.size() : 0;
        if (img) img.close();
        LittleFS.remove(path);
        LittleFS.remove(OTA_COMMAND_FILE);
        LittleFS.remove(OTA_STAGED_RECORD);
        logInfo("OTA", "Reclaimed %u bytes: the bootloader has run the update staged as %s", (unsigned)size, path);
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

#if defined(KNX_HAS_TP) && !defined(KNX_IS_ROUTER)
        TpUartDataLinkLayer* dll = KNX_TP_DLL;
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
#elif defined(ARDUINO_ARCH_ESP32)
        return temperatureRead(); // internal die-temperature sensor (rough, but a useful thermal indicator)
#else
        return 0.0f;
#endif
    }

#ifdef KNX_HAS_TP
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
        KNX_TP_DLL->getTPUart().registerReceivedFrame(
            [](TPUart::Frame& tpFrame) {
                // Process received frame
                if (openknx.console.bcuDebug())
                    openknx.logger.logWithPrefixAndValues("BCU<Debug>", "Received frame: %s", tpFrame.printFrame().c_str());
            });
    }
#endif

} // namespace OpenKNX
