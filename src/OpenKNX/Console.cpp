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
                openknx.logger.log("");
                break;
            case 'h':
                showHelp();
                break;
            case 'i':
                showInformations();
                break;
            case 'r':
                delay(10);
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
            case 'f':
                showFilesystem();
                break;
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
        openknx.logger.log("");
        openknx.logger.color(4);
        openknx.logger.log("Device Console", "===== Information =====");
        openknx.logger.color(0);
        openknx.logger.log("KNX Address", "%s (%i)", openknx.info.humanIndividualAddress().c_str(), openknx.info.individualAddress());
        openknx.logger.log("Application (ETS)", "Number: %s (%i)  Version: %s (%i)  Configured: %i", openknx.info.humanApplicationNumber().c_str(), openknx.info.applicationNumber(), openknx.info.humanApplicationVersion().c_str(), openknx.info.applicationVersion(), knx.configured());
        openknx.logger.log("Firmware", "Number: %s (%i)  Version: %s (%i)  Name: %s", openknx.info.humanFirmwareNumber().c_str(), openknx.info.firmwareNumber(), openknx.info.humanFirmwareVersion().c_str(), openknx.info.firmwareVersion(), MAIN_OrderNumber);
        openknx.logger.log("Serial number", "00FA:%08X", knx.platform().uniqueSerialNumber());
#ifdef HARDWARE_NAME
        openknx.logger.log("Board", "%s", HARDWARE_NAME);
#endif
#ifdef ARDUINO_ARCH_RP2040
        const char* cpuMode = openknx.usesDualCore() ? "Dual-Core" : "Single-Core";

        openknx.logger.log("CPU-Mode", "%s (Temperature %.1f °C)", cpuMode, openknx.hardware.cpuTemperature());
#endif
        openknx.logger.log("Free memory", "%.2f KiB (min. %.2f KiB)", ((float)freeMemory() / 1024), ((float)openknx.freeMemoryMin() / 1024));

        Modules* modules = openknx.getModules();
        char modulePrefix[12];
        for (uint8_t i = 0; i < modules->count; i++)
        {
            sprintf(modulePrefix, "Module %i", modules->ids[i]);
            openknx.logger.log(modulePrefix, "%s (%s)", modules->list[i]->name().c_str(), modules->list[i]->version().c_str());
        }
        openknx.logger.log("");
    }

#ifdef ARDUINO_ARCH_RP2040
    void Console::showFilesystem()
    {
        LittleFS.begin();
        openknx.logger.color(4);
        openknx.logger.log("Device Console", "===== Filesystem =====");
        openknx.logger.color(0);
        showFilesystemDirectory("/");
    }

    void Console::showFilesystemDirectory(std::string path)
    {
        openknx.logger.log("Filesystem", "%s", path.c_str());

        Dir directory = LittleFS.openDir(path.c_str());

        while (directory.next())
        {
            std::string full = path + directory.fileName().c_str();
            if (directory.isDirectory())
                showFilesystemDirectory(full + "/");
            else
            {
                openknx.logger.log("Filesystem", "%s (%i bytes)", full.c_str(), directory.fileSize());
            }
        }
    }
#endif

    void Console::showHelp()
    {
        openknx.logger.log("");
        openknx.logger.color(4);
        openknx.logger.log("Device Console", "===== Help =====");
        openknx.logger.color(0);
        openknx.logger.log("", ">  h  <  Show this help");
        openknx.logger.log("", ">  i  <  Show device information");
#ifdef ARDUINO_ARCH_RP2040
        openknx.logger.log("", ">  f  <  Show filesystem");
#endif
        openknx.logger.log("", ">  r  <  Restart the device");
        openknx.logger.log("", ">  p  <  Toggle the ProgMode");
        openknx.logger.log("", ">  w  <  Write data to Flash");
        openknx.logger.log("", ">  s  <  Sleep for %ims time", sleepTime());
#ifdef ARDUINO_ARCH_RP2040
        openknx.logger.log("", ">  b  <  Reset into Bootloader Mode");
        openknx.logger.log("", ">  N  <  Delete (nuke) complete device flash");
        openknx.logger.log("", ">  n  <  Delete (nuke) Userflash");
#endif
        openknx.logger.log("", ">  E  <  Trigger a FatalError");
        openknx.logger.log("", ">  P  <  Trigger a powerloss (SavePin)");
        openknx.logger.log("");
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
            openknx.logger.log("", "repeat \"%c\" %ix to trigger \"%s\"", key, (repeats - _consoleCharRepeats), action);
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
        openknx.logger.log("", "Delete (nuke) complete device flash (%i -> %i)", 0, NUKE_FLASH_SIZE_BYTES);
        __nukeFlash(0, NUKE_FLASH_SIZE_BYTES);
    }

    void Console::nukeFlashKnxOnly()
    {
#ifdef WATCHDOG
        Watchdog.enable(2147483647);
#endif
        openknx.logger.log("", "Delete (nuke) Userflash (%i -> %i)", KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
        __nukeFlash(KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
    }

    void Console::resetToBootloader()
    {
        reset_usb_boot(0, 0);
    }
#endif
} // namespace OpenKNX