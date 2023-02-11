#include "Common.h"
#include "../HardwareDevices.h"

namespace OpenKNX
{
    Common::Common()
    {}

    void Common::init(uint8_t firmwareRevision)
    {
        SERIAL_DEBUG.begin(115200);
        ArduinoPlatform::SerialDebug = &SERIAL_DEBUG;

        pinMode(PROG_LED_PIN, OUTPUT);
        digitalWrite(PROG_LED_PIN, HIGH);
#ifdef DEBUG_DELAY
        delay(DEBUG_DELAY);
#endif

        log("OpenKNX", "init");
        openknx.info.firmwareRevision(firmwareRevision);

#ifdef INFO_LED_PIN
        pinMode(INFO_LED_PIN, OUTPUT);
        ledInfo(true);
#endif

        initKnx();
    }

    void Common::initKnx()
    {
        log("OpenKNX", "init knx");
#ifdef ARDUINO_ARCH_RP2040
        Serial1.setRX(KNX_UART_RX_PIN);
        Serial1.setTX(KNX_UART_TX_PIN);
#endif

        uint8_t hardwareType[LEN_HARDWARE_TYPE] = {0x00, 0x00, MAIN_OpenKnxId, MAIN_ApplicationNumber, MAIN_ApplicationVersion, 0x00};

        // first setup flash version check
        knx.bau().versionCheckCallback(versionCheck);
        // set correct hardware type for flash compatibility check
        knx.bau().deviceObject().hardwareType(hardwareType);
        // read flash data
        knx.readMemory();
        // set hardware type again, in case an other hardware type was deserialized from flash
        knx.bau().deviceObject().hardwareType(hardwareType);
        // set firmware version als user info (PID_VERSION)
        // 5 bit revision, 5 bit major, 6 bit minor
        // output in ETS as [revision] major.minor
        knx.bau().deviceObject().version(openknx.info.firmwareVersion());

#ifdef MAIN_OrderNumber
        knx.orderNumber((const uint8_t*)MAIN_OrderNumber); // set the OrderNumber
#endif
    }

    VersionCheckResult Common::versionCheck(uint16_t manufacturerId, uint8_t* hardwareType, uint16_t firmwareVersion)
    {
        // save ets app data in information struct
        openknx.info.applicationNumber((hardwareType[2] << 8) | hardwareType[3]);
        openknx.info.applicationVersion(hardwareType[4]);

        if (manufacturerId != 0x00FA)
        {
            openknx.log("OpenKNX", "This firmware supports only ManufacturerID 0x00FA");
            return FlashAllInvalid;
        }

        // hardwareType has the format 0x00 00 Ap nn vv 00
        if (memcmp(knx.bau().deviceObject().hardwareType(), hardwareType, 4) != 0)
        {
            openknx.log("OpenKNX", "MAIN_ApplicationVersion changed, ETS has to reprogram the application!");
            return FlashAllInvalid;
        }

        if (knx.bau().deviceObject().hardwareType()[4] != hardwareType[4])
        {
            openknx.log("OpenKNX", "MAIN_ApplicationVersion changed, ETS has to reprogram the application!");
            return FlashTablesInvalid;
        }

        return FlashValid;
    }

    void Common::setup()
    {
        log("OpenKNX", "setup");

        digitalWrite(PROG_LED_PIN, LOW);

        // pin or GPIO the programming led is connected to. Default is LED_BUILDIN
        knx.ledPin(PROG_LED_PIN);
        // is the led active on HIGH or low? Default is LOW
        knx.ledPinActiveOn(PROG_LED_PIN_ACTIVE_ON);
        // pin or GPIO programming button is connected to. Default is 0
        knx.buttonPin(PROG_BUTTON_PIN);
        // Is the interrup created in RISING or FALLING signal? Default is RISING
        // knx.buttonPinInterruptOn(PROG_BUTTON_PIN_INTERRUPT_ON);

        appSetup();

        // start the framework
        knx.start();

#ifdef WATCHDOG
        watchdogSetup();
#endif

#ifdef INFO_LED_PIN
        ledInfo(false);
#endif
    }

    void Common::appSetup()
    {
        if (!knx.configured())
            return;

#ifdef LOG_StartupDelayBase
        _startupDelay = millis();
#endif
#ifdef LOG_HeartbeatDelayBase
        _heartbeatDelay = 0;
#endif

        // Handle loop of modules
        for (uint8_t i = 0; i < _modules.count; i++)
        {
            _modules.list[i]->setup();
            if (_modules.list[i]->usesSecCore())
                _useSecondCore = true;
            collectMemoryStats();
        }

        flash.load();

        // register callbacks
        registerCallbacks();

#ifdef ARDUINO_ARCH_RP2040
        if (useSecondCore())
        {
            multicore_launch_core1(Common::loop2);
        }
#endif
    }

    bool Common::useSecondCore()
    {
#if defined(ARDUINO_ARCH_RP2040)
        if (!knx.configured())
            return false;

        return _useSecondCore;
#else
        return false;
#endif
    }

#ifdef WATCHDOG
    void Common::watchdogSetup()
    {
        if (!ParamLOG_Watchdog)
            return;

#if defined(ARDUINO_ARCH_SAMD)
        // used for Diagnose command
        watchdog.resetCause = Watchdog.resetCause();

        // setup watchdog to prevent endless loops
        Watchdog.enable(WATCHDOG_MAX_PERIOD_MS, false);
#elif defined(ARDUINO_ARCH_RP2040)
        Watchdog.enable(WATCHDOG_MAX_PERIOD_MS);
#endif

        log("Watchdog", "Started with a watchtime of %i seconds", WATCHDOG_MAX_PERIOD_MS / 1000);
    }
    void Common::watchdogLoop()
    {
        if (!delayCheck(watchdog.timer, WATCHDOG_MAX_PERIOD_MS / 10))
            return;

        if (!ParamLOG_Watchdog)
            return;

        Watchdog.reset();
        watchdog.timer = millis();
    }
#endif

    // main loop
    void Common::loop()
    {

#ifdef DEBUG_LOOP_TIME
        uint32_t start = millis();
#endif

        // loop console helper
        console.loop();
        collectMemoryStats();

        // loop  knx stack
        knx.loop();
        collectMemoryStats();

        // loop  appstack
        _loopMicros = micros();
        appLoop();
        collectMemoryStats();

#ifdef WATCHDOG
        watchdogLoop();
#endif

#ifdef DEBUG_LOOP_TIME
        // loop took to long and last out is min 1s ago
        if (delayCheck(start, DEBUG_LOOP_TIME) && delayCheck(lastDebugTime, 1000))
        {
            log("OpenKNX", "loop took too long %i", (millis() - start));
            lastDebugTime = millis();
        }
#endif
    }

    // loop with abort conditions
    void Common::appLoop()
    {
        // knx is not configured
        if (!knx.configured())
            return;

#ifdef LOG_HeartbeatDelayBase
        // Handle heartbeat delay
        processHeartbeat();
#endif

        processSavePin();
        processRestoreSavePin();
        processAfterStartupDelay();
        processModulesLoop();
    }

    bool Common::freeLoopTime()
    {
        return ((micros() - _loopMicros) < OPENKNX_MAX_LOOPTIME);
    }

    void Common::collectMemoryStats()
    {
        int current = freeMemory();

        if (current < 0)
            return;

        if ((uint)current < _freeMemoryMin)
            _freeMemoryMin = current;
    }

    void Common::processModulesLoop()
    {
        for (uint8_t processed = 0; freeLoopTime() && (processed < _modules.count); processed++)
        {
            if (_currentModule >= _modules.count)
            {
                _currentModule = 0;
            }

            loopModule(_currentModule);

            _currentModule++;
        }
    }

    void Common::loopModule(uint8_t index)
    {
        if (index >= _modules.count)
            return;

        _modules.list[index]->loop();
    }

    void Common::loop2()
    {
        while (true)
            openknx.appLoop2();
    }

    void Common::appLoop2()
    {
        for (uint8_t i = 0; i < _modules.count; i++)
            _modules.list[i]->loop2();
    }

    void Common::addModule(uint8_t id, Module* module)
    {
        _modules.count++;
        _modules.list[_modules.count - 1] = module;
        _modules.ids[_modules.count - 1] = id;
    }

    Module* Common::getModule(uint8_t id)
    {
        for (uint8_t i = 0; i < _modules.count; i++)
        {
            if (_modules.ids[i - 1] == id)
                return _modules.list[i];
        }

        return nullptr;
    }

    Modules* Common::getModules()
    {
        return &_modules;
    }

    bool Common::afterStartupDelay()
    {
        return _afterStartupDelay;
    }

    void Common::processAfterStartupDelay()
    {
        if (afterStartupDelay())
            return;

#ifdef LOG_StartupDelayBase
        if (!delayCheck(_startupDelay, ParamLOG_StartupDelayTimeMS))
            return;
#endif

        _afterStartupDelay = true;

        for (uint8_t i = 0; i < _modules.count; i++)
        {
            _modules.list[i]->processAfterStartupDelay();
        }
    }

#ifdef LOG_HeartbeatDelayBase
    void Common::processHeartbeat()
    {
        // the first heartbeat is send directly after startup delay of the device
        if (_heartbeatDelay == 0 || delayCheck(_heartbeatDelay, ParamLOG_HeartbeatDelayTimeMS))
        {
            // we waited enough, let's send a heartbeat signal
            KoLOG_Heartbeat.value(true, DPT_Switch);
            _heartbeatDelay = millis();
        }
    }
#endif

    void Common::triggerSavePin()
    {
        _savePinTriggered = true;
    }

    void Common::processSavePin()
    {
        // savePin not triggered
        if (!_savePinTriggered)
            return;

        // processSavePin already called
        if (_savedPinProcessed > 0)
            return;

        uint32_t start = millis();
        log("OpenKNX", "savePower");

        // first save all modules to save power before...
        for (uint8_t i = 0; i < _modules.count; i++)
            _modules.list[i]->savePower();

        // ... save power by shutdown >5V power trail
        deactivatePowerRail();
        log("OpenKNX", "savePower (%ims)", millis() - start);

        // save data
        flash.save();

        _savedPinProcessed = millis();
    }
    void Common::processRestoreSavePin()
    {
        // savePin not triggered
        if (!_savePinTriggered)
            return;

        if (!delayCheck(_savedPinProcessed, 500))
            return;

        log("OpenKNX", "restorePower after wait 500ms");

        // restore  >5V power trail
        activatePowerRail();

        bool noReboot = true;

        // the inform modules
        for (uint8_t i = 1; i <= _modules.count && noReboot; i++)
            noReboot = noReboot && _modules.list[i]->restorePower();

        if (!noReboot)
        {
            log("OpenKNX", "  need reboot");
            knx.platform().restart();
        }

        log("OpenKNX", "  restorePower without reboot was successfull");

        _savePinTriggered = false;
        _savedPinProcessed = 0;
    }

    void Common::processBeforeRestart()
    {
        log("OpenKNX", "processBeforeRestart");
        for (uint8_t i = 0; i < _modules.count; i++)
        {
            _modules.list[i]->processBeforeRestart();
        }

        flash.save();
    }

    void Common::processBeforeTablesUnload()
    {
        log("OpenKNX", "processBeforeTablesUnload");
        for (uint8_t i = 0; i < _modules.count; i++)
        {
            _modules.list[i]->processBeforeTablesUnload();
        }
#ifdef ARDUINO_ARCH_RP2040
        multicore_reset_core1();
#endif
        flash.save();
    }

    void Common::processInputKo(GroupObject& iKo)
    {
        for (uint8_t i = 0; i < _modules.count; i++)
        {
            _modules.list[i]->processInputKo(iKo);
        }
    }

    void Common::registerCallbacks()
    {
        knx.beforeRestartCallback([]() -> void {
            openknx.processBeforeRestart();
        });
        GroupObject::classCallback([](GroupObject& iKo) -> void {
            openknx.processInputKo(iKo);
        });
        TableObject::beforeTablesUnloadCallback([]() -> void {
            openknx.processBeforeTablesUnload();
        });
#ifdef SAVE_INTERRUPT_PIN
        // we need to do this as late as possible, tried in constructor, but this doesn't work on RP2040
        pinMode(SAVE_INTERRUPT_PIN, INPUT);
        attachInterrupt(
            digitalPinToInterrupt(SAVE_INTERRUPT_PIN), []() -> void {
                openknx.triggerSavePin();
            },
            FALLING);
#endif
    }

    uint Common::freeMemoryMin()
    {
        return _freeMemoryMin;
    }
} // namespace OpenKNX

OpenKNX::Common openknx;
