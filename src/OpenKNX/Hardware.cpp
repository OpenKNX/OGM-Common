#include "OpenKNX/Facade.h"

#ifdef ARDUINO_ARCH_RP2040
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

        initFilesystem();
#endif
#ifndef ARDUINO_ARCH_SAMD
        requestBcuSystemState();
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
        pinMode(PROG_BUTTON_PIN, INPUT_PULLUP);
        attachInterrupt(
            digitalPinToInterrupt(PROG_BUTTON_PIN),
            []() -> void { openknx.progButton.change(!digitalRead(PROG_BUTTON_PIN)); },
            CHANGE);

#ifdef FUNC1_BUTTON_PIN
        pinMode(FUNC1_BUTTON_PIN, INPUT_PULLUP);
        attachInterrupt(
            digitalPinToInterrupt(FUNC1_BUTTON_PIN),
            []() -> void { openknx.func1Button.change(!digitalRead(FUNC1_BUTTON_PIN)); },
            CHANGE);
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

    void Hardware::requestBcuSystemState()
    {
        logDebug(openknx.logger.buildPrefix("Hardware", "BCU"), "Request system state");
        logIndentUp();
        uint8_t command[] = {U_SYSTEM_STATE, U_SYSTEM_STAT_IND};
        sendCommandToBcu(command, 2, "SYSTEM_STATE");

        uint8_t response[2] = {};
        receiveResponseFromBcu(response, 2);

        // Check is NCN5130 - untested!
        if ((response[1] & 3) == 3) // second byte
            features |= BOARD_HW_NCN5130;

        logHexTrace(openknx.logger.buildPrefix("Hardware", "BCU"), response, 2);
        logIndentDown();
    }

    void Hardware::sendCommandToBcu(const uint8_t* command, const uint8_t length, const char* debug)
    {
        if (!knx.platform().knxUart())
            return;

        if (debug != nullptr)
            logTrace(openknx.logger.buildPrefix("Hardware", "BCU"), "Send command %s to BCU", debug);

        // send system state command and interpret answer
        knx.platform().knxUart()->flush();
        knx.platform().knxUart()->write(command, length);
    }

    void Hardware::receiveResponseFromBcu(uint8_t* response, const uint8_t length, const uint16_t wait /* = 100 */)
    {
        uint8_t i = 0;
        uint32_t waitTimer = millis();
        while (!delayCheck(waitTimer, wait))
        {
            if (knx.platform().knxUart()->available())
            {
                response[i] = knx.platform().knxUart()->read();
                i++;
            }
        }
    }

    bool Hardware::validateResponse(const uint8_t* expected, const uint8_t* response, const uint8_t length)
    {
        for (uint8_t i = 0; i < length; i++)
        {
            if (expected[i] != response[i])
            {
                logError(openknx.logger.buildPrefix("Hardware", "BCU"), "FAILED - received unexpected response:");
                logHexError(openknx.logger.buildPrefix("Hardware", "BCU"), response, length);
                return false;
            }
        }

        return true;
    }

    void Hardware::deactivatePowerRail()
    {
        logDebug(openknx.logger.buildPrefix("Hardware", "BCU"), "Switching off 5V / 20V rail of BCU");
        logIndentUp();
        uint8_t command[] = {U_INT_REG_WR_REQ_ACR0, ACR0_FLAG_XCLKEN | ACR0_FLAG_V20VCLIMIT};
        sendCommandToBcu(command, 2, "U_INT_REG_WR_REQ_ACR0");
        // sendCommandToBCU("READ_ACR0 (Analog control register 0)", U_INT_REG_RD_REQ_ACR0, ACR0_FLAG_XCLKEN | ACR0_FLAG_V20VCLIMIT);
        logIndentDown();
    }

    void Hardware::activatePowerRail()
    {
        logDebug(openknx.logger.buildPrefix("Hardware", "BCU"), "Switching on 5V rail of BCU");
        logIndentUp();
        uint8_t command[] = {U_INT_REG_WR_REQ_ACR0, ACR0_FLAG_DC2EN | ACR0_FLAG_V20VEN | ACR0_FLAG_XCLKEN | ACR0_FLAG_V20VCLIMIT};
        sendCommandToBcu(command, 2, "U_INT_REG_WR_REQ_ACR0");
        logIndentDown();
    }

    void Hardware::stopKnxMode(bool waiting /* = true */)
    {
        logDebug(openknx.logger.buildPrefix("Hardware", "BCU"), "Stop KNX Mode");
        logIndentUp();
        uint8_t command[] = {U_STOP_MODE_REQ};
        sendCommandToBcu(command, 1, "STOP_MODE");

        if (waiting)
        {
            uint8_t response[2] = {};
            receiveResponseFromBcu(response, 2); // Receive 2 bytes

            uint8_t expected[] = {U_STOP_MODE_IND};
            validateResponse(expected, response, 1); // Check first Byte
        }
        logIndentDown();
    }

    void Hardware::startKnxMode(bool waiting /* = true */)
    {
        logDebug(openknx.logger.buildPrefix("Hardware", "BCU"), "Start KNX Mode");
        logIndentUp();
        uint8_t command[] = {U_EXIT_STOP_MODE_REQ};
        sendCommandToBcu(command, 1, "EXIT_STOP_MODE"); // U_RESET_IND

        if (waiting)
        {
            uint8_t response[1] = {};
            receiveResponseFromBcu(response, 1); // Receive 1 byte

            uint8_t expected[] = {U_RESET_IND};
            validateResponse(expected, response, 1); // Check first Byte
        }
        logIndentDown();
    }

    void Hardware::fatalError(uint8_t code, const char* message)
    {
        logError("FatalError", "Code: %d (%s)", code, message);
        logIndentUp();
#ifdef INFO1_LED_PIN
        openknx.info1Led.on();
#endif
        openknx.progLed.errorCode(code);

        // stopknx
        stopKnxMode();
        // poweroff
        deactivatePowerRail();
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
