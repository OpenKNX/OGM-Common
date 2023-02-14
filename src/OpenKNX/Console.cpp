#include "OpenKNX/Console.h"
#include "../HardwareDevices.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    void Console::loop()
    {
        if (SERIAL_DEBUG.available())
            processSerialInput();
    }

    void Console::processSerialInput()
    {

        uint8_t current = SERIAL_DEBUG.read();
        if (_consoleCharLast == current)
        {
            _consoleCharRepeats++;
        }
        else
        {
            _consoleCharLast = current;
            _consoleCharRepeats = 0;
        }

        switch (current)
        {
            case 0x50: // P
                openknx.triggerSavePin();
                break;
            case 0x57: // W
                openknx.flash.save(true);
                break;
            case 0x45: // E
                fatalError(1, "Test fatal error");
                break;
            case 0x68: // h
                showHelp();
                break;
            case 0x69: // i
                showInformations();
                break;
            case 0x0D: // Enter
                openknx.log("");
                break;

#ifdef ARDUINO_ARCH_RP2040
            case 0x6E: // n
                if (_consoleCharRepeats < 3)
                {
                    openknx.log("Nuker", "repeat \"%c\" %ix to nuke flash (knx only)", current, (3 - _consoleCharRepeats));
                    break;
                }
                openknx.nukeFlashKnxOnly();
                break;
            case 0x4E: // N
                if (_consoleCharRepeats < 3)
                {
                    openknx.log("Nuker", "repeat \"%c\" %ix to nuke flash", current, (3 - _consoleCharRepeats));
                    break;
                }
                openknx.nukeFlash();
                break;
#endif
#ifdef WATCHDOG
            case 0x77: // w
                if (!ParamLOG_Watchdog)
                    break;

                if (_consoleCharRepeats < 3)
                {
                    openknx.log("Watchdog", "repeat \"%c\" %ix to trigger watchdog", current, (3 - _consoleCharRepeats));
                    break;
                }

                openknx.log("Watchdog", "wait for %is to trigger watchdog", WATCHDOG_MAX_PERIOD_MS / 1000);
                delay(WATCHDOG_MAX_PERIOD_MS + 1);
                break;
#endif
        }
    }

    void Console::showInformations()
    {
        openknx.log("");
        openknx.log("= OpenKNX Device Information =");
        openknx.log("KNX Address", "%s (%i)", openknx.info.humanIndividualAddress().c_str(), openknx.info.individualAddress());
        openknx.log("Application (ETS)", "Number: %s (%i)  Version: %s (%i)  Configured: %i", openknx.info.humanApplicationNumber().c_str(), openknx.info.applicationNumber(), openknx.info.humanApplicationVersion().c_str(), openknx.info.applicationVersion(), knx.configured());
        openknx.log("Firmware", "Number: %s (%i)  Version: %s (%i)  Name: %s", openknx.info.humanFirmwareNumber().c_str(), openknx.info.firmwareNumber(), openknx.info.humanFirmwareVersion().c_str(), openknx.info.firmwareVersion(), MAIN_OrderNumber);
        openknx.log("Serial number", "00FA:%08X", knx.platform().uniqueSerialNumber());
#ifdef HARDWARE_NAME
        openknx.log("Board", "%s", HARDWARE_NAME);
#endif
        Modules* modules = openknx.getModules();
        char* modulePrefix = new char[12];
        for (uint8_t i = 1; i <= modules->count; i++)
        {
            sprintf(modulePrefix, "Module %i", modules->ids[i - 1]);
            openknx.log(modulePrefix, "Version %s  Name: %s", modules->list[i - 1]->version(), modules->list[i - 1]->name());
        }
        delete[] modulePrefix;
        openknx.log("Free memory", "%.2f KiB (min. %.2f KiB)", ((float)freeMemory() / 1024), ((float)openknx.freeMemoryMin() / 1024));
    }

    void Console::showHelp()
    {
        openknx.log("");
        openknx.log("= OpenKNX Device Console help =");
        openknx.log("P - Trigger Reaction to Save Pin");
        openknx.log("W - Write Userflash");
        openknx.log("E - Test Fatal Error");
        openknx.log("h - Show this help");
        openknx.log("i - Show device information");
#ifdef ARDUINO_ARCH_RP2040
        openknx.log("n - Delete (nuke) Userflash");
        openknx.log("N - Delete (nuke) complete device flash");
#endif
#ifdef WATCHDOG
        openknx.log("w - Trigger Watchdog");
#endif
    }
} // namespace OpenKNX