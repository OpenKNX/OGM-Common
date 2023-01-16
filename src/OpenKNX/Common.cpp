#include "OpenKNX/Common.h"

namespace OpenKNX
{
    Common::Common()
    {
        flashUserData = new FlashUserData();
    }
    Common::~Common()
    {}

    void Common::init(uint8_t firmwareRevision)
    {
        this->firmwareRevision = firmwareRevision;
        SERIAL_DEBUG.begin(115200);
        SERIAL_DEBUG.println("init...");
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

    uint16_t Common::version()
    {
        return ((firmwareRevision & 0x1F) << 11) | ((MAIN_ApplicationVersion & 0xF0) << 2) | (MAIN_ApplicationVersion & 0x0F);
    }

    void Common::initKnx()
    {
        SERIAL_DEBUG.println("knx init...");
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
        knx.bau().deviceObject().version(version());

        if (MAIN_OrderNumber)
        {
            knx.orderNumber((const uint8_t*)MAIN_OrderNumber); // set the OrderNumber
        }
    }

    VersionCheckResult Common::versionCheck(uint16_t manufacturerId, uint8_t* hardwareType, uint16_t firmwareVersion)
    {
        VersionCheckResult check = FlashAllInvalid;
        if (manufacturerId == 0x00FA)
        {
            // hardwareType has the format 0x00 00 Ap nn vv 00
            if (memcmp(knx.bau().deviceObject().hardwareType(), hardwareType, 4) == 0)
            {
                check = FlashTablesInvalid;
                if (knx.bau().deviceObject().hardwareType()[4] == hardwareType[4])
                {
                    check = FlashValid;
                }
                else
                {
                    println("MAIN_ApplicationVersion changed, ETS has to reprogram the application!");
                }
            }
        }
        else
        {
            println("This firmware supports only applicationId 0x00FA");
        }
        return check;
    }

    void Common::setup()
    {
        SERIAL_DEBUG.println("setup...");
        flashUserData->load();

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

        startupDelay = millis();
        heartbeatDelay = 0;

        // Handle loop of modules
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->setup();
        }

        registerCallbacks();
    }

    // main loop
    void Common::loop()
    {
        uint32_t start = millis();

        // loop  knx stack
        knx.loop();

        // loop  appstack
        appLoop();

        if (SERIAL_DEBUG.available())
            processSerialInput();

#ifdef DEBUG_LOOP_TIME
        // loop took to long and last out is min 1s ago
        if (delayCheck(start, DEBUG_LOOP_TIME) && delayCheck(lastDebugTime, 1000))
        {
            SERIAL_DEBUG.printf("loop took too long %i\n\r", (millis() - start));
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

    void Common::processFirstLoop()
    {
        // skip if already executed
        if (firstLoopProcessed)
            return;

        SERIAL_DEBUG.println("processFirstLoop");

        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->firstLoop();
        }

        firstLoopProcessed = true;
    }

    void Common::processModulesLoop()
    {
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->loop();
        }
    }

    void Common::addModule(Module* module)
    {
        modules.count++;
        modules.list[modules.count - 1] = module;
    }

    Modules* Common::getModules()
    {
        return &modules;
    }

#ifdef LOG_StartupDelayBase
    bool Common::processStartupDelay()
    {
        return !delayCheck(startupDelay, getDelayPattern(LOG_StartupDelayBase));
    }
#endif

#ifdef LOG_HeartbeatDelayBase
    void Common::processHeartbeat()
    {
        // the first heartbeat is send directly after startup delay of the device
        if (heartbeatDelay == 0 || delayCheck(heartbeatDelay, getDelayPattern(LOG_HeartbeatDelayBase)))
        {
            // we waited enough, let's send a heartbeat signal
            knx.getGroupObject(LOG_KoHeartbeat).value(true, getDPT(VAL_DPT_1));
            heartbeatDelay = millis();
        }
    }
#endif

    void Common::processSavePin()
    {
        // savePin not triggered
        if (!save)
            return;

        // saveHandler already called
        if (saved)
            return;

        SERIAL_DEBUG.println("processSavePin");
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->processSavePin();
        }

        flashUserData->save();
        //flashUserData->save(true);
        saved = true;
    }

    void Common::processBeforeRestart()
    {
        SERIAL_DEBUG.println("processBeforeRestart");
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->processBeforeRestart();
        }

        flashUserData->save();
    }

    void Common::processBeforeTablesUnload()
    {
        SERIAL_DEBUG.println("processBeforeTablesUnload");
        flashUserData->save();
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
                openknx.save = true;
            },
            FALLING);
#endif
    }

    void Common::processSerialInput()
    {
        std::stringstream output;
        switch (SERIAL_DEBUG.read())
        {
            case 0x57: // W
                flashUserData->save(true);
                break;
            case 0x56: // V
                output << MAIN_ApplicationNumber << "." << MAIN_ApplicationVersion << "." << firmwareRevision;
                debug("VERSION", output.str().c_str());
                break;
            case 0x4F: // O
                output << "0x" << std::hex << std::uppercase << MAIN_OpenKnxId;
                debug("OPENKNX", output.str().c_str());
                // SERIAL_DEBUG.printf("OPENKNX: 0x%02X\n\r", MAIN_OpenKnxId);
                break;
            case 0x53: // S
                output << MAIN_OrderNumber;
                debug("SOFTWARE", output.str().c_str());
                break;
            case 0x48: // H
                output << HARDWARE_NAME;
                debug("HARDWARE", output.str().c_str());
                break;
        }
    }

    int Common::debug(const char* prefix, const char* output, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, output);
        int result = vsnprintf(buffer, 256, output, args);
        va_end(args);
        SERIAL_DEBUG.printf("%s: %s\n\r", prefix, buffer);
        return result;
    }

} // namespace OpenKNX

OpenKNX::Common openknx;