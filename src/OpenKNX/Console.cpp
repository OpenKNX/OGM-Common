#include "OpenKNX/Console.h"
#include "OpenKNX/Facade.h"
#include "OpenKNX/Flash/Driver.h"

#ifdef ARDUINO_ARCH_RP2040
    #include "LittleFS.h"
#endif

namespace OpenKNX
{
    void Console::loop()
    {
        if (OPENKNX_LOGGER_DEVICE.available())
            processSerialInput();
    }

#ifdef LOG_KoDiagnose
    void Console::writeDiagenoseKo(const char* message, ...)
    {
        va_list values;
        va_start(values, message);

        char buffer[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Last Byte muss Zero!
        uint8_t len = vsnprintf(buffer, 15, message, values);

        if (len >= 15)
            openknx.hardware.fatalError(FATAL_SYSTEM, "BufferOverflow: writeDiagenoseKo message too long");

        va_end(values);

        _diagnoseKoOutput = true;
        KoLOG_Diagnose.value(buffer, Dpt(16, 1));
        knx.loop();
        _diagnoseKoOutput = false;
    }

    void Console::processDiagnoseKo(GroupObject& ko)
    {
        // prevent the nested call by the output on the diagnose ko
        if (_diagnoseKoOutput)
            return;

        // prevent empty command
        if (ko.valueRef()[0] == '\0')
            return;

        openknx.logger.logWithPrefixAndValues("DiagnoseKO", "command \"%s\" received", ko.valueRef());
        logIndentUp();

        if (!processCommand((char*)ko.valueRef(), true))
            openknx.logger.logWithPrefix("DiagnoseKO", "command not found");

        logIndentDown();
    }
#endif

    bool Console::processCommand(std::string cmd, bool diagnoseKo /* = false */)
    {
        if (!diagnoseKo && (cmd == "i" || cmd == "info"))
        {
            showInformations();
        }
        else if (!diagnoseKo && (cmd == "h" || cmd == "help"))
        {
            showHelp();
        }
        else if (cmd == "v" || cmd == "versions")
        {
            showVersions();
        }
        else if (cmd == "m" || cmd == "mem" || cmd == "memory")
        {
            showMemory(diagnoseKo);
        }
        else if (!diagnoseKo && (cmd == "p" || cmd == "prog"))
        {
            knx.toggleProgMode();
        }
        else if (!diagnoseKo && (cmd == "sleep"))
        {
            sleep();
        }
        else if (!diagnoseKo && (cmd == "r" || cmd == "restart"))
        {
            delay(20);
            knx.platform().restart();
        }
        else if (!diagnoseKo && (cmd == "fatal"))
        {
            openknx.hardware.fatalError(5, "Test with 5x blinking");
        }
        else if (!diagnoseKo && (cmd == "powerloss"))
        {
            openknx.common.triggerSavePin();
        }
        else if (cmd == "s" || cmd == "w" || cmd == "save")
        {
            openknx.flash.save();
        }
        else if (cmd == "flash knx")
        {
            showMemoryContent(openknx.hardware.knxFlash()->flashAddress(), openknx.hardware.knxFlash()->size());
        }
        else if (cmd == "flash openknx")
        {
            showMemoryContent(openknx.hardware.openknxFlash()->flashAddress(), openknx.hardware.openknxFlash()->size());
        }
        else if (cmd.substr(0, 6) == "mem 0x" && cmd.length() > 6)
        {
            std::string addrstr = cmd.substr(6, cmd.length() - 6);
            uint32_t addr = std::stoi(addrstr, nullptr, 16);
            showMemoryContent((uint8_t*)addr, 0x40);
        }
#ifdef OPENKNX_RUNTIME_STAT
        else if (!diagnoseKo && (cmd == "runtime"))
        {
            openknx.common.showRuntimeStat();
        }
#endif
#ifdef ARDUINO_ARCH_RP2040
        else if (!diagnoseKo && (cmd == "fs" || cmd == "files"))
        {
            showFilesystem();
        }
        else if (!diagnoseKo && (cmd == "file dummy"))
        {
            File file = LittleFS.open("dummy.dummy", "a");
            file.seek(rp2040.hwrand32());
            file.write("DUMMY");
            file.close();
            showFilesystem();
        }
        else if (!diagnoseKo && (cmd == "bootloader"))
        {
            resetToBootloader();
        }
        else if (!diagnoseKo && (cmd == "erase knx"))
        {
            erase(EraseMode::KnxFlash);
        }
        else if (!diagnoseKo && (cmd == "erase openknx"))
        {
            erase(EraseMode::OpenKnxFlash);
        }
        else if (!diagnoseKo && (cmd == "erase files"))
        {
            erase(EraseMode::Filesystem);
        }
        else if (!diagnoseKo && (cmd == "erase all"))
        {
            erase(EraseMode::All);
        }
#endif
        else
        {
            // check modules for command
            for (uint8_t i = 0; i < openknx.modules.count; i++)
                if (openknx.modules.list[i]->processCommand(cmd, diagnoseKo))
                    return true;
            return false;
        }
        return true;
    }

    void Console::processSerialInput()
    {
        const uint8_t current = OPENKNX_LOGGER_DEVICE.read();
        if (current == '\r' || current == '\n')
        {
            if (_consoleCharLast == '\r' && current == '\n')
            {
                _consoleCharLast = current;
                return;
            }

            openknx.logger.log(prompt);
            if (strlen(prompt) > 0)
            {
                if (!processCommand(prompt))
                {
                    // Command not found
                    openknx.logger.logWithValues("%s: command not found", prompt);
                }
            }
            memset(prompt, 0, 15); // Reset Promptbuffer
        }

        if (current == '\b' && strlen(prompt) > 0)
            prompt[strlen(prompt) - 1] = 0x0;

        if (strlen(prompt) < 14 && current >= 32 && current <= 126) // Max. 14 printables chars allowed
            prompt[strlen(prompt)] = current;

        openknx.logger.printPrompt();
        _consoleCharLast = current;
    }

    void Console::showInformations()
    {
        logBegin();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("======================== Information ===========================================");
        openknx.logger.color(0);
        openknx.logger.logWithPrefixAndValues("KNX Address", "%s", openknx.info.humanIndividualAddress().c_str());
        openknx.logger.logWithPrefixAndValues("Application (ETS)", "Number: %s  Version: %s  Configured: %i", openknx.info.humanApplicationNumber().c_str(), openknx.info.humanApplicationVersion().c_str(), knx.configured());
        openknx.logger.logWithPrefixAndValues("Firmware", "Number: %s  Version: %s  Name: %s", openknx.info.humanFirmwareNumber().c_str(), openknx.info.humanFirmwareVersion().c_str(), MAIN_OrderNumber);
        openknx.logger.logWithPrefixAndValues("Serial number", "00FA:%08X", knx.platform().uniqueSerialNumber());
#ifdef HARDWARE_NAME
        openknx.logger.logWithPrefixAndValues("Board", "%s", HARDWARE_NAME);
#endif

#ifdef OPENKNX_DUALCORE
        const char* cpuMode = openknx.usesDualCore() ? "Dual-Core" : "Single-Core";
#else
        const char* cpuMode = "Single-Core";
#endif

        if (openknx.hardware.cpuTemperature() > 0)
            openknx.logger.logWithPrefixAndValues("CPU-Mode", "%s (Temperature %.1f °C)", cpuMode, openknx.hardware.cpuTemperature());
        else
            openknx.logger.logWithPrefixAndValues("CPU-Mode", "%s", cpuMode);

        showMemory();

        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->showInformations();

        openknx.logger.log("--------------------------------------------------------------------------------");
        openknx.logger.log("");
        logEnd();
    }

#ifdef ARDUINO_ARCH_RP2040
    void Console::showFilesystem()
    {
        logBegin();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("======================== Filesystem ============================================");

        openknx.logger.color(0);
        showFilesystemDirectory("/");
        openknx.logger.log("--------------------------------------------------------------------------------");
        logEnd();
    }

    void Console::showFilesystemDirectory(std::string path)
    {
        logBegin();
        openknx.logger.logWithPrefixAndValues("Filesystem", "%s", path.c_str());

        Dir directory = LittleFS.openDir(path.c_str());

        while (directory.next())
        {
            std::string full = path + directory.fileName().c_str();
            if (directory.isDirectory())
                showFilesystemDirectory(full + "/");
            else
            {
                openknx.logger.logWithPrefixAndValues("Filesystem", "%s (%i bytes)", full.c_str(), directory.fileSize());
            }
        }
        logEnd();
    }
#endif

    void Console::showVersions()
    {
        logBegin();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("======================== Versions ==============================================");
        openknx.logger.color(0);

        openknx.logger.logWithPrefix("KNX", KNX_Version);
        openknx.logger.logWithPrefix("Firmware", MAIN_Version);
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.logger.logWithPrefix(openknx.modules.list[i]->name().c_str(), openknx.modules.list[i]->version().c_str());

        openknx.logger.log("--------------------------------------------------------------------------------");
        logEnd();
    }

    void Console::showHelp()
    {
        logBegin();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("======================== Help ==================================================");
        openknx.logger.color(0);
        openknx.logger.log("Command(s)               Description");
        printHelpLine("help, h", "Show this help");
        printHelpLine("info, i", "Show general information");
        printHelpLine("version, v", "Show compiled versions");
        printHelpLine("memory, mem", "Show memory usage");
        printHelpLine("mem 0xXXXXXXXX", "Show memory content (64byte) starting at 0xXXXXXXXX");
        printHelpLine("flash knx", "Show knx flash content");
        printHelpLine("flash openknx", "Show openknx flash content");
#ifdef ARDUINO_ARCH_RP2040
        printHelpLine("files, fs", "Show files on filesystem");
#endif
#ifdef OPENKNX_RUNTIME_STAT
        printHelpLine("runtime", "Show runtime statistics");
#endif
        printHelpLine("restart, r", "Restart the device");
        printHelpLine("prog, p", "Toggle the ProgMode");
        printHelpLine("save, s, w", "Save data in Flash");
        printHelpLine("sleep", "Sleep for up to 20 seconds");
        printHelpLine("fatal", "Trigger a FatalError");
        printHelpLine("powerloss", "Trigger a PowerLoss (SavePin)");
#ifdef ARDUINO_ARCH_RP2040
        printHelpLine("erase knx", "Erase knx parameters");
        printHelpLine("erase openknx", "Erase opeknx module data");
        printHelpLine("erase files", "Erase filesystem");
        printHelpLine("erase all", "Erase all");
        printHelpLine("bootloader", "Reset into Bootloader Mode");
#endif
        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->showHelp();

        openknx.logger.log("--------------------------------------------------------------------------------");
        logEnd();
    }

    void Console::sleep()
    {
        openknx.logger.log("sleep up to 20 seconds");
        delay(sleepTime());
    }

    uint32_t Console::sleepTime()
    {
#ifdef OPENKNX_WATCHDOG
        return MAX(OPENKNX_WATCHDOG_MAX_PERIOD + 1, 20000);
#else
        return 20000;
#endif
    }

    void Console::showMemory(bool diagnoseKo /* = false */)
    {
#ifdef LOG_KoDiagnose
        if (diagnoseKo)
        {
            openknx.console.writeDiagenoseKo("MIN %.3fKiB", ((float)openknx.common.freeMemoryMin() / 1024));
            openknx.console.writeDiagenoseKo("CUR %.3fKiB", ((float)freeMemory() / 1024));
        }
#endif
        openknx.logger.logWithPrefixAndValues("Free memory", "%.3f KiB (min. %.3f KiB)", ((float)freeMemory() / 1024), ((float)openknx.common.freeMemoryMin() / 1024));
    }

    void Console::showMemoryContent(uint8_t* start, uint32_t size)
    {
        logBegin();
        openknx.logger.logWithPrefixAndValues("Memory content", "Address 0x%08X - Size: 0x%04X (%d bytes)", start, size, size);
        for (uint32_t i = 0; i < floor(size / 16); i++)
        {
            const uint8_t* ptr = start + (i * 16);
            char prefix[24] = {};
            snprintf(prefix, 24, "0x%06X (0x%08X)", (uint)(i * 16), (uint)ptr);
            openknx.logger.logHexWithPrefix(prefix, ptr, 16);
        }
        logEnd();
    }

    void Console::printHelpLine(const char* command, const char* message)
    {
        // TODO Beautify
        openknx.logger.logWithPrefix(command, message);
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
            openknx.logger.logWithPrefixAndValues("Erase", "KNX parameters (%i -> %i)", KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
            __nukeFlash(KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
        }

        if (mode == EraseMode::All || mode == EraseMode::OpenKnxFlash)
        {
            openknx.logger.logWithPrefixAndValues("Erase", "OpenKNX save data (%i -> %i)", OPENKNX_FLASH_OFFSET, OPENKNX_FLASH_SIZE);
            __nukeFlash(OPENKNX_FLASH_OFFSET, OPENKNX_FLASH_SIZE);
        }

        if (mode == EraseMode::All || mode == EraseMode::Filesystem)
        {
            openknx.logger.logWithPrefix("Erase", "Format Filesystem");
            if (LittleFS.format())
            {
                openknx.logger.logWithPrefix("Erase", "Succesful");
            }
        }

        if (mode == EraseMode::All)
        {
            openknx.logger.logWithPrefix("Erase", "First bytes of Firmware");
            __nukeFlash(0, 4096);
        }

        openknx.progLed.forceOn();
        openknx.logger.logWithPrefix("Erase", "Completed");
        delay(1000);
        openknx.logger.log("Restart device");
        delay(100);
        knx.platform().restart();
    }

    void Console::resetToBootloader()
    {
        reset_usb_boot(0, 0);
    }
#endif
} // namespace OpenKNX