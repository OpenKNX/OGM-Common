#include "OpenKNX/Common.h"

namespace OpenKNX
{
    Common::Common()
    {
    }
    Common::~Common()
    {}

    void Common::init(uint8_t firmwareRevision)
    {
        _firmwareRevision = firmwareRevision;
        SERIAL_DEBUG.begin(115200);
        debug("OpenKNX", "init");
        ArduinoPlatform::SerialDebug = &SERIAL_DEBUG;

        pinMode(PROG_LED_PIN, OUTPUT);
        digitalWrite(PROG_LED_PIN, HIGH);
#ifdef DEBUG_DELAY
        delay(DEBUG_DELAY);
#endif

#ifdef INFO_LED_PIN
        pinMode(INFO_LED_PIN, OUTPUT);
        ledInfo(true);
#endif

        initKnx();
    }

    void Common::initKnx()
    {
        debug("OpenKNX", "init knx");
#ifdef ARDUINO_ARCH_RP2040
        debug("OpenKNX", "test");
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
        knx.bau().deviceObject().version(applicationVersion());

        if (MAIN_OrderNumber)
        {
            knx.orderNumber((const uint8_t*)MAIN_OrderNumber); // set the OrderNumber
        }
    }

    VersionCheckResult Common::versionCheck(uint16_t manufacturerId, uint8_t* hardwareType, uint16_t firmwareVersion)
    {
        if (manufacturerId != 0x00FA)
        {
            openknx.debug("OpenKNX", "This firmware supports only applicationId 0x00FA");
            return FlashAllInvalid;
        }

        // hardwareType has the format 0x00 00 Ap nn vv 00
        if (memcmp(knx.bau().deviceObject().hardwareType(), hardwareType, 4) != 0)
        {
            openknx.debug("OpenKNX", "MAIN_ApplicationVersion changed, ETS has to reprogram the application!");
            return FlashAllInvalid;
        }

        if (knx.bau().deviceObject().hardwareType()[4] != hardwareType[4])
        {
            openknx.debug("OpenKNX", "MAIN_ApplicationVersion changed, ETS has to reprogram the application!");
            return FlashTablesInvalid;
        }

        return FlashValid;
    }

    void Common::setup()
    {
        debug("OpenKNX", "setup");

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

#ifdef INFO_LED_PIN
        ledInfo(false);
#endif
    }

    void Common::appSetup()
    {
        if (!knx.configured())
            return;

        flash.load();

#ifdef LOG_StartupDelayBase
        _startupDelay = millis();
#endif
#ifdef LOG_HeartbeatDelayBase
        _heartbeatDelay = 0;
#endif
        collectMemoryStats();

        // Handle loop of modules
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->setup();
            collectMemoryStats();
        }

        // register callbacks
        registerCallbacks();
    }

    // main loop
    void Common::loop()
    {

#ifdef DEBUG_LOOP_TIME
        uint32_t start = millis();
#endif

        // loop  knx stack
        knx.loop();
        collectMemoryStats();

        // loop  appstack
        appLoop();

        if (SERIAL_DEBUG.available())
            processSerialInput();

        collectMemoryStats();

#ifdef DEBUG_LOOP_TIME
        // loop took to long and last out is min 1s ago
        if (delayCheck(start, DEBUG_LOOP_TIME) && delayCheck(lastDebugTime, 1000))
        {
            debug("OpenKNX", "loop took too long %i", (millis() - start));
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

#ifdef LOG_StartupDelayBase
        // Handle Startup delay
        if (processStartupDelay())
            return;
#endif

#ifdef LOG_HeartbeatDelayBase
        // Handle heartbeat delay
        processHeartbeat();
#endif

        processSavePin();
        processFirstLoop();
        processModulesLoop();
    }

    void Common::loopModule(uint8_t index)
    {
        modules.list[index]->loop();
        collectMemoryStats();
    }

    void Common::collectMemoryStats()
    {
        _freeMemoryMin = MIN((uint)freeMemory(), _freeMemoryMin);
        _freeMemoryMax = MAX((uint)freeMemory(), _freeMemoryMax);
    }

    void Common::showMemoryStats()
    {
        collectMemoryStats();
        debug("OpenKNX", "Free Memory\n\r         current: %i\n\r         min: %i\n\r         max: %i\n\r", freeMemory(), _freeMemoryMin, _freeMemoryMax);
    }

    void Common::processFirstLoop()
    {
        // skip if already executed
        if (_firstLoopProcessed)
            return;

        debug("OpenKNX", "processFirstLoop");

        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->firstLoop();
        }

        _firstLoopProcessed = true;
    }

    void Common::processModulesLoop()
    {
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->loop();
        }
    }

    void Common::addModule(uint8_t id, Module* module)
    {
        modules.count++;
        modules.list[modules.count - 1] = module;
        modules.ids[modules.count - 1] = id;
    }

    Module* Common::getModule(uint8_t id)
    {
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            if (modules.ids[i - 1] == id)
                return modules.list[i - 1];
        }

        return nullptr;
    }

    Modules* Common::getModules()
    {
        return &modules;
    }

#ifdef LOG_StartupDelayBase
    bool Common::processStartupDelay()
    {
        return !delayCheck(_startupDelay, ParamLOG_StartupDelayTimeMS);
    }
#endif

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

    void Common::processSavePin()
    {
        // savePin not triggered
        if (!_save)
            return;

        // saveHandler already called
        if (_saved)
            return;

        debug("OpenKNX", "processSavePin");
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->processSavePin();
        }

        flash.save();
        _saved = true;
    }

    void Common::processBeforeRestart()
    {
        debug("OpenKNX", "processBeforeRestart");
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->processBeforeRestart();
        }

        flash.save();
    }

    void Common::processBeforeTablesUnload()
    {
        debug("OpenKNX", "processBeforeTablesUnload");
        flash.save();
    }

    void Common::processInputKo(GroupObject& iKo)
    {
        // SERIAL_DEBUG.printf("hook onInputKo %i\n\r", iKo.asap());
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->processInputKo(iKo);
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
                openknx._save = true;
            },
            FALLING);
#endif
    }

    void Common::processSerialInput()
    {
        switch (SERIAL_DEBUG.read())
        {
            case 0x57: // W
                flash.save(true);
                break;
            case 0x41: // A
                debug("APPLICATION", "%02X%02X", openknx.openKnxId(), openknx.applicationNumber());
                break;
            case 0x56: // V
                debug("VERSION", "%i (%s)", openknx.applicationVersion(), openknx.applicationHumanVersion());
                break;
            case 0x53: // S
                debug("SOFTWARE", "%s", MAIN_OrderNumber);
                break;
            case 0x48: // H
                debug("HARDWARE", "%s", HARDWARE_NAME);
                break;
            case 0x6D: // m
                showMemoryStats();
                break;
        }
    }

    uint8_t Common::openKnxId()
    {
        return MAIN_OpenKnxId;
    }

    uint8_t Common::applicationNumber()
    {
        return MAIN_ApplicationNumber;
    }

    uint16_t Common::applicationVersion()
    {
        return ((_firmwareRevision & 0x1F) << 11) | ((MAIN_ApplicationVersion & 0xF0) << 2) | (MAIN_ApplicationVersion & 0x0F);
    }

    const char* Common::applicationHumanVersion()
    {
        char* buffer = new char[20];
        sprintf(buffer, "%i.%i.%i", ((MAIN_ApplicationVersion & 0xF0) >> 4), (MAIN_ApplicationVersion & 0x0F), _firmwareRevision);
        return buffer;
    }
} // namespace OpenKNX

OpenKNX::Common openknx;