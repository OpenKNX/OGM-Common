#include "OpenKNX/Common.h"
#include "OpenKNX/Facade.h"
#if defined(OPENKNX_HEAP_INTEGRITY_TRACE) && defined(ARDUINO_ARCH_ESP32) // ESP-only: heap_caps_* is ESP-IDF
    #include "OpenKNX/Heap/HeapCheck.h"
#endif
#include "OpenKNX/Stat/RuntimeStat.h"
#ifdef DEVICE_DISPLAY_MODULE
    // Bare includes resolve via the OFM-DeviceDisplay src/ include path.
    #include "DeviceDisplay.h"
    #include "OpenKNX/Widgets/WidgetKnxBcu.h"
    #include "OpenKNX/Widgets/WidgetSysInfo.h"
#endif
#ifdef ARDUINO_ARCH_RP2040
    #include <USB.h>
    #include <device/usbd.h>
    #include <class/msc/msc.h>
    #include <tusb-msc.h>
// LDF pulls in tusb-msc regardless of #ifdef guards. Without OPENKNX_USB_MSC
    #if !defined(OPENKNX_USB_MSC) || defined(OPENKNX_DEBUGGER)
// these stubs satisfy the missing callbacks at link time.
extern "C"
{
    uint8_t tud_msc_get_maxlun_cb(void) { return 0; }
    void tud_msc_inquiry_cb(uint8_t, uint8_t[8], uint8_t[16], uint8_t[4]) {}
    void tud_msc_capacity_cb(uint8_t, uint32_t* block_count, uint16_t* block_size) { *block_count = 0; *block_size = 0; }
    bool tud_msc_test_unit_ready_cb(uint8_t) { return false; }
    bool tud_msc_start_stop_cb(uint8_t, uint8_t, bool, bool) { return false; }
    bool tud_msc_is_writable_cb(uint8_t) { return false; }
    int32_t tud_msc_read10_cb(uint8_t, uint32_t, uint32_t, void*, uint32_t) { return -1; }
    int32_t tud_msc_write10_cb(uint8_t, uint32_t, uint32_t, uint8_t*, uint32_t) { return -1; }
    void tud_msc_write10_complete_cb(uint8_t) {}
    int32_t tud_msc_scsi_cb(uint8_t, uint8_t const[16], void*, uint16_t) { return -1; }
}
    #endif
#endif

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
#ifndef OPENKNX_DEBUGGER
    #ifdef ARDUINO_ARCH_RP2040
        USB.disconnect();
        USB.setManufacturer("OpenKNX");
        #ifdef FIRMWARE_NAME
        USB.setProduct(FIRMWARE_NAME);
        #endif

        #ifdef OPENKNX_USB_MSC
        uint8_t epIn = USB.registerEndpointIn();
        uint8_t epOut = USB.registerEndpointOut();
        static uint8_t msd_desc[] = {TUD_MSC_DESCRIPTOR(1 /* placeholder */, 0, epOut, epIn, CFG_TUD_MSC_EP_BUFSIZE)};
        USB.registerInterface(1, USBClass::simpleInterface, msd_desc, sizeof(msd_desc), 2, 0);
        #endif
        USB.connect();
    #endif
#endif
        openknx.logger.init();

#ifdef DEVICE_INIT
        DEVICE_INIT();
#endif
        ArduinoPlatform::SerialDebug = new OpenKNX::Log::VirtualSerial("KNX");

        openknx.timerInterrupt.init();
        openknx.gpio.init();
        openknx.leds.init();
        openknx.ledFunctions.init();
#if defined(SAVE_POWER_PIN) && SAVE_POWER_PIN >= 0
        openknx.gpio.pinMode(SAVE_POWER_PIN, OUTPUT);
        openknx.gpio.digitalWrite(SAVE_POWER_PIN, SAVE_POWER_PIN_POWER_ON);
#endif

        _progLedFunc = openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_PROG);
        _stateLedFunc = openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_STATE);

#if defined(PROG_BUTTON_PIN) && PROG_BUTTON_PIN >= 0 && OPENKNX_RECOVERY_TIME > 0
        processRecovery();
#endif

        openknx.hardware.initButtons();

        _progLedFunc->color(Led::Color::Blue);
        _progLedFunc->pulsing();

        debugWait();

        logInfoP("Init firmware");

#ifdef OPENKNX_DEBUG
        showDebugInfo();
#endif

        openknx.hardware.initFlash();
        openknx.info.serialNumber(knx.platform().uniqueSerialNumber());
        openknx.info.firmwareRevision(firmwareRevision);

        if (openknx.watchdog.lastReset()) logErrorP("Restarted by watchdog");
        openknx.watchdog.activate();
        openknx.watchdog.fastCheck();

        initKnx();

#ifdef OPENKNX_WATCHDOG
        if (knx.configured() && !ParamBASE_Watchdog)
            openknx.watchdog.deactivate();
#endif

        openknx.hardware.init();
        _ledFunctions.init();
    }

#ifdef OPENKNX_DEBUG
    void Common::showDebugInfo()
    {
        logDebugP("Debug logging is enabled!");
    #if defined(OPENKNX_TRACE)
        logDebugP("Trace logging is enabled with:");
        logIndentUp();
        logDebugP("Filter: %s", TRACE_STRINGIFY(OPENKNX_TRACE));
        logIndentDown();
    #endif
    }
#endif

#if defined(PROG_BUTTON_PIN) && PROG_BUTTON_PIN >= 0 && OPENKNX_RECOVERY_TIME > 0
    void Common::processRecovery()
    {
        bool erase = false;
    #ifndef PROG_BUTTON_PIN_MODE
        #define PROG_BUTTON_PIN_MODE INPUT_PULLUP
    #endif
        openknx.gpio.pinMode(PROG_BUTTON_PIN, PROG_BUTTON_PIN_MODE);
        while (!openknx.gpio.digitalRead(PROG_BUTTON_PIN))
        {
            if (millis() >= OPENKNX_RECOVERY_TIME)
            {
                if (!erase)
                {
                    _progLedFunc->color(Led::Color::Red);
                    _progLedFunc->blinking(200);
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

        _progLedFunc->off();
    }
#endif

    void Common::initKnx()
    {
        logInfoP("Init knx stack");
        logIndentUp();
#if MASK_VERSION == 0x07B0 or MASK_VERSION == 0x091A
        openknx.hardware.initKnxInterface();
#endif
        openknx.progButton.onShortClick([] { knx.toggleProgMode(); });

        knx.ledPin(0);
        knx.setProgLedOnCallback([] { openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_PROG)->forceOn(true); });
        knx.setProgLedOffCallback([] { openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_PROG)->forceOn(false); });

        uint8_t hardwareType[LEN_HARDWARE_TYPE] = {0x00, 0x00, MAIN_OpenKnxId, MAIN_ApplicationNumber, MAIN_ApplicationVersion, 0x00};

        // first setup flash version check
        knx.bau().versionCheckCallback(versionCheck);
        // set correct hardware type for flash compatibility check
        knx.bau().deviceObject().hardwareType(hardwareType);
        // read flash data
        knx.readMemory();
#if defined(OPENKNX_HEAP_INTEGRITY_TRACE) && defined(ARDUINO_ARCH_ESP32)
        OPENKNX_HEAPCHK("post knx.readMemory"); // tripwire: did the KNX-lib table restore corrupt the heap?
#endif
        // set hardware type again, in case an other hardware type was deserialized from flash
        knx.bau().deviceObject().hardwareType(hardwareType);
        // set firmware version as user info (PID_VERSION)
        // 5 bit revision, 5 bit major, 6 bit minor
        // output in ETS as [revision] major.minor
        knx.bau().deviceObject().version(openknx.info.firmwareVersion());
#ifdef OPENKNX_OVERRIDE_MASK_VERSION
        // set mask version returned by the stack regardless of the mask version used for the build
        knx.bau().deviceObject().maskVersion(OPENKNX_OVERRIDE_MASK_VERSION);
#endif
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
#if OPENKNX_WAIT_FOR_SERIAL > 1 && !defined(OPENKNX_RTT) && defined(SERIAL_DEBUG)
        uint32_t timeoutBase = millis();
        while (!SERIAL_DEBUG)
        {
            if (delayCheck(timeoutBase, OPENKNX_WAIT_FOR_SERIAL))
                break;
        }
#endif
    }

    void Common::setup()
    {
        openknx.ledFunctions.setup();

#if defined(OPENKNX_HEAP_INTEGRITY_TRACE) && defined(ARDUINO_ARCH_ESP32)
        OPENKNX_HEAPCHK("setup baseline"); // heap state entering setup (after knx.readMemory)
#endif

        // Handle init of modules
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->init();

        openknx.ledFunctions.setup();

        bool configured = knx.configured();
        openknx.time.setup(configured);

#ifdef OPENKNX_TimeProvider
        if (!openknx.time.hasTimerProvder() && !ParamBASE_InternalTime)
            openknx.time.setTimeProvider(new OPENKNX_TimeProvider());
#endif
#ifdef BASE_StartupDelayBase
        _startupDelay = millis();
#endif

        // Handle setup of modules
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->setup(configured);
#if defined(OPENKNX_HEAP_INTEGRITY_TRACE) && defined(ARDUINO_ARCH_ESP32)
            OPENKNX_HEAPCHK((std::string("setup ") + openknx.modules.list[i]->name()).c_str());
#endif
        }

        _ledFunctions.setup(); // run after setup of all modules, because some modules might add leds to functions

        if (configured) openknx.flash.load();
#if defined(OPENKNX_HEAP_INTEGRITY_TRACE) && defined(ARDUINO_ARCH_ESP32)
        OPENKNX_HEAPCHK("post flash.load"); // tripwire: did a module's readFlash() overrun on a stale blob?
#endif

        // start the framework + isr if needed
        knx.start();
        // openknx.hardware.initKnxRxISR();

        // register callbacks
        registerCallbacks();

        // setup0 is done
        _setup0Ready = true;

#ifdef OPENKNX_WATCHDOG
        // Mark a healthy, configured boot: from here on a watchdog/panic reset is a
        // RUNTIME crash (e.g. IP flood, heap exhaustion), not an unloadable config -
        // so it must not let the auto-erase wipe the KNX config (see Watchdog ctor).
        if (configured) openknx.watchdog.markSetupCompleted();
#endif

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

#ifndef OPENKNX_DUALCORE
        afterSetupLedStatus();
#endif

        if (!knx.configured()) // fallback if unconfigured
        {
            openknx.gpio.showInitResults();
            openknx.console.showInformations();
        }
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

        afterSetupLedStatus();
    }
#endif

    void Common::afterSetupLedStatus()
    {
        _progLedFunc->off();
        _progLedFunc->color(Led::Color::Red);

        if (knx.configured())
        {
            _stateLedFunc->color(Led::Color::Yellow);
            _stateLedFunc->on(Led::Capability::COLOR);
        }
        else
        {
#if MASK_VERSION == 0x091A
            uint8_t count = knx.individualAddress() == 0xFF00 ? 2 : 1;
#else
            uint8_t count = knx.individualAddress() == 0xFFFF ? 2 : 1;
#endif
            _progLedFunc->flash(count, 3000);

            _stateLedFunc->color(Led::Color::Orange);
            _stateLedFunc->flash(count, 3000);
        }
    }

    // main loop
    void Common::loop()
    {
#ifdef ARDUINO_ARCH_ESP32
        collectStackStats();
#endif

#ifdef OPENKNX_LOOPTIME_WARNING
        _skipLooptimeWarning = false;
#endif

        uptime(false);

        if (!_setup0Ready) return;
#ifdef OPENKNX_DUALCORE
        if (!_setup1Ready) return;
#endif

        RUNTIME_MEASURE_BEGIN(_runtimeLoop);

#ifdef OPENKNX_HEARTBEAT
        _progLedFunc->debugLoop();
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

#ifdef OPENKNX_LOOPTIME_WARNING
        uint32_t startStack = millis();
#endif
        // loop  knx stack
        RUNTIME_MEASURE_BEGIN(_runtimeKnxStack);
        knx.loop();
        RUNTIME_MEASURE_END(_runtimeKnxStack);
#ifdef OPENKNX_LOOPTIME_WARNING
        uint32_t endStack = millis();
#endif

        // loop IO
        RUNTIME_MEASURE_BEGIN(_runtimeGPIO);
        openknx.gpio.loop();
        RUNTIME_MEASURE_END(_runtimeGPIO);

        // loop timemanager helper
        RUNTIME_MEASURE_BEGIN(_runtimeTimeManager);
        openknx.time.loop();
        RUNTIME_MEASURE_END(_runtimeTimeManager);

#ifdef ParamBASE_Latitude
        // loop sun-timemanager helper
        RUNTIME_MEASURE_BEGIN(_runtimeSunCalculation);
        openknx.sun.loop();
        RUNTIME_MEASURE_END(_runtimeSunCalculation);
#endif

        _ledFunctions.loop();

        // LED Manager loop - must ALWAYS run for LED effects (PULSING, BLINK, etc.)
        // Also handles pending I2C writes depending on pattern
        openknx.leds.loop();

#if defined(OPENKNX_WIRE_PIO) || defined(OPENKNX_WIRE1_PIO)
    #ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
        // Process I2C queue in Main Loop (2x for Display/LED performance)
        // Only for PIO I2C - native Wire has no queue
        OPENKNX_GPIO_WIRE.processQueue();
    #endif
#endif

        // loop  appstack
        _loopMicros = micros();

        // knx is configured
        bool configured = knx.configured();
#ifdef OPENKNX_DUALCORE
        // Update cache for Core1 (volatile ensures visibility)
        _knxConfiguredCache = configured;
#endif

        if (configured)
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

#ifdef OPENKNX_LOOPTIME_WARNING
        // loop took to long and last output is at least 1s ago
        if (!_skipLooptimeWarning && delayCheck(start, OPENKNX_LOOPTIME_WARNING) && delayCheck(_lastLooptimeWarning, OPENKNX_LOOPTIME_WARNING_INTERVAL))
        {
            logWarningP("Warning: The loop took longer than usual (%i >= %i); stack %i", (millis() - start), OPENKNX_LOOPTIME_WARNING, (endStack - startStack));
            _lastLooptimeWarning = millis();
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
#ifdef OPENKNX_LOOPTIME_WARNING
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
        if (_stateLedFunc != nullptr) _stateLedFunc->debugLoop();
    #endif

        // Read cached configured state from Core0 (volatile ensures visibility)
        bool configured = _knxConfiguredCache;

        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            RUNTIME_MEASURE_BEGIN(openknx.modules.runtime1[i]);
            openknx.modules.list[i]->loop1(configured);
            RUNTIME_MEASURE_END(openknx.modules.runtime1[i]);
        }
    }
#endif

#ifdef DEVICE_DISPLAY_MODULE
    void Common::registerCommonWidgets()
    {
        // Guard against double-add on a second call or boot phase.
        if (_commonWidgetsRegistered) return;
        _commonWidgetsRegistered = true;

        // tryAddWidget() null-checks the WidgetsManager and, when the display is absent
        // at runtime, deletes the widget (no leak) instead of dereferencing null.
        auto* sysInfo = new WidgetSysInfo(8000, WidgetFlags::DefaultWidget);
        sysInfo->setName("System-Info");
        if (!openknxDisplayModule.tryAddWidget(sysInfo))
            logDebugP("WidgetSysInfo not registered (no display/manager)");

        // KNX / BCU widget only makes sense on a device that HAS a TP-UART BCU:
        // the TP device (0x07B0) or the IP-Router coupler (0x091A). Skip on IP-only.
    #if MASK_VERSION == 0x07B0 || MASK_VERSION == 0x091A
        auto* knxBcu = new WidgetKnxBcu(8000, WidgetFlags::DefaultWidget);
        knxBcu->setName("KNX / BCU");
        if (!openknxDisplayModule.tryAddWidget(knxBcu))
            logDebugP("WidgetKnxBcu not registered (no display/manager)");
    #endif
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

        logDebugP("processAfterStartupDelay");
        openknx.gpio.showInitResults();
        openknx.console.showInformations();
        openknx.logger.log("Type \"help\" to view a list of available commands.");
        logIndentUp();

        _afterStartupDelay = true;

        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            openknx.modules.list[i]->processAfterStartupDelay();
        }

#ifdef DEVICE_DISPLAY_MODULE
        // Registered here because DeviceDisplay::init()/setup() has already run, so the
        // widget manager is valid. Runs once (guarded) and no-ops if the display is absent.
        registerCommonWidgets();
#endif

        logIndentDown();

        _stateLedFunc->color(Led::Color::Green);
        _stateLedFunc->on();
    }

#ifdef BASE_HeartbeatDelayBase
    void Common::processHeartbeat()
    {
    // check thermal warning
    #if MASK_VERSION == 0x07B0
        TPUart::SystemState& systemState = knx.bau().getDataLinkLayer()->getTPUart().getSystemState();
        if (systemState.thermalWarning())
            extendedHeartbeatValue |= (1 << 3);
        else
            extendedHeartbeatValue &= ~(1 << 3);
    #endif

        // the first heartbeat is send directly after startup delay of the device
        if (!afterStartupDelay()) return;

        uint8_t value = extendedHeartbeatValue;

        // first startup
        if (_firstStartup)
        {
            value |= (1 << 1);
            if (openknx.watchdog.lastReset()) value |= (1 << 2);
            _firstStartup = false;
        }

        if (ParamBASE_HeartbeatExtended)
        {
            if (KoBASE_Heartbeat.valueCompare(value, DPT_DecimalFactor) || delayCheck(_heartbeatDelay, ParamBASE_HeartbeatDelayTimeMS))
            {
                logDebugP("Send Heartbeat (0x%02X)", value);
                KoBASE_Heartbeat.value(value, DPT_DecimalFactor);
                knx.loop();
                value &= ~(1 << 1); // clear first startup bit
                value &= ~(1 << 2); // clear last reset bit
                KoBASE_Heartbeat.valueNoSend(value, DPT_DecimalFactor);
                _heartbeatDelay = millis();
            }
        }
        else
        {
            if (_heartbeatDelay == 0 || delayCheck(_heartbeatDelay, ParamBASE_HeartbeatDelayTimeMS))
            {
                logDebugP("Send Heartbeat");
                KoBASE_Heartbeat.value(true, DPT_Switch);
                _heartbeatDelay = millis();
            }
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

        openknx.leds.powerSave();

#if MASK_VERSION == 0x07B0
        TpUartDataLinkLayer* dll = knx.bau().getDataLinkLayer();
        dll->stop(true);
#endif

        // first save all modules to save power before...
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->savePower();

#if MASK_VERSION == 0x07B0
        dll->powerControl(false);
#endif

#if defined(SAVE_POWER_PIN) && SAVE_POWER_PIN >= 0
        logInfoP("Shut off aux power with pin %i", SAVE_POWER_PIN);
        openknx.gpio.digitalWrite(SAVE_POWER_PIN, SAVE_POWER_PIN_POWER_OFF);
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

        openknx.leds.powerSave(false);

#if MASK_VERSION == 0x07B0
        TpUartDataLinkLayer* dll = knx.bau().getDataLinkLayer();
        dll->powerControl(true);
        dll->stop(false);
#endif

#if defined(SAVE_POWER_PIN) && SAVE_POWER_PIN >= 0
        logInfoP("Switch on aux power with pin %i", SAVE_POWER_PIN);
        openknx.gpio.digitalWrite(SAVE_POWER_PIN, SAVE_POWER_PIN_POWER_ON);
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

        openknx.flash.save();
        logIndentDown();
        logInfoP("System will restart now");
        delay(10);
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
        openknx.gpio.pinMode(SAVE_INTERRUPT_PIN, INPUT);
        openknx.gpio.attachInterrupt(
            SAVE_INTERRUPT_PIN,
            [this](openknx_gpio_number_t pin, bool state) -> void { openknx.common.triggerSavePin(); }, FALLING);
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

    // this function allows to use a normal property function with lower APDU.
    // It allows receive data in small apdu packages,
    // then calls the target property function with the full dataset,
    // and finally sends back resultData as packages
    bool Common::processFunctionPropertyWrapper(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength)
    {
        const uint8_t sequenceStart = 2;

        bool result = false;
        // first Byte is command (0, 1) or sequence number (<=-2 send data, >=2 receive data )
        int8_t cmd = (int8_t)data[0];
        if (cmd == 0)
        {
            // receive header information for data transmission
            _apduLength = data[1];                    // available APDU length
            _packageCount = (data[2] << 8) + data[3]; // number of expected packages
            _sequenceNumber = sequenceStart;          // first expected sequence number
            _receivedLength = 0;                      // init length counter
            resultData[0] = 0;                        // ok
            resultData[1] = _sequenceNumber;          // tells the client the next sequence number
            resultLength = 2;
            result = true;
        }
        else if (cmd == 1)
        {
            // execute target function property
            uint8_t targetObjectIndex = data[1];
            uint8_t targetPropertyId = data[2];
            uint8_t tmpResultLength = 0;
            // call function property
            result = processFunctionProperty(targetObjectIndex, targetPropertyId, _receivedLength, _receivedData, _resultData, tmpResultLength);
            // send result to the client
            _resultLength = tmpResultLength;
            resultData[0] = !result;                     // success indicator
            resultData[1] = (_resultLength >> 8) & 0xFF; // reserved high byte for longer results;
            resultData[2] = _resultLength & 0xFF;        // result length
            resultData[3] = -sequenceStart;              // first sequence number to send
            resultLength = 4;
            result = true;
        }
        else if (cmd >= sequenceStart)
        {
            // sequence number >=2, we receive data
            // ETS sends a package, data[0] contains sequence number
            if (cmd == _sequenceNumber)
            {
                // we got correct sequence number, increase for next one
                _sequenceNumber++;
                if (_sequenceNumber >= 127)
                {
                    // max sequence number reached
                    resultData[0] = 2;
                    resultLength = 1;
                    return false;
                }
                // receive package
                memcpy(_receivedData + _receivedLength, data + 1, length - 1);
                _receivedLength += (length - 1);
                result = true;
                // check if all data received
                if (--_packageCount == 0)
                {
                    _sequenceNumber = -sequenceStart;
                }
                // create answer
                resultData[0] = (bool)!result;   // ok
                resultData[1] = _sequenceNumber; // tells the client the next sequence number
            }
            else
            {
                // request correct sequence number again
                resultData[0] = 1;               // wrong sequence number
                resultData[1] = _sequenceNumber; // tells the client the last correct to resend
                result = true;
            }
            resultLength = 2;
        }
        else if (cmd <= -sequenceStart)
        {
            // check if all data sent
            if (_resultLength < 1)
            {
                // finish marker
                resultData[0] = 0;
                resultLength = 1;
                return true;
            }
            // ETS wants to receive a package, data[0] contains sequence number
            resultData[0] = data[0]; // return the requested sequence number
            // calculate data offset based on apdu and sequenceNumber
            uint16_t resultOffset = (~cmd + 1 - sequenceStart) * (_apduLength - 1);
            // send package
            resultLength = (_apduLength < (_resultLength + 1) ? _apduLength : (_resultLength + 1));
            memcpy(resultData + 1, _resultData + resultOffset, resultLength - 1);
            _resultLength -= (resultLength - 1);
            result = true;
        }
        return result;
    }

    bool Common::processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength)
    {
        // Unsupported ETS Modules
        if (objectIndex == 0x9E && propertyId == 2)
        {
            resultData[0] = 0; // status OK
            resultData[1] = (_unsupportedEtsModules >> 0) & 0xFF;
            resultData[2] = (_unsupportedEtsModules >> 8) & 0xFF;
            resultData[3] = (_unsupportedEtsModules >> 16) & 0xFF;
            resultData[4] = (_unsupportedEtsModules >> 24) & 0xFF;
            resultLength = 5;
            return true; // handled
        }
        else if (objectIndex == 0x9E && propertyId == 3)
        {
            return processFunctionPropertyWrapper(objectIndex, propertyId, length, data, resultData, resultLength);
        }

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
        openknx.common.processBeforeRestart(); // this function also saves the flash
        knx.platform().restart();
    }

    void Common::unsupportedEtsModule(uint8_t etsModuleId)
    {
        if (etsModuleId > 33) return;
        _unsupportedEtsModules |= (1 << etsModuleId - 2);
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
            _runtimeGPIO.showStat("__GPIO", 0, stat, hist);
            _runtimeTimeManager.showStat("__Time", 0, stat, hist);
    #ifdef ParamBASE_Latitude
            _runtimeSunCalculation.showStat("__Sun", 0, stat, hist);
    #endif
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
