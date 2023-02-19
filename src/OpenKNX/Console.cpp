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
            case 0x0D: // Enter
                openknx.log("");
                break;
            case 'h':
                showHelp();
                break;
            case 'i':
                showInformations();
                break;
            case 'r':
                knx.platform().restart();
                break;
            case 'p':
                knx.toggleProgMode();
                break;
            case 'w':
                openknx.flash.save(true);
                break;
            case 's':
                sleep();
                break;
#ifdef ARDUINO_ARCH_RP2040
            case 'b':
                resetToBootloader();
                break;
            case 'N':
                if (confirmation(current, 3, "Nuke knwhole flash"))
                    break;

                nukeFlash();
                break;
            case 'n':
                if (confirmation(current, 3, "Nuke KNX settings"))
                    break;

                nukeFlashKnxOnly();
                break;
#endif
            case 'P':
                openknx.triggerSavePin();
                break;
            case 'E':
                if (confirmation(current, 1, "FatalError"))
                    break;

                fatalError(5, "Test with 5x blinking");
                break;
        }
    }

    void Console::showInformations()
    {
        openknx.log("");
        openknx.log("Device Console", "===== Information =====");
        openknx.log("KNX Address", "%s (%i)", openknx.info.humanIndividualAddress().c_str(), openknx.info.individualAddress());
        openknx.log("Application (ETS)", "Number: %s (%i)  Version: %s (%i)  Configured: %i", openknx.info.humanApplicationNumber().c_str(), openknx.info.applicationNumber(), openknx.info.humanApplicationVersion().c_str(), openknx.info.applicationVersion(), knx.configured());
        openknx.log("Firmware", "Number: %s (%i)  Version: %s (%i)  Name: %s", openknx.info.humanFirmwareNumber().c_str(), openknx.info.firmwareNumber(), openknx.info.humanFirmwareVersion().c_str(), openknx.info.firmwareVersion(), MAIN_OrderNumber);
        openknx.log("Serial number", "00FA:%08X", knx.platform().uniqueSerialNumber());
#ifdef HARDWARE_NAME
        openknx.log("Board", "%s", HARDWARE_NAME);
#endif
#ifdef ARDUINO_ARCH_RP2040
        const char* cpuMode = openknx.usesDualCore() ? "Dual-Core" : "Single-Core";

        openknx.log("CPU-Mode", "%s  (Temperature %.1f °C)", cpuMode, openknx.hardware.cpuTemperature());
#endif
        openknx.log("Free memory", "%.2f KiB (min. %.2f KiB)", ((float)freeMemory() / 1024), ((float)openknx.freeMemoryMin() / 1024));

        openknx.log("", "Modules");
        Modules* modules = openknx.getModules();
        char modulePrefix[12];
        for (uint8_t i = 0; i < modules->count; i++)
        {
            sprintf(modulePrefix, "Module %i", modules->ids[i]);
            openknx.log(modulePrefix, "Version %s  Name: %s", modules->list[i]->version().c_str(), modules->list[i]->name().c_str());
        }
        openknx.log("");
    }

    void Console::showHelp()
    {
        openknx.log("");
        openknx.log("Device Console", "===== Help =====");
        openknx.log("", ">  h  <  Show this help");
        openknx.log("", ">  i  <  Show device information");
        openknx.log("", ">  r  <  Restart the device");
        openknx.log("", ">  p  <  Toggle the ProgMode");
        openknx.log("", ">  w  <  Write data to Flash");
        openknx.log("", ">  s  <  Sleep for %ims time", sleepTime());
#ifdef ARDUINO_ARCH_RP2040
        openknx.log("", ">  b  <  Reset into Bootloader Mode");
        openknx.log("", ">  N  <  Delete (nuke) complete device flash");
        openknx.log("", ">  n  <  Delete (nuke) Userflash");
#endif
        openknx.log("", ">  E  <  Trigger a FatalError");
        openknx.log("", ">  P  <  Trigger a powerloss (SavePin)");
        openknx.log("");
    }

    void Console::sleep()
    {
        delay(sleepTime());
    }

    uint32_t Console::sleepTime()
    {
#ifdef WATCHDOG
        return MAX(WATCHDOG_MAX_PERIOD_MS + 1, 20000);
#else
        return 20;
#endif
    }

    bool Console::confirmation(char key, uint8_t repeats, const char* action)
    {
        if (_consoleCharRepeats < repeats)
        {
            openknx.log("", "repeat \"%c\" %ix to trigger \"%s\"", key, (repeats - _consoleCharRepeats), action);
            return true;
        }
        else
        {
            return false;
        }
    }

#ifdef ARDUINO_ARCH_RP2040
    void Console::nukeFlash()
    {
#ifdef WATCHDOG
        Watchdog.enable(2147483647);
#endif
        openknx.log("", "Delete (nuke) complete device flash (%i -> %i)", 0, NUKE_FLASH_SIZE_BYTES);
        __nukeFlash(0, NUKE_FLASH_SIZE_BYTES);
    }

    void Console::nukeFlashKnxOnly()
    {
#ifdef WATCHDOG
        Watchdog.enable(2147483647);
#endif
        openknx.log("", "Delete (nuke) Userflash (%i -> %i)", KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
        __nukeFlash(KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
    }

    void Console::resetToBootloader()
    {
        reset_usb_boot(0, 0);
    }
#endif
} // namespace OpenKNX