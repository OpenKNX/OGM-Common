#include "OpenKNX/Common.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    std::string Common::logPrefix()
    {
        return "Common";
    }

    void Common::init(uint8_t firmwareRevision)
    {
        SERIAL_DEBUG.begin(115200);
        ArduinoPlatform::SerialDebug = new OpenKNX::Log::VirtualSerial("KNX");

        openknx.timerInterrupt.init();
        openknx.progLed.init(PROG_LED_PIN, PROG_LED_PIN_ACTIVE_ON);
#ifdef INFO_LED_PIN
        openknx.infoLed.init(INFO_LED_PIN, INFO_LED_PIN_ACTIVE_ON);
#endif

#if defined(ARDUINO_ARCH_RP2040) && defined(OPENKNX_RECOVERY_ON)
        // Recovery
        processRecovery();
#endif

#ifdef OPENKNX_PULSATING_BOOT
        openknx.progLed.pulsing();
        openknx.infoLed.pulsing();
#else
        openknx.progLed.on();
        openknx.infoLed.on();
#endif

        debugWait();

        logInfoP("Init firmware");

#ifdef OPENKNX_DEBUG
        showDebugInfo();
#endif

        openknx.info.firmwareRevision(firmwareRevision);

        initKnx();

        openknx.hardware.init();
    }

#ifdef OPENKNX_DEBUG
    void Common::showDebugInfo()
    {
        logDebugP("Debug logging is enabled!");
#if defined(OPENKNX_TRACE1) || defined(OPENKNX_TRACE2) || defined(OPENKNX_TRACE3) || defined(OPENKNX_TRACE4) || defined(OPENKNX_TRACE5)
        logDebugP("Trace logging is enabled with:");
        logIndentUp();
#ifdef OPENKNX_TRACE1
        logDebugP("Filter 1: %s", TRACE_STRINGIFY(OPENKNX_TRACE1));
#endif
#ifdef OPENKNX_TRACE2
        logDebugP("Filter 2: %s", TRACE_STRINGIFY(OPENKNX_TRACE2));
#endif
#ifdef OPENKNX_TRACE3
        logDebugP("Filter 3: %s", TRACE_STRINGIFY(OPENKNX_TRACE3));
#endif
#ifdef OPENKNX_TRACE4
        logDebugP("Filter 4: %s", TRACE_STRINGIFY(OPENKNX_TRACE4));
#endif
#ifdef OPENKNX_TRACE5
        logDebugP("Filter 5: %s", TRACE_STRINGIFY(OPENKNX_TRACE5));
#endif
        logIndentDown();
#endif
    }
#endif

#if defined(ARDUINO_ARCH_RP2040) && defined(OPENKNX_RECOVERY_ON)
    void Common::processRecovery()
    {
        uint8_t mode = 0;
        uint32_t recoveryStart = millis();
        pinMode(PROG_BUTTON_PIN, INPUT_PULLUP);
        while (digitalRead(PROG_BUTTON_PIN) == OPENKNX_RECOVERY_ON && mode < 3)
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
        logInfoP("Init knx stack");
        logIndentUp();

#if defined(ARDUINO_ARCH_RP2040) && defined(KNX_SERIAL)
        KNX_SERIAL.setRX(KNX_UART_RX_PIN);
        KNX_SERIAL.setTX(KNX_UART_TX_PIN);
#endif

        // pin or GPIO programming button is connected to. Default is 0
        knx.buttonPin(PROG_BUTTON_PIN);
        // Is the interrupt created in RISING or FALLING signal? Default is RISING
        // knx.buttonPinInterruptOn(PROG_BUTTON_PIN_INTERRUPT_ON);

        knx.ledPin(0);
        knx.setProgLedOnCallback([]() -> void {
            openknx.progLed.forceOn(true);
        });
        knx.setProgLedOffCallback([]() -> void {
            openknx.progLed.forceOn(false);
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
        knx.orderNumber((const uint8_t *)MAIN_OrderNumber); // set the OrderNumber
#endif
        logIndentDown();
    }

    VersionCheckResult Common::versionCheck(uint16_t manufacturerId, uint8_t *hardwareType, uint16_t firmwareVersion)
    {
        // save ets app data in information struct
        openknx.info.applicationNumber((hardwareType[2] << 8) | hardwareType[3]);
        openknx.info.applicationVersion(hardwareType[4]);

        if (manufacturerId != 0x00FA)
        {
            logError(openknx.common.logPrefix(), "This firmware supports only ManufacturerID 0x00FA");
            return FlashAllInvalid;
        }

        // hardwareType has the format 0x00 00 Ap nn vv 00
        if (memcmp(knx.bau().deviceObject().hardwareType(), hardwareType, 4) != 0)
        {
            logError(openknx.common.logPrefix(), "MAIN_ApplicationVersion changed, ETS has to reprogram the application!");
            return FlashAllInvalid;
        }

        if (knx.bau().deviceObject().hardwareType()[4] != hardwareType[4])
        {
            logError(openknx.common.logPrefix(), "MAIN_ApplicationVersion changed, ETS has to reprogram the application!");
            return FlashTablesInvalid;
        }

        return FlashValid;
    }

    void Common::debugWait()
    {
#if OPENKNX_WAIT_FOR_SERIAL > 1
        uint32_t timeoutBase = millis();
#endif

#ifdef OPENKNX_PULSATING_BOOT
        openknx.progLed.pulsing(500);
        openknx.infoLed.pulsing(500);
#else
        openknx.progLed.blinking();
        openknx.infoLed.blinking();
#endif

#if OPENKNX_WAIT_FOR_SERIAL > 1
        while (!SERIAL_DEBUG)
        {
            if (delayCheck(timeoutBase, OPENKNX_WAIT_FOR_SERIAL))
                break;
        }
#endif

#ifdef OPENKNX_PULSATING_BOOT
        openknx.progLed.pulsing();
        openknx.infoLed.pulsing();
#else
        openknx.progLed.on();
        openknx.infoLed.on();
#endif

        // for serial output
        delay(50); // delay in bootup is not critical
    }

    void Common::setup()
    {
        logTraceP("setup");

        // Register Callbacks for FunctionProperty also if the device is unloaded
        knx.bau().functionPropertyCallback([](uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength) -> bool {
            return openknx.common.processFunctionProperty(objectIndex, propertyId, length, data, resultData, resultLength);
        });
        knx.bau().functionPropertyStateCallback([](uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength) -> bool {
            return openknx.common.processFunctionPropertyState(objectIndex, propertyId, length, data, resultData, resultLength);
        });

        // Handle loop of modules
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->init();
        }

        // setup modules
        appSetup();

        // start the framework
        openknx.progLed.off();
        knx.start();

        // when module was restarted during bcu was disabled, reenable
        openknx.hardware.activatePowerRail();

#ifdef OPENKNX_WATCHDOG
        watchdogSetup();
#endif
        // setup complete turn infoLed off
        openknx.infoLed.off();
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
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->setup();
        }

        openknx.flash.load();

        // register callbacks
        registerCallbacks();

#ifdef ARDUINO_ARCH_RP2040
        // Enable loop1 if any module use dual core
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            if (openknx.modules.list[i]->usesDualCore())
                _usesDualCore = true;
        }

        // when not needed, shutdown core1
        if (!_usesDualCore)
            multicore_reset_core1();

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

#ifdef OPENKNX_WATCHDOG
    void Common::watchdogSetup()
    {
        if (!ParamLOG_Watchdog)
            return;

#if defined(ARDUINO_ARCH_SAMD)
        // used for Diagnose command
        watchdog.resetCause = Watchdog.resetCause();

        // setup watchdog to prevent endless loops
        Watchdog.enable(OPENKNX_WATCHDOG_MAX_PERIOD, false);
#elif defined(ARDUINO_ARCH_RP2040)
        Watchdog.enable(OPENKNX_WATCHDOG_MAX_PERIOD);
#endif

        logInfo("Watchdog", "Started with a watchtime of %i seconds", OPENKNX_WATCHDOG_MAX_PERIOD / 1000);
    }
    void Common::watchdogLoop()
    {
        if (!delayCheck(watchdog.timer, OPENKNX_WATCHDOG_MAX_PERIOD / 10))
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
#ifdef OPENKNX_HEARTBEAT
        openknx.progLed.debugLoop();
#endif

#ifdef OPENKNX_LOOPTIME_WARNING
        uint32_t start = millis();
#endif

        // loop console helper
        openknx.console.loop();

        // loop  knx stack
        knx.loop();

        // loop  appstack
        _loopMicros = micros();
        appLoop();

#ifdef OPENKNX_WATCHDOG
        watchdogLoop();
#endif

#if OPENKNX_LOOPTIME_WARNING > 1
        // loop took to long and last out is min 1ms ago
        if (delayCheck(start, OPENKNX_LOOPTIME_WARNING) && delayCheck(_lastLoopOutput, OPENKNX_LOOPTIME_WARNING_INTERVAL))
        {
            logErrorP("Loop took too long %i >= %i", (millis() - start), OPENKNX_LOOPTIME_WARNING);
            resetLastLoopOutput();
        }
#endif
    }

#if OPENKNX_LOOPTIME_WARNING > 1
    void Common::resetLastLoopOutput()
    {
        _lastLoopOutput = millis();
    }
#endif

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
        for (uint8_t processed = 0; freeLoopTime() && (processed < openknx.modules.count); processed++)
        {
            if (_currentModule >= openknx.modules.count)
                _currentModule = 0;

            loopModule(_currentModule);

            _currentModule++;
        }
    }

    void Common::loopModule(uint8_t index)
    {
        if (index >= openknx.modules.count)
            return;

        openknx.modules.list[index]->loop();
    }

    void Common::loop1()
    {
        if (!usesDualCore())
            return;

#ifdef OPENKNX_HEARTBEAT
        openknx.infoLed.debugLoop();
#endif
        appLoop1();
    }

    void Common::appLoop1()
    {
        // knx is not configured
        if (!knx.configured())
            return;

        for (uint8_t i = 0; i < openknx.modules.count; i++)
            if (openknx.modules.list[i]->usesDualCore())
                openknx.modules.list[i]->loop1();
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

        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->processAfterStartupDelay();
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

        openknx.progLed.powerSave();
        openknx.infoLed.powerSave();
        openknx.hardware.stopKnxMode(false);

        // first save all modules to save power before...
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->savePower();

        openknx.hardware.deactivatePowerRail();

        logInfoP("completed (%ims)", millis() - start);
        logIndentDown();

        // save data
        openknx.flash.save();

        // manual recevie stopKnxMode respone
        uint8_t response[2] = {};
        openknx.hardware.receiveResponseFromBcu(response, 2); // Receive 2 bytes

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

        openknx.progLed.powerSave(false);
        openknx.infoLed.powerSave(false);
        openknx.hardware.activatePowerRail();
        openknx.hardware.startKnxMode();

        bool reboot = false;

        // the inform modules
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            if (!openknx.modules.list[i]->restorePower())
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

        logInfoP("Restore power without reboot was successful");

        _savePinTriggered = false;
        _savedPinProcessed = 0;
        logIndentDown();
    }

    void Common::processBeforeRestart()
    {
        logTraceP("processBeforeRestart");
        logIndentUp();
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->processBeforeRestart();
        }

        openknx.flash.save();
        logIndentDown();
    }

    void Common::processBeforeTablesUnload()
    {
        logTraceP("processBeforeTablesUnload");
        logIndentUp();
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->processBeforeTablesUnload();
        }

        openknx.flash.save();
        logIndentDown();
    }
#if (MASK_VERSION & 0x0900) != 0x0900 // Coupler do not have GroupObjects
    void Common::processInputKo(GroupObject &iKo)
    {
#ifdef LOG_KoDiagnose
        if (iKo.asap() == LOG_KoDiagnose)
        {
            processDiagnoseCommand(iKo);
        }
        else
        {
#endif
            for (uint8_t i = 0; i < openknx.modules.count; i++)
            {
                openknx.modules.list[i]->processInputKo(iKo);
            }
#ifdef LOG_KoDiagnose
        }
#endif
    }

#ifdef LOG_KoDiagnose
    void Common::processDiagnoseCommand(GroupObject &iKo)
    {
        static bool processDiagnoseCommandCalled = false;

        if (!processDiagnoseCommandCalled)
        {
            processDiagnoseCommandCalled = true;
            // we store received diagnose command
            memcpy(_diagnoseInput, iKo.valueRef(), 14);
            for (uint8_t i = 0; i < openknx.modules.count; i++)
            {
                // init output buffer
                memset(_diagnoseOutput, 0, 15);
                bool output = true;
                // allow multiline output per module, max 10 lines
                for (uint8_t count = 0; count < 10 && output; count++)
                {
                    output = openknx.modules.list[i]->processDiagnoseCommand(_diagnoseInput, _diagnoseOutput, count);
                    if (output)
                    {
                        _diagnoseOutput[15] = 0;
                        iKo.value(_diagnoseOutput, Dpt(16, 1));
                        logInfoP("Diagnose: %s", _diagnoseOutput);
                        knx.loop();
                    }
                }
            }
            processDiagnoseCommandCalled = false;
        }
    }
#endif

#endif

    void Common::registerCallbacks()
    {
        knx.beforeRestartCallback([]() -> void {
            openknx.common.processBeforeRestart();
        });
#if (MASK_VERSION & 0x0900) != 0x0900 // Coupler do not have GroupObjects
        GroupObject::classCallback([](GroupObject &iKo) -> void {
            openknx.common.processInputKo(iKo);
        });
#endif
        TableObject::beforeTablesUnloadCallback([]() -> void {
            openknx.common.processBeforeTablesUnload();
        });
#ifdef SAVE_INTERRUPT_PIN
        // we need to do this as late as possible, tried in constructor, but this doesn't work on RP2040
        pinMode(SAVE_INTERRUPT_PIN, INPUT);
        attachInterrupt(
            digitalPinToInterrupt(SAVE_INTERRUPT_PIN), []() -> void {
                openknx.common.triggerSavePin();
            },
            FALLING);
#endif
    }

    uint Common::freeMemoryMin()
    {
        return _freeMemoryMin;
    }

    bool Common::processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
    {
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            if (openknx.modules.list[i]->processFunctionProperty(objectIndex, propertyId, length, data, resultData, resultLength))
                return true;
        }
        return false;
    }

    bool Common::processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
    {
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            if (openknx.modules.list[i]->processFunctionPropertyState(objectIndex, propertyId, length, data, resultData, resultLength))
                return true;
        }
        return false;
    }
} // namespace OpenKNX
