#include "OpenKNX/Console.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    void Console::loop()
    {
        if (SERIAL_DEBUG.available())
            processSerialInput();
    }

    void Console::processCommand(std::string cmd)
    {
        if (cmd == "info" || cmd == "i")
        {
            showInformations();
        }
        else if (cmd == "help" || cmd == "h")
        {
            showHelp();
        }
        else if (cmd == "versions" || cmd == "v")
        {
            showVersions();
        }
        else if (cmd == "mem")
        {
            showMemory();
        }
        else if (cmd == "prog" || cmd == "p")
        {
            knx.toggleProgMode();
        }
        else if (cmd == "sleep")
        {
            sleep();
        }
        else if (cmd == "restart" || cmd == "r")
        {
            delay(20);
            knx.platform().restart();
        }
        else if (cmd == "fatal")
        {
            openknx.hardware.fatalError(5, "Test with 5x blinking");
        }
        else if (cmd == "powerloss")
        {
            openknx.common.triggerSavePin();
        }
        else if (cmd == "save" || cmd == "s" || cmd == "w")
        {
            openknx.flash.save();
        }

#ifdef ARDUINO_ARCH_RP2040
        else if (cmd == "files" || cmd == "fs")
        {
            showFilesystem();
        }
        else if (cmd == "bootloader")
        {
            resetToBootloader();
        }
        else if (cmd == "erase knx")
        {
            erase(EraseMode::KnxFlash);
        }
        else if (cmd == "erase openknx")
        {
            erase(EraseMode::OpenKnxFlash);
        }
        else if (cmd == "erase files")
        {
            erase(EraseMode::Filesystem);
        }
        else if (cmd == "erase all")
        {
            erase(EraseMode::All);
        }
#endif
        else
        {
            // check modules for command
            for (uint8_t i = 0; i < openknx.modules.count; i++)
                if (openknx.modules.list[i]->processCommand(cmd, false))
                    return;

            // Command not found;
            openknx.logger.log("unknown command");
        }
    }

    void Console::processSerialInput()
    {
        const uint8_t current = SERIAL_DEBUG.read();
        if (current == '\n')
        {
            openknx.logger.log(prompt);

            if (strlen(prompt) > 0)
                processCommand(prompt);

            memset(prompt, 0, 15); // Reset Promptbuffer
        }

        if (current == '\b' && strlen(prompt) > 0)
            prompt[strlen(prompt) - 1] = 0x0;

        if (strlen(prompt) < 14 && current >= 32 && current <= 126) // Max. 14 printables chars allowed
            prompt[strlen(prompt)] = current;

        openknx.logger.printPrompt();
    }

    void Console::showInformations()
    {
        openknx.logger.mutex_block();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("════════════════════════ Information ═══════════════════════════════════════════");
        openknx.logger.color(0);
        openknx.logger.log("KNX Address", "%s", openknx.info.humanIndividualAddress().c_str());
        openknx.logger.log("Application (ETS)", "Number: %s  Version: %s  Configured: %i", openknx.info.humanApplicationNumber().c_str(), openknx.info.humanApplicationVersion().c_str(), knx.configured());
        openknx.logger.log("Firmware", "Number: %s  Version: %s  Name: %s", openknx.info.humanFirmwareNumber().c_str(), openknx.info.humanFirmwareVersion().c_str(), MAIN_OrderNumber);
        openknx.logger.log("Serial number", "00FA:%08X", knx.platform().uniqueSerialNumber());
#ifdef HARDWARE_NAME
        openknx.logger.log("Board", "%s", HARDWARE_NAME);
#endif
#ifdef ARDUINO_ARCH_RP2040
        const char* cpuMode = openknx.common.usesDualCore() ? "Dual-Core" : "Single-Core";

        openknx.logger.log("CPU-Mode", "%s (Temperature %.1f °C)", cpuMode, openknx.hardware.cpuTemperature());
#endif
        showMemory();

        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->showInformations();

        openknx.logger.log("────────────────────────────────────────────────────────────────────────────────");
        openknx.logger.log("");
        openknx.logger.mutex_unblock();
    }

#ifdef ARDUINO_ARCH_RP2040
    void Console::showFilesystem()
    {
        openknx.logger.mutex_block();
        LittleFS.begin();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("════════════════════════ Filesystem ════════════════════════════════════════════");

        openknx.logger.color(0);
        showFilesystemDirectory("/");
        openknx.logger.log("────────────────────────────────────────────────────────────────────────────────");
        openknx.logger.mutex_unblock();
    }

    void Console::showFilesystemDirectory(std::string path)
    {
        openknx.logger.mutex_block();
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
        openknx.logger.mutex_unblock();
    }
#endif

    void Console::showVersions()
    {
        openknx.logger.mutex_block();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("════════════════════════ Versions ══════════════════════════════════════════════");
        openknx.logger.color(0);

        openknx.logger.log("KNX", KNX_Version);
        openknx.logger.log("Firemware", MAIN_Version);
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.logger.log(openknx.modules.list[i]->name().c_str(), openknx.modules.list[i]->version().c_str());

        openknx.logger.log("────────────────────────────────────────────────────────────────────────────────");
        openknx.logger.mutex_unblock();
    }

    void Console::showHelp()
    {
        openknx.logger.mutex_block();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("════════════════════════ Help ══════════════════════════════════════════════════");
        openknx.logger.color(0);
        openknx.logger.log("Command(s)               Description");
        printHelpLine("help, h", "Show this help");
        printHelpLine("info, i", "Show generel information");
        printHelpLine("version, v", "Show compiled versions");
        printHelpLine("mem", "Show memory usage");
#ifdef ARDUINO_ARCH_RP2040
        printHelpLine("files, fs", "Show files on filesystem");
#endif
        printHelpLine("restart, r", "Restart the device");
        printHelpLine("prog, p", "Toggle the ProgMode");
        printHelpLine("save, s, w", "Save data in Flash");
        printHelpLine("sleep", "Sleep for up to 20 seconds");
        printHelpLine("fatal", "Trigger a FatalError");
        printHelpLine("powerloss", "Trigger a powerloss (SavePin)");
#ifdef ARDUINO_ARCH_RP2040
        printHelpLine("erase knx", "Erase knx parameters");
        printHelpLine("erase openknx", "Erase opeknx module data");
        printHelpLine("erase files", "Erase filesystem");
        printHelpLine("erase all", "Erase all");
        printHelpLine("bootloader", "Reset into Bootloader Mode");
#endif
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->showHelp();

        openknx.logger.log("────────────────────────────────────────────────────────────────────────────────");
        openknx.logger.mutex_unblock();
    }

    void Console::sleep()
    {
        openknx.logger.log("sleep up to 20 seconds");
        delay(sleepTime());
    }

    uint32_t Console::sleepTime()
    {
#ifdef OPENKNX_WATCHDOG
        return MAX(WATCHDOG_MAX_PERIOD_MS + 1, 20000);
#else
        return 20000;
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
    void Console::erase(EraseMode mode)
    {
#ifdef OPENKNX_WATCHDOG
        Watchdog.enable(2147483647);
#endif

        openknx.progLed.blinking();
        openknx.infoLed.off();
        openknx.hardware.stopKnxMode();

        if (mode == EraseMode::All || mode == EraseMode::KnxFlash)
        {
            openknx.logger.log("Erase", "KNX parameters (%i -> %i)", KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
            __nukeFlash(KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
        }

        if (mode == EraseMode::All || mode == EraseMode::OpenKnxFlash)
        {
            openknx.logger.log("Erase", "OpenKNX save data (%i -> %i)", OPENKNX_FLASH_OFFSET, OPENKNX_FLASH_SIZE);
            __nukeFlash(OPENKNX_FLASH_OFFSET, OPENKNX_FLASH_SIZE);
        }

        if (mode == EraseMode::All || mode == EraseMode::Filesystem)
        {
            openknx.logger.log("Erase", "Format Filesystem");
            LittleFS.begin();
            if (LittleFS.format())
            {
                openknx.logger.log("Erase", "Succesful");
            }
        }

        if (mode == EraseMode::All)
        {
            openknx.logger.log("Erase", "First bytes of Firmware");
            __nukeFlash(0, 4096);
        }

        openknx.progLed.forceOn();
        openknx.logger.log("Erase", "Completed");
        delay(1000);
        openknx.logger.log("Restart device");
        delay(100);
        knx.platform().restart();
    }

    void Console::resetToBootloader()
    {
        reset_usb_boot(0, 0);
    }

    void Console::printHelpLine(const char* command, const char* message)
    {
        // TODO Beautify
        openknx.logger.log(command, message);
    }

    void Console::showMemory() {
        openknx.logger.log("Free memory", "%.3f KiB (min. %.3f KiB)", ((float)freeMemory() / 1024), ((float)openknx.common.freeMemoryMin() / 1024));
    }
#endif
} // namespace OpenKNX