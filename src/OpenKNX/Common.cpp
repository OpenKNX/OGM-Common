#include "OpenKNX/Common.h"
#include "OpenKNX/Facade.h"
#include "OpenKNX/Stat/RuntimeStat.h"

#ifndef ParamBASE_InternalTime 
    #define ParamBASE_InternalTime 0
#endif
#ifndef OPENKNX_TimeProvider
    #ifdef BASE_KoTime
        #include "OpenKNX/Time/TimeProviderKnx.h"
        #define OPENKNX_TimeProvider Time::TimeProviderKnx
    #endif
#endif

#if defined(OPENKNX_DUALCORE) && defined(ARDUINO_ARCH_ESP32)
extern void loop1();
extern void setup1();
#endif

namespace OpenKNX
{
    std::string Common::logPrefix()
    {
        return "Common";
    }

    void Common::init(uint8_t firmwareRevision)
    {
        ArduinoPlatform::SerialDebug = new OpenKNX::Log::VirtualSerial("KNX");

        openknx.timerInterrupt.init();
        openknx.hardware.initLeds();

#if defined(PROG_BUTTON_PIN) && PROG_BUTTON_PIN >= 0 && OPENKNX_RECOVERY_TIME > 0
        processRecovery();
#endif

        openknx.hardware.initButtons();

#ifdef OPENKNX_NO_BOOT_PULSATING
        openknx.progLed.on();
    #ifdef INFO1_LED_PIN
        openknx.info1Led.on();
    #endif
#else
        openknx.progLed.pulsing();
    #ifdef INFO1_LED_PIN
        openknx.info1Led.pulsing();
    #endif
#endif

        debugWait();

        if (openknx.watchdog.lastReset()) logErrorP("Restarted by watchdog");

        logInfoP("Init firmware");

#ifdef OPENKNX_DEBUG
        showDebugInfo();
#endif

        openknx.hardware.initFlash();
        openknx.info.serialNumber(knx.platform().uniqueSerialNumber());
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

#if defined(PROG_BUTTON_PIN) && PROG_BUTTON_PIN >= 0 && OPENKNX_RECOVERY_TIME > 0
    void Common::processRecovery()
    {
        bool erase = false;
        pinMode(PROG_BUTTON_PIN, INPUT_PULLUP);
        while (!digitalRead(PROG_BUTTON_PIN))
        {
            if (millis() >= OPENKNX_RECOVERY_TIME)
            {
                if (!erase)
                {
                    openknx.progLed.blinking(200);
                    erase = true;
                }
            }
        }

        if (erase)
        {
            openknx.hardware.initFlash();
            openknx.knxFlash.erase();
            restart();
        }

        openknx.progLed.off();
    }
#endif

    void Common::initKnx()
    {
        logInfoP("Init knx stack");
        logIndentUp();

#if (defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_ESP32)) && defined(KNX_UART_RX_PIN) && defined(KNX_UART_TX_PIN)
        knx.platform().knxUartPins(KNX_UART_RX_PIN, KNX_UART_TX_PIN);
#endif

        openknx.progButton.onShortClick([] { knx.toggleProgMode(); });

        knx.ledPin(0);
        knx.setProgLedOnCallback([] { openknx.progLed.forceOn(true); });
        knx.setProgLedOffCallback([] { openknx.progLed.forceOn(false); });

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
#ifdef OPENKNX_NO_BOOT_PULSATING
        openknx.progLed.blinking();
    #ifdef INFO1_LED_PIN
        openknx.info1Led.blinking();
    #endif
#else
        openknx.progLed.pulsing(500);
    #ifdef INFO1_LED_PIN
        openknx.info1Led.pulsing(500);
    #endif
#endif

#if OPENKNX_WAIT_FOR_SERIAL > 1 && !defined(OPENKNX_RTT) && defined(SERIAL_DEBUG)
        uint32_t timeoutBase = millis();
        while (!SERIAL_DEBUG)
        {
            if (delayCheck(timeoutBase, OPENKNX_WAIT_FOR_SERIAL))
                break;
        }
#endif

#ifdef OPENKNX_NO_BOOT_PULSATING
        openknx.progLed.on();
    #ifdef INFO1_LED_PIN
        openknx.info1Led.on();
    #endif
#else
        openknx.progLed.pulsing();
    #ifdef INFO1_LED_PIN
        openknx.info1Led.pulsing();
    #endif
#endif
    }

    void Common::setup()
    {
        // Handle init of modules
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->init();

        bool configured = knx.configured();
        openknx.time.setup(configured);

#ifdef OPENKNX_TimeProvider
        if (!openknx.time.hasTimerProvder() && !ParamBASE_InternalTime)
            openknx.time.setTimeProvider(new OPENKNX_TimeProvider());
#endif
#ifdef BASE_StartupDelayBase
        _startupDelay = millis();
#endif

#ifdef INFO1_LED_PIN
        // pre setup complete
        openknx.info1Led.off();
#endif

        // Handle setup of modules
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->setup(configured);

        if (configured) openknx.flash.load();

        // start the framework + isr if needed
        knx.start();
        openknx.hardware.initKnxRxISR();

#ifdef OPENKNX_WATCHDOG
        if (ParamBASE_Watchdog) openknx.watchdog.activate();
#endif

        // register callbacks
        registerCallbacks();

        // setup0 is done
        _setup0Ready = true;

#ifdef OPENKNX_DUALCORE
    #ifdef ARDUINO_ARCH_ESP32
        xTaskCreateUniversal([](void* parms) {
            ::setup1();
            for (;;)
            {
                ::loop1();
                vTaskDelay(1);
            } }, "setup1AndLoop1", ARDUINO_LOOP1_STACK_SIZE, NULL, 0, nullptr, 0);
    #endif

        // if we have a second core wait for setup1 is done
        if (openknx.usesDualCore())
            while (!_setup1Ready)
                delay(1);
#endif // OPENKNX_DUALCORE

        openknx.logger.logOpenKnxHeader();

#ifndef OPENKNX_DUALCORE
        openknx.progLed.off();
#endif
    }

#ifdef OPENKNX_DUALCORE
    void Common::setup1()
    {
        openknx.timerInterrupt.init1();

        // wait for setup0
        while (!_setup0Ready)
            delay(50);

        // skip if no dual core is used
        if (!openknx.usesDualCore())
        {
            logErrorP("setup1 is invoked without utilizing dual-core modules");
            _setup1Ready = true;
            return;
        }

        bool configured = knx.configured();

        // Handle setup1 of modules
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->setup1(configured);

        _setup1Ready = true;
        openknx.progLed.off();
    }
#endif

    // main loop
    void Common::loop()
    {
#ifdef ARDUINO_ARCH_ESP32
        collectStackStats();
#endif

        _skipLooptimeWarning = false;

        uptime(false);

        if (!_setup0Ready) return;
#ifdef OPENKNX_DUALCORE
        if (!_setup1Ready) return;
#endif

        RUNTIME_MEASURE_BEGIN(_runtimeLoop);

#ifdef OPENKNX_HEARTBEAT
        openknx.progLed.debugLoop();
#endif
#ifdef OPENKNX_WATCHDOG
        openknx.watchdog.loop();
#endif

#ifdef OPENKNX_LOOPTIME_WARNING
        uint32_t start = millis();
#endif
        // loop console helper
        RUNTIME_MEASURE_BEGIN(_runtimeConsole);
        openknx.console.loop();
        RUNTIME_MEASURE_END(_runtimeConsole);

        // loop  knx stack
        RUNTIME_MEASURE_BEGIN(_runtimeKnxStack);
        knx.loop();
        RUNTIME_MEASURE_END(_runtimeKnxStack);

        // loop timemanager helper
        RUNTIME_MEASURE_BEGIN(_runtimeTimeManager);
        openknx.time.loop();
        RUNTIME_MEASURE_END(_runtimeTimeManager);
        // loop timemanager helper
        RUNTIME_MEASURE_BEGIN(_runtimeSunCalculation);
#if defined(ParamBASE_Latitude) && defined(ParamBASE_Longitude)
        openknx.sun.loop();
#endif
        RUNTIME_MEASURE_END(_runtimeSunCalculation);
#ifdef KoBASE_Date // HACK to prevent read telegrams from logic and common. Can be removed if logic is updated to use the time from common
        bool checkForDateRead = !openknx.time._disableKoRead && knx.configured() && !ParamBASE_InternalTime;
        if (checkForDateRead && KoBASE_Date.commFlag() == ComFlag::ReadRequest)
            checkForDateRead = false;
#endif

        // loop  appstack
        _loopMicros = micros();

        // knx is configured
        if (knx.configured())
        {

#ifdef BASE_HeartbeatDelayBase
            // Handle heartbeat delay
            processHeartbeat();
#endif
#ifdef BASE_PeriodicSave
            processPeriodicSave();
#endif

            processSavePin();
            processRestoreSavePin();
            processAfterStartupDelay();
        }

        RUNTIME_MEASURE_BEGIN(_runtimeModuleLoop);
        processModulesLoop();
        RUNTIME_MEASURE_END(_runtimeModuleLoop);

        RUNTIME_MEASURE_END(_runtimeLoop);

#if OPENKNX_LOOPTIME_WARNING > 1
        // loop took to long and last out is min 1ms ago
        if (!_skipLooptimeWarning && delayCheck(start, OPENKNX_LOOPTIME_WARNING) && delayCheck(_lastLooptimeWarning, OPENKNX_LOOPTIME_WARNING_INTERVAL))
        {
            logErrorP("Warning: The loop took longer than usual (%i >= %i)", (millis() - start), OPENKNX_LOOPTIME_WARNING);
            _lastLooptimeWarning = millis();
        }
#endif
#ifdef KoBASE_Date // HACK to prevent read telegrams from logic and common. Can be removed if logic is updated to use the time from common
        if (checkForDateRead && KoBASE_Date.commFlag() == ComFlag::ReadRequest)
        {
            logErrorP("Disable time KO' reads in Common because the LogicModule is old and sent the read request");
            openknx.time._disableKoRead = true;
            if (openknx.time.hasTimerProvder())
                openknx.time.getTimeProvder()->_disableKoRead = true;
        }
#endif
    }

#ifdef BASE_PeriodicSave
    void Common::processPeriodicSave()
    {
        const uint32_t delay = ParamBASE_PeriodicSave * 3600000;
        if (delay > 0 && delayCheck(openknx.flash.lastWrite(), delay))
        {
            logInfoP("Start periodic save");
            logIndentUp();
            openknx.flash.save();
            logIndentDown();
        }
    }
#endif

    void Common::skipLooptimeWarning()
    {
#if OPENKNX_LOOPTIME_WARNING > 1
        _skipLooptimeWarning = true;
#endif
    }

    bool Common::freeLoopTime()
    {
        return !delayCheckMicros(_loopMicros, OPENKNX_MAX_LOOPTIME);
    }

    bool Common::freeLoopIterate(uint8_t size, uint8_t& position, uint8_t& processed)
    {
        processed++;
        position++;

        // when you have to start from the beginning again
        if (position >= size) position = 0;

        // if freeloop time over
        if (!freeLoopTime()) return false;

        // once completely run through
        if (processed >= size) return false;

        return true;
    }

    void __time_critical_func(Common::collectHeapStats)()
    {
        _freeMemoryMin = MIN(_freeMemoryMin, freeMemory());
    }

    void __time_critical_func(Common::collectStackStats)()
    {
#if defined(ARDUINO_ARCH_RP2040)
    #ifdef OPENKNX_DUALCORE
        if (rp2040.cpuid())
            _freeStackMin1 = MIN(_freeStackMin1, rp2040.getFreeStack());
        else
    #endif
            _freeStackMin = MIN(_freeStackMin, rp2040.getFreeStack());
#elif defined(ARDUINO_ARCH_ESP32)
    #ifdef OPENKNX_DUALCORE
        if (!xPortGetCoreID())
            _freeStackMin1 = uxTaskGetStackHighWaterMark(nullptr);
        else
    #endif
            _freeStackMin = uxTaskGetStackHighWaterMark(nullptr);
#endif
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
        // Skip if no modules have been added (for testing)
        if (openknx.modules.count == 0) return;

        bool configured = knx.configured();

        uint8_t processed = 0;
        do
        {
            RUNTIME_MEASURE_BEGIN(openknx.modules.runtime[_currentModule]);
            openknx.modules.list[_currentModule]->loop(configured);
            RUNTIME_MEASURE_END(openknx.modules.runtime[_currentModule]);
        }
        while (freeLoopIterate(openknx.modules.count, _currentModule, processed));
    }

#ifdef OPENKNX_DUALCORE
    void Common::loop1()
    {
    #ifdef ARDUINO_ARCH_ESP32
        collectStackStats();
    #endif

        if (!_setup1Ready) return;

    #ifdef OPENKNX_HEARTBEAT
        #ifdef INFO1_LED_PIN
        openknx.info1Led.debugLoop();
        #endif
    #endif

        bool configured = knx.configured();

        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            RUNTIME_MEASURE_BEGIN(openknx.modules.runtime1[i]);
            openknx.modules.list[i]->loop1(configured);
            RUNTIME_MEASURE_END(openknx.modules.runtime1[i]);
        }
    }
#endif

    bool Common::afterStartupDelay()
    {
        return _afterStartupDelay;
    }

    void Common::processAfterStartupDelay()
    {
        if (afterStartupDelay())
            return;

#ifdef BASE_StartupDelayBase
        if (!delayCheck(_startupDelay, ParamBASE_StartupDelayTimeMS))
            return;
#endif

        logInfoP("processAfterStartupDelay");
        logIndentUp();

        _afterStartupDelay = true;

        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->processAfterStartupDelay();
        }

        logIndentDown();
    }

#ifdef BASE_HeartbeatDelayBase
    void Common::processHeartbeat()
    {
        // the first heartbeat is send directly after startup delay of the device
        if (!afterStartupDelay()) return;

        if (_heartbeatDelay == 0 || delayCheck(_heartbeatDelay, ParamBASE_HeartbeatDelayTimeMS))
        {
            uint8_t value = extendedHeartbeatValue;

            // first startup
            if (_firstStartup)
            {
                value |= (1 << 1);
                if (openknx.watchdog.lastReset()) value |= (1 << 2);
            }

            logDebugP("Send Heartbeat %i", value);

            if (ParamBASE_HeartbeatExtended)
            {
                KoBASE_Heartbeat.value(value, DPT_DecimalFactor);
            }
            else
            {
                KoBASE_Heartbeat.value(true, DPT_Switch);
            }

            _firstStartup = false;
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
        openknx.common.skipLooptimeWarning();

        logErrorP("SavePIN triggered!");
        logIndentUp();
        logInfoP("Save power");
        logIndentUp();

        openknx.progLed.powerSave();
#ifdef INFO1_LED_PIN
        openknx.info1Led.powerSave();
#endif
#ifdef INFO1_LED_PIN
        openknx.info1Led.powerSave();
#endif
#ifdef INFO2_LED_PIN
        openknx.info2Led.powerSave();
#endif

#if MASK_VERSION == 0x07B0
        TpUartDataLinkLayer* ddl = knx.bau().getDataLinkLayer();
        ddl->stop(true);
#endif

        // first save all modules to save power before...
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->savePower();

#if MASK_VERSION == 0x07B0 && defined(NCN5120)
        ddl->powerControl(false);
#endif

        logInfoP("Completed (%ims)", millis() - start);
        logIndentDown();

        // save data
        openknx.flash.save();

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

        openknx.common.skipLooptimeWarning();
        logInfoP("Restore power (after 1s)");
        logIndentUp();

        openknx.progLed.powerSave(false);
#ifdef INFO1_LED_PIN
        openknx.info1Led.powerSave(false);
#endif
#ifdef INFO1_LED_PIN
        openknx.info1Led.powerSave(false);
#endif
#ifdef INFO2_LED_PIN
        openknx.info2Led.powerSave(false);
#endif

#if MASK_VERSION == 0x07B0
        TpUartDataLinkLayer* ddl = knx.bau().getDataLinkLayer();
    #ifdef NCN5120
        ddl->powerControl(true);
    #endif
        ddl->stop(false);
#endif

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
            restart();
        }

        logInfoP("Restore power without reboot was successful");

        _savePinTriggered = false;
        _savedPinProcessed = 0;
        logIndentDown();
    }

    void Common::processBeforeRestart()
    {
        logInfoP("processBeforeRestart");
        logIndentUp();
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->processBeforeRestart();
        }

        openknx.watchdog.safeRestart();
        openknx.flash.save();
        logIndentDown();
    }

    void Common::processBeforeTablesUnload()
    {
        logInfoP("processBeforeTablesUnload");
        logIndentUp();
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->processBeforeTablesUnload();
        }

        openknx.flash.save();
        logIndentDown();
    }

#if (MASK_VERSION & 0x0900) != 0x0900 // Coupler do not have GroupObjects
    void Common::processInputKo(GroupObject& ko)
    {
    #ifdef BASE_KoDiagnose
        if (ko.asap() == BASE_KoDiagnose)
            return openknx.console.processDiagnoseKo(ko);
    #endif

    #ifdef BASE_KoManualSave
        if (ko.asap() == BASE_KoManualSave)
            return processSaveKo(ko);
    #endif
        openknx.time.processInputKo(ko);

        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->processInputKo(ko);
        }
    }
#endif

#ifdef BASE_KoManualSave
    void Common::processSaveKo(GroupObject& ko)
    {

        if (ParamBASE_ManualSave && ko.value(DPT_Trigger))
        {
            uint32_t time = 60; // 3
            if (ParamBASE_ManualSave == 2)
                time = 15;
            else if (ParamBASE_ManualSave == 1)
                time = 5;

            if (openknx.flash.lastWrite() == 0 || delayCheck(openknx.flash.lastWrite(), time * 1000 * 60))
            {
                logInfoP("Process incoming save event");
                logIndentUp();
                openknx.flash.save();
                logIndentDown();
            }
            else
            {
                logErrorP("Ignore the incoming save event (write protection %imin)", time);
            }
        }
    }
#endif

    void Common::registerCallbacks()
    {
        // Register Callbacks for FunctionProperty also when knx ist not configured
        knx.bau().functionPropertyCallback([](uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength) -> bool {
            return openknx.common.processFunctionProperty(objectIndex, propertyId, length, data, resultData, resultLength);
        });
        knx.bau().functionPropertyStateCallback([](uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength) -> bool {
            return openknx.common.processFunctionPropertyState(objectIndex, propertyId, length, data, resultData, resultLength);
        });

        // abort if knx not configured
        if (!knx.configured()) return;

        knx.beforeRestartCallback([]() -> void {
            openknx.common.processBeforeRestart();
        });
#if (MASK_VERSION & 0x0900) != 0x0900 // Coupler do not have GroupObjects
        GroupObject::classCallback([](GroupObject& iKo) -> void {
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

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_ESP32)
    int Common::freeStackMin()
    {
        return _freeStackMin;
    }
    #ifdef OPENKNX_DUALCORE
    int Common::freeStackMin1()
    {
        return _freeStackMin1;
    }
    #endif
#endif

    bool Common::processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength)
    {
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            if (openknx.modules.list[i]->processFunctionProperty(objectIndex, propertyId, length, data, resultData, resultLength))
                return true;

        return false;
    }

    bool Common::processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength)
    {
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            if (openknx.modules.list[i]->processFunctionPropertyState(objectIndex, propertyId, length, data, resultData, resultLength))
                return true;

        return false;
    }

    void Common::restart()
    {
        logInfoP("System will restart now");
        delay(10);
        openknx.watchdog.safeRestart();
        knx.platform().restart();
    }

#ifdef OPENKNX_RUNTIME_STAT
    void Common::showRuntimeStat(const bool stat /*= true*/, const bool hist /*= false*/)
    {
        logInfoP("Runtime Statistics: (Uptime=%dms)", millis());
        logIndentUp();
        {
            Stat::RuntimeStat::showStatHeader();
            // Use prefix '_' to preserve structure on sorting
            _runtimeLoop.showStat("___Loop", 0, stat, hist);
            _runtimeConsole.showStat("__Console", 0, stat, hist);
            _runtimeKnxStack.showStat("__KnxStack", 0, stat, hist);
            _runtimeTimeManager.showStat("__Time", 0, stat, hist);
            _runtimeSunCalculation.showStat("__Sun", 0, stat, hist);
            _runtimeModuleLoop.showStat("_All_Modules_Loop", 0, stat, hist);
            for (uint8_t i = 0; i < openknx.modules.count; i++)
            {
                openknx.modules.runtime[i].showStat(openknx.modules.list[i]->name().c_str(), 0, stat, hist);
    #ifdef OPENKNX_DUALCORE
                openknx.modules.runtime1[i].showStat(openknx.modules.list[i]->name().c_str(), 1, stat, hist);
    #endif
            }
        }
        logIndentDown();
    }
#endif

} // namespace OpenKNX
