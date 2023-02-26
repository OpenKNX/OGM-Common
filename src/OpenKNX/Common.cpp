#include "Common.h"
#include "../HardwareDevices.h"

namespace OpenKNX
{
    std::string Common::logPrefix()
    {
        return "Common";
    }

    void Common::init(uint8_t firmwareRevision)
    {
        SERIAL_DEBUG.begin(115200);
        ArduinoPlatform::SerialDebug = &SERIAL_DEBUG;

        timerInterrupt.init();
        hardware.progLed.init(PROG_LED_PIN, PROG_LED_PIN_ACTIVE_ON);
#ifdef INFO_LED_PIN
        hardware.infoLed.init(INFO_LED_PIN, INFO_LED_PIN_ACTIVE_ON);
#endif

#if defined(ARDUINO_ARCH_RP2040)
        // Recovery
        processRecovery();
#endif

#ifdef OPENKNX_PULSATING_BOOT
        hardware.progLed.pulsing();
        hardware.infoLed.pulsing();
#else
        hardware.progLed.on();
        hardware.infoLed.on();
#endif

        debugWait();

        logInfoP("Init firmware");

        openknx.info.firmwareRevision(firmwareRevision);

        initKnx();

        hardware.init();
    }

#if defined(ARDUINO_ARCH_RP2040)
    void Common::processRecovery()
    {
        uint8_t mode = 0;
        uint32_t recoveryStart = millis();
        pinMode(PROG_BUTTON_PIN, INPUT);
        while (digitalRead(PROG_BUTTON_PIN) == LOW && mode < 3)
        {
            if (mode == 0 && delayCheck(recoveryStart, 500))
            {
                hardware.progLed.blinking(400);
                mode++;
            }

            if (mode == 1 && delayCheck(recoveryStart, 5500))
            {
                hardware.progLed.blinking(200);
                mode++;
            }

            if (mode == 2 && delayCheck(recoveryStart, 10500))
            {
                mode++;
                hardware.progLed.on();
            }
        }

        switch (mode)
        {
            case 1: // usbMode
                reset_usb_boot(0, 0);
                break;
            case 2: // nukeFLash KNX
                __nukeFlash(KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
                break;
            case 3: // nukeFLash
                __nukeFlash(0, NUKE_FLASH_SIZE_BYTES);
                break;
        }
    }
#endif

    void Common::initKnx()
    {
        logTraceP("Init knx stack");
        logIndentUp();

#if defined(ARDUINO_ARCH_RP2040) && defined(KNX_SERIAL)
        KNX_SERIAL.setRX(KNX_UART_RX_PIN);
        KNX_SERIAL.setTX(KNX_UART_TX_PIN);
#endif

        // pin or GPIO programming button is connected to. Default is 0
        knx.buttonPin(PROG_BUTTON_PIN);
        // Is the interrup created in RISING or FALLING signal? Default is RISING
        // knx.buttonPinInterruptOn(PROG_BUTTON_PIN_INTERRUPT_ON);

        knx.ledPin(0);
        knx.setProgLedOnCallback([]() -> void {
            openknx.hardware.progLed.forceOn(true);
        });
        knx.setProgLedOffCallback([]() -> void {
            openknx.hardware.progLed.forceOn(false);
        });

        uint8_t hardwareType[LEN_HARDWARE_TYPE] = {0x00, 0x00, MAIN_OpenKnxId, MAIN_ApplicationNumber, MAIN_ApplicationVersion, 0x00};

        // first setup flash version check
        knx.bau().versionCheckCallback(versionCheck);
        // set correct hardware type for flash compatibility check
        knx.bau().deviceObject().hardwareType(hardwareType);
        // read flash data
        knx.readMemory();
        // set hardware type again, in case an other hardware type was deserialized from flash
        knx.bau().deviceObject().hardwareType(hardwareType);
        // set firmware version as user info (PID_VERSION)
        // 5 bit revision, 5 bit major, 6 bit minor
        // output in ETS as [revision] major.minor
        knx.bau().deviceObject().version(openknx.info.firmwareVersion());

#ifdef MAIN_OrderNumber
        knx.orderNumber((const uint8_t*)MAIN_OrderNumber); // set the OrderNumber
#endif
        logIndentDown();
    }

    VersionCheckResult Common::versionCheck(uint16_t manufacturerId, uint8_t* hardwareType, uint16_t firmwareVersion)
    {
        // save ets app data in information struct
        openknx.info.applicationNumber((hardwareType[2] << 8) | hardwareType[3]);
        openknx.info.applicationVersion(hardwareType[4]);

        if (manufacturerId != 0x00FA)
        {
            logError(openknx.logPrefix(), "This firmware supports only ManufacturerID 0x00FA");
            return FlashAllInvalid;
        }

        // hardwareType has the format 0x00 00 Ap nn vv 00
        if (memcmp(knx.bau().deviceObject().hardwareType(), hardwareType, 4) != 0)
        {
            logError(openknx.logPrefix(), "MAIN_ApplicationVersion changed, ETS has to reprogram the application!");
            return FlashAllInvalid;
        }

        if (knx.bau().deviceObject().hardwareType()[4] != hardwareType[4])
        {
            logError(openknx.logPrefix(), "MAIN_ApplicationVersion changed, ETS has to reprogram the application!");
            return FlashTablesInvalid;
        }

        return FlashValid;
    }

    void Common::debugWait()
    {
#if defined(DEBUG_WAIT_FOR_SERIAL)
        uint32_t timeoutBase = millis();

#ifdef OPENKNX_PULSATING_BOOT
        hardware.progLed.pulsing(500);
        hardware.infoLed.pulsing(500);
#else
        hardware.progLed.blinking();
        hardware.infoLed.blinking();
#endif

        while (!SERIAL_DEBUG)
        {
#if DEBUG_WAIT_FOR_SERIAL > 1
            if (delayCheck(timeoutBase, DEBUG_WAIT_FOR_SERIAL))
                break;
#endif
        }
#elif defined(DEBUG_DELAY)
        delay(DEBUG_DELAY);
#endif

#ifdef OPENKNX_PULSATING_BOOT
        hardware.progLed.pulsing();
        hardware.infoLed.pulsing();
#else
        hardware.progLed.on();
        hardware.infoLed.on();
#endif

        // for serial output
        delay(50);
    }

    void Common::setup()
    {
        logTraceP("setup");

        // setup modules
        appSetup();

        // start the framework
        hardware.progLed.off();
        knx.start();

        // when module was restarted during bcu was disabled, reenable
        hardware.activatePowerRail();

#ifdef WATCHDOG
        watchdogSetup();
#endif
        // setup complete turn infoLed off
        hardware.infoLed.off();
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
            if (_modules.list[i]->usesDualCore())
                _usesDualCore = true;
        }

        flash.load();

        // register callbacks
        registerCallbacks();

#ifdef ARDUINO_ARCH_RP2040
        if (usesDualCore())
        {
            logTraceP("activate loop2");
            multicore_launch_core1(Common::loop2);
        }
#endif
    }

    bool Common::usesDualCore()
    {
#if defined(ARDUINO_ARCH_RP2040)
        return _usesDualCore;
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

        logInfo("Watchdog", "Started with a watchtime of %i seconds", WATCHDOG_MAX_PERIOD_MS / 1000);
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
#ifdef DEBUG_HEARTBEAT
        hardware.progLed.debugLoop();
#endif

#ifdef DEBUG_LOOP_TIME
        uint32_t start = millis();
#endif

        // loop console helper
        console.loop();

        // loop  knx stack
        knx.loop();

        // loop  appstack
        _loopMicros = micros();
        appLoop();

#ifdef WATCHDOG
        watchdogLoop();
#endif

#ifdef DEBUG_LOOP_TIME
        // loop took to long and last out is min 1s ago
#if DEBUG_LOOP_TIME > 1
        if (delayCheck(start, DEBUG_LOOP_TIME) && delayCheck(lastDebugTime, 1000))
#else
        if (delayCheck(start, 5000) && delayCheck(lastDebugTime, 1000))
#endif
        {
            logDebugP("Loop took too long %i >= %i", (millis() - start), DEBUG_LOOP_TIME);
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
#ifdef __time_critical_func
    void __time_critical_func(Common::collectMemoryStats)()
#else
    void Common::collectMemoryStats()
#endif
    {
        // int current = freeMemory();
        _freeMemoryMin = MIN(_freeMemoryMin, freeMemory());
    }

    /**
     * Run loop() of as many modules as possible, within available free loop time.
     * Each module will be processed 0 or 1 times only, not more.
     *
     * Repeated calls will resume with the first/next unprocessed module.
     * For all modules the min/max number of loop()-calls will not differ more than 1.
     */
    void Common::processModulesLoop()
    {
        for (uint8_t processed = 0; freeLoopTime() && (processed < _modules.count); processed++)
        {
            if (_currentModule >= _modules.count)
                _currentModule = 0;

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
        {
#ifdef DEBUG_HEARTBEAT
            openknx.hardware.infoLed.debugLoop();
#endif
            openknx.appLoop2();
        }
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
            if (_modules.ids[i] == id)
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

        logTraceP("processAfterStartupDelay");
        logIndentUp();

        _afterStartupDelay = true;

        for (uint8_t i = 0; i < _modules.count; i++)
        {
            _modules.list[i]->processAfterStartupDelay();
        }

        logIndentDown();
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

        logErrorP("SavePIN triggered!");
        logIndentUp();
        logInfoP("Save power");
        logIndentUp();

        openknx.hardware.progLed.powerSave();
        openknx.hardware.infoLed.powerSave();
        hardware.stopKnxMode(false);

        // first save all modules to save power before...
        for (uint8_t i = 0; i < _modules.count; i++)
            _modules.list[i]->savePower();

        hardware.deactivatePowerRail();

        logInfoP("completed (%ims)", millis() - start);
        logIndentDown();

        // save data
        flash.save();

        // manual recevie stopKnxMode respone
        uint8_t response[2] = {};
        hardware.receiveResponseFromBcu(response, 2); // Receive 2 bytes

        _savedPinProcessed = millis();
        logIndentDown();
    }
    void Common::processRestoreSavePin()
    {
        // savePin not triggered
        if (!_savePinTriggered)
            return;

        if (!delayCheck(_savedPinProcessed, 1000))
            return;

        logInfoP("Restore power (after 1s)");
        logIndentUp();

        openknx.hardware.progLed.powerSave(false);
        openknx.hardware.infoLed.powerSave(false);
        hardware.activatePowerRail();
        hardware.startKnxMode();

        bool reboot = false;

        // the inform modules
        for (uint8_t i = 0; i < _modules.count; i++)
            if (!_modules.list[i]->restorePower())
            {
                reboot = true;
                break;
            }

        if (reboot)
        {
            logInfoP("Need reboot");
            delay(10);
            knx.platform().restart();
        }

        logInfoP("Restore power without reboot was successfull");

        _savePinTriggered = false;
        _savedPinProcessed = 0;
        logIndentDown();
    }

    void Common::processBeforeRestart()
    {
        logTraceP("processBeforeRestart");
        logIndentUp();
        for (uint8_t i = 0; i < _modules.count; i++)
        {
            _modules.list[i]->processBeforeRestart();
        }

        flash.save();
        logIndentDown();
    }

    void Common::processBeforeTablesUnload()
    {
        logTraceP("processBeforeTablesUnload");
        logIndentUp();
        for (uint8_t i = 0; i < _modules.count; i++)
        {
            _modules.list[i]->processBeforeTablesUnload();
        }
#ifdef ARDUINO_ARCH_RP2040
        multicore_reset_core1();
#endif
        flash.save();
        logIndentDown();
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
