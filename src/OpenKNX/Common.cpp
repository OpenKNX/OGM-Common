#include "OpenKNX/Common.h"
#include "hardware.h"
#include <knx.h>

namespace OpenKNX
{
    Common::Common()
    {}
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
        knx.bau().deviceObject().version(((firmwareRevision & 0x1F) << 11) | ((MAIN_ApplicationVersion & 0xF0) << 2) | (MAIN_ApplicationVersion & 0x0F));

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
        digitalWrite(PROG_LED_PIN, LOW);

        // pin or GPIO the programming led is connected to. Default is LED_BUILDIN
        knx.ledPin(PROG_LED_PIN);
        // is the led active on HIGH or low? Default is LOW
        knx.ledPinActiveOn(PROG_LED_PIN_ACTIVE_ON);
        // pin or GPIO programming button is connected to. Default is 0
        knx.buttonPin(PROG_BUTTON_PIN);
        // Is the interrup created in RISING or FALLING signal? Default is RISING
        // knx.buttonPinInterruptOn(PROG_BUTTON_PIN_INTERRUPT_ON);

        // Handle loop of modules
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->setup();
        }

        // start the framework
        knx.start();
#ifdef INFO_LED_PIN
        ledInfo(false);
#endif

        registerCallbacks();
    }

    void Common::loop()
    {
        uint32_t start = millis();

        processSaveHandler();

        // knx is not configured
        // if (!knx.configured())
        //     return;

#ifdef LOG_StartupDelayBase
        // Handle Startup delay
        if (processStartupDelay())
            return;
#endif

#ifdef LOG_HeartbeatDelayBase
        // Handle heartbeat delay
        processHeartbeat();
#endif

        // Trigger afterSetup once (used for readrequest etc)
        if (!firstLoop)
        {
            for (uint8_t i = 1; i <= modules.count; i++)
            {
                modules.list[i - 1]->firstLoop();
            }
            firstLoop = true;
        }

        // Handle loop of modules
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->loop();
        }

        // loop  knx stack
        knx.loop();

        if (SERIAL_DEBUG.available())
            processSerialInput();

#ifdef DEBUG_LOOP_TIME
        if (delayCheck(start) > DEBUG_LOOP_TIME)
            SERIAL_DEBUG.printf("loop took too long %i\n\r", (millis() - start));
#endif
    }

    void Common::addModule(Module* module)
    {
        modules.count++;
        modules.list[modules.count - 1] = module;
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

    void Common::processSaveHandler()
    {
        // savePin not triggered
        if (!save)
            return;
        // saveHandler already called
        if (saved)
            return;

        SERIAL_DEBUG.println("processSaveHandler");
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->processSaveHandler();
        }

        FlashUserData::onSafePinInterruptHandler();
        saved = true;
    }
    void Common::processBeforeRestart()
    {
        SERIAL_DEBUG.println("processBeforeRestart");
        for (uint8_t i = 1; i <= modules.count; i++)
        {
            modules.list[i - 1]->processBeforeRestart();
        }

        FlashUserData::onBeforeRestartHandler();
    }
    void Common::processBeforeTablesUnload()
    {
        SERIAL_DEBUG.println("processBeforeTablesUnload");

        FlashUserData::onBeforeTablesUnloadHandler();
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
            case 0x56: // V
                output << MAIN_ApplicationNumber << "." << MAIN_ApplicationVersion << "."
                       << firmwareRevision;
                SERIAL_DEBUG.print("VERSION: ");
                SERIAL_DEBUG.print(output.str().c_str());
                SERIAL_DEBUG.println("");
                break;
            case 0x4F: // O
                SERIAL_DEBUG.printf("OPENKNX: 0x%02X\n\r", MAIN_OpenKnxId);
                break;
            case 0x53: // S
                output << MAIN_OrderNumber;
                SERIAL_DEBUG.print("SOFTWARE: ");
                SERIAL_DEBUG.print(output.str().c_str());
                SERIAL_DEBUG.println("");
                break;
            case 0x48: // H
                output << HARDWARE_NAME;
                SERIAL_DEBUG.print("HARDWARE: ");
                SERIAL_DEBUG.print(output.str().c_str());
                SERIAL_DEBUG.println("");
                break;
        }
    }

} // namespace OpenKNX

OpenKNX::Common openknx;

// namespace OpenKNX
// {

//   bool Common::loop()
//   {

//     // Trigger afterSetup once (used for readrequest etc)
//     if (!calledAfterSetup)
//     {
//       for (uint8_t i = 1; i <= modules.count; i++)
//       {
//         modules.list[i - 1]->afterSetup();
//       }
//       calledAfterSetup = true;
//     }

//     if (calledSaveInterrupt)
//     {
//       SERIAL_DEBUG.println("execute save handling");
//       for (uint8_t i = 1; i <= modules.count; i++)
//       {
//         modules.list[i - 1]->onSafePinInterruptHandler();
//       }
//       // TODO SaveToFlash

//       // Then reboot
//       watchdog_reboot(0, 0, 0);
//       calledSaveInterrupt = true;
//     }

//     if (!knx.configured())
//       return false;

//       // Handle Startup delay
// #ifdef LOG_StartupDelayBase
//     if (processStartupDelay())
//       return false;
// #endif

//       // Handle heartbeat delay
// #ifdef LOG_HeartbeatDelayBase
//     processHeartbeat();
// #endif

//     for (uint8_t i = 1; i <= modules.count; i++)
//     {
//       modules.list[i - 1]->loop();
//     }

//     return true;
//   }

//   bool Common::setup()
//   {
//     if (!knx.configured())
//       return false;

//     startupDelay = millis();
//     heartbeatDelay = 0;

//     for (uint8_t i = 1; i <= modules.count; i++)
//     {
//       modules.list[i - 1]->setup();
//     }

//     return true;
//   }

//   void Common::onSafePinInterruptHandler()
//   {
//     SERIAL_DEBUG.println("hook onSafePinInterruptHandler");
//     calledSaveInterrupt = true;
//   }

//   void Common::onBeforeRestartHandler()
//   {
//     SERIAL_DEBUG.println("hook onBeforeRestartHandler");
//   }

//   void Common::onBeforeTablesUnloadHandler()
//   {
//     SERIAL_DEBUG.println("hook onBeforeTablesUnloadHandler");
//   }

//   void Common::processInputKo(GroupObject &iKo)
//   {
//     // SERIAL_DEBUG.printf("hook onInputKo %i\n\r", iKo.asap());
//     for (uint8_t i = 1; i <= modules.count; i++)
//     {
//       modules.list[i - 1]->processInputKo(iKo);
//     }
//   }

//   void Common::registerCallbacks()
//   {
//     knx.beforeRestartCallback(onBeforeRestartHandler);
//     // GroupObject::classCallback(processInputKo);
//     TableObject::beforeTablesUnloadCallback(onBeforeTablesUnloadHandler);
// #ifdef SAVE_INTERRUPT_PIN
//     // we need to do this as late as possible, tried in constructor, but this doesn't work on RP2040
//     pinMode(SAVE_INTERRUPT_PIN, INPUT);
//     SERIAL_DEBUG.printf("SAVE %i\n\r", digitalPinToInterrupt(SAVE_INTERRUPT_PIN));
//     attachInterrupt(digitalPinToInterrupt(SAVE_INTERRUPT_PIN), onSafePinInterruptHandler, FALLING);
// #endif
//   }

//   void Common::log(const char *iModuleName, const char *iMessage)
//   {
//     // TODO Timestamp?
//     SERIAL_DEBUG.print(iModuleName);
//     SERIAL_DEBUG.print(": ");
//     SERIAL_DEBUG.print(iMessage);
//     SERIAL_DEBUG.println("");
//   }

//   void Common::processSerialInput()
//   {
//     std::stringstream lStringHelper;
//     switch (SERIAL_DEBUG.read())
//     {
//     case 0x56: // V
//       // TODO firmwareRevision
//       lStringHelper << MAIN_MAIN_ApplicationNumber << "." << MAIN_MAIN_ApplicationVersion << "." << "0";
//       log("VERSION", lStringHelper.str().c_str());
//       break;
//     case 0x4F: // O
//       lStringHelper << "0x" << std::hex << std::uppercase << MAIN_MAIN_OpenKnxId;
//       log("OPENKNX", lStringHelper.str().c_str());
//       break;
//     case 0x53: // S
//       lStringHelper << MAIN_OrderNumber;
//       log("SOFTWARE", lStringHelper.str().c_str());
//       break;
//     case 0x48: // H
//       lStringHelper << MAIN_Hardware;
//       log("HARDWARE", lStringHelper.str().c_str());
//       break;
//     }
//   }

// }

// FlashUserData* OpenKNXfacade::flashUserData()
// {
//     return _flashUserDataPtr;
// }

// void OpenKNXfacade::loop() {
//     _flashUserDataPtr->loop();
//     knx.loop();
// }

// void OpenKNXfacade::readMemory(uint8_t MAIN_OpenKnxId, uint8_t MAIN_ApplicationNumber, uint8_t MAIN_ApplicationVersion, uint8_t firmwareRevision, const char* MAIN_OrderNumber /*= nullptr*/)
// {
//     OpenKNX::knxRead(MAIN_OpenKnxId, MAIN_ApplicationNumber, MAIN_ApplicationVersion, firmwareRevision, MAIN_OrderNumber);
// }
