#include "OpenKNX/Console.h"
#include "OpenKNX/Facade.h"
#include "OpenKNX/Flash/Driver.h"
#include "buildtime.h"

#if OPENKNX_LITTLE_FS
    #include "LittleFS.h"
#endif

namespace OpenKNX
{
    void Console::loop()
    {
        if (_disableConsole)
            return;
        if (OPENKNX_LOGGER_DEVICE.available())
            processSerialInput();
    }

    void Console::disableConsole(bool disable)
    {
        _disableConsole = disable;
    }

#ifdef BASE_KoDiagnose
    void Console::writeDiagnoseKo(const char* message, ...)
    {
        va_list values;
        va_start(values, message);
        _diagnoseKoOutput = true;
        writeDpt16Ko(KoBASE_Diagnose, message, values);
        knx.loop();
        _diagnoseKoOutput = false;
        va_end(values);
    }

    // fallback for old spelling mistake in method name
    void Console::writeDiagenoseKo(const char* message, ...)
    {
        va_list values;
        va_start(values, message);
        _diagnoseKoOutput = true;
        writeDpt16Ko(KoBASE_Diagnose, message, values);
        knx.loop();
        _diagnoseKoOutput = false;
        va_end(values);
    }

    void Console::processDiagnoseKo(GroupObject& ko)
    {
        // prevent the nested call by the output on the diagnose ko
        if (_diagnoseKoOutput)
            return;

        // quick-fix to ensure \0 at end of 14 char strings
        // TODO cleanup implementation and read DPT16.001
        char cmdBuf[15] = {};
        memcpy(cmdBuf, ko.valueRef(), 14);

        // prevent empty command
        if (cmdBuf[0] == '\0')
            return;

        openknx.logger.logWithPrefixAndValues("DiagnoseKO", "command \"%s\" received", cmdBuf);
        logIndentUp();

        if (!processCommand(cmdBuf, true))
            openknx.logger.logWithPrefix("DiagnoseKO", "command not found");

        logIndentDown();
    }
#endif

    bool Console::processCommand(std::string cmd, bool diagnoseKo /* = false */)
    {
        openknx.common.skipLooptimeWarning();
#if MASK_VERSION == 0x07B0
        TpUartDataLinkLayer* dll = knx.bau().getDataLinkLayer();
#elif MASK_VERSION == 0x091A
        TpUartDataLinkLayer* dll = knx.bau().getSecondaryDataLinkLayer();
#endif

        if (!diagnoseKo && (cmd == "i" || cmd == "info"))
        {
            showInformations();
        }
        else if (!diagnoseKo && (cmd == "h" || cmd == "help"))
        {
            showHelp();
        }
        else if (!diagnoseKo && (cmd == "v" || cmd == "versions"))
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
        else if ((cmd == "u" || cmd == "uptime"))
        {
            showUptime(diagnoseKo);
        }
        else if (!diagnoseKo && (cmd == "sleep"))
        {
            sleep();
        }
        else if (!diagnoseKo && (cmd == "r" || cmd == "restart"))
        {
            delay(20);
            openknx.restart();
        }
        else if (!diagnoseKo && (cmd == "fatal"))
        {
            openknx.hardware.fatalError(5, "Test with 5x blinking");
        }
        else if (!diagnoseKo && (cmd == "powerloss"))
        {
            openknx.common.triggerSavePin();
        }
        else if (!diagnoseKo && (cmd == "s" || cmd == "w" || cmd == "save"))
        {
            openknx.flash.save();
        }
        else if (cmd == "flash knx")
        {
            showMemoryContent(openknx.knxFlash.flashAddress(), openknx.knxFlash.size());
        }
        else if (cmd == "flash openknx")
        {
            showMemoryContent(openknx.openknxFlash.flashAddress(), openknx.openknxFlash.size());
        }
        else if (cmd.substr(0, 6) == "mem 0x" && cmd.length() > 6)
        {
            std::string addrstr = cmd.substr(6, cmd.length() - 6);
            uint32_t addr = std::stoi(addrstr, nullptr, 16);
            showMemoryContent((uint8_t*)addr, 0x40);
        }
#ifndef ARDUINO_ARCH_SAMD
        else if ((!diagnoseKo && (cmd.compare(0, 3, "dw ") == 0 || cmd.compare(0, 3, "aw ") == 0)) ||
                 (cmd.compare(0, 3, "dr ") == 0 || cmd.compare(0, 3, "ar ") == 0))
        {
            processPinCommand(cmd);
        }
        else if (!diagnoseKo && (cmd.rfind("dwon ", 0) == 0 || cmd.rfind("dwoff ", 0) == 0))
        {
            processPinCommand("dw " + cmd.substr(((cmd.rfind("dwon ", 0) == 0) ? 5 : 6)) + (cmd.rfind("dwon ", 0) == 0 ? " 1" : " 0"));
        }
#endif

#ifdef OPENKNX_RUNTIME_STAT
        else if (!diagnoseKo && (cmd == "runtime"))
        {
            openknx.common.showRuntimeStat();
        }
        else if (!diagnoseKo && (cmd == "runtime hist"))
        {
            openknx.common.showRuntimeStat(false, true);
        }
        else if (!diagnoseKo && (cmd == "runtime full"))
        {
            openknx.common.showRuntimeStat(true, true);
        }
#endif
#ifdef OPENKNX_WATCHDOG
        else if (cmd == "watchdog")
        {
            showWatchdogResets(diagnoseKo);
        }
#endif
#if OPENKNX_LITTLE_FS
        else if (!diagnoseKo && (cmd == "fs" || cmd == "files"))
        {
            showFilesystem();
        }
        else if (!diagnoseKo && (cmd == "file dummy"))
        {
            const char* buffer = "DUMMY";
            File file = LittleFS.open("/dummy.dummy", "a");
            file.seek((uint32_t)random());
            file.write((const uint8_t*)buffer, strlen(buffer));
            file.close();
            showFilesystem();
        }
        else if (!diagnoseKo && cmd.rfind("fs dmp ", 0) == 0 && cmd.length() > 7)
        {
            auto fileName = cmd.substr(7, cmd.length() - 7);
            if (fileName[0] != '/')
                fileName = "/" + fileName;
            File file = LittleFS.open(fileName.c_str(), "r");
            if (file.available() && !file.isDirectory())
            {
                logInfo("Filesystem", "Dump of file %s (%u bytes):", fileName.c_str(), file.size());
                uint8_t buffer[16] = {};
                size_t readBytes;
                while ((readBytes = file.readBytes((char*)buffer, sizeof(buffer))) > 0)
                {
                    openknx.logger.logHexWithPrefix("Filesystem", buffer, readBytes);
                }
                openknx.logger.logDividingLine();
            }
            else
            {
                logError("Filesystem", "File %s not found", fileName.c_str());
            }
            file.close();
        }
        else if (!diagnoseKo && cmd.rfind("fs del ", 0) == 0 && cmd.length() > 7)
        {
            auto fileName = cmd.substr(7, cmd.length() - 7);
            if (fileName[0] != '/')
                fileName = "/" + fileName;
            if (LittleFS.remove(fileName.c_str()))
                logInfo("Filesystem", "File %s deleted", fileName.c_str());
            else
                logError("Filesystem", "File %s not found", fileName.c_str());
        }
#endif
#ifdef ARDUINO_ARCH_RP2040
        else if (!diagnoseKo && (cmd == "bootloader"))
        {
            resetToBootloader();
        }
#endif
        else if (!diagnoseKo && (cmd == "erase knx"))
        {
            erase(EraseMode::KnxFlash);
        }
        else if (!diagnoseKo && (cmd == "erase openknx"))
        {
            erase(EraseMode::OpenKnxFlash);
        }
#if OPENKNX_LITTLE_FS
        else if (!diagnoseKo && (cmd == "erase files"))
        {
            erase(EraseMode::Filesystem);
        }
#endif
        else if (!diagnoseKo && (cmd == "erase all"))
        {
            erase(EraseMode::All);
        }
#if MASK_VERSION == 0x07B0 || MASK_VERSION == 0x091A
        else if (cmd.compare("bcu") == 0)
        {
            TPUart::Statistics& statistics = dll->getTPUart().getStatistics();

            // Die Buslast gehört hierher und nicht zu RX: sie speist sich zwar aus den empfangenen
            // Telegrammbytes, aber der Chip spiegelt jedes selbst gesendete Oktett zurück - der eigene
            // Versand steckt also mit drin. Es ist die Last auf dem BUS, nicht die einer Richtung.
            // Der Prozentwert ist der Anteil der Zeit, in der der Bus belegt war - aus Oktetts UND
            // Telegrammzahl gerechnet, weil vor jedem Telegramm 50 Bitzeiten frei sein müssen. 100% heißt
            // damit wirklich "hier passt nichts mehr hinein". Nicht enthalten sind die Quittungen (~2,7ms
            // je quittiertem Telegramm), die der Host außerhalb des Busmonitors gar nicht sieht - der Wert
            // liegt also eher etwas zu niedrig. Die Bytezahl bleibt daneben stehen, weil sie die Größe
            // nennt, aus der sich das ergibt.
            logInfo("BCU<Status>", "%s | Load %u%% (%u B/s)", dll->getTPUart().getBcuStateInfo(),
                    statistics.getBusLoadPercent(), statistics.getBusLoad());

            // DER TAKT, und er gehört nach oben, weil alles Folgende an ihm hängt. Die Schicht bewegt ein
            // Byte je Richtung und Aufruf; kommt tick() zu selten dran, zeigt sich das nicht als Fehler,
            // sondern als unterdrückte Quittung, als Rückstand im Interface und irgendwann als Überlauf.
            //
            // Drei Befunde stecken darin: "Driver no" heißt, der Hauptloop tickt und die Rate hängt an der
            // Anwendung. Eine Ist-Rate deutlich unter dem Soll heißt dasselbe, auch wenn der Treiber läuft.
            // Und ein Maximum weit über dem Soll heißt, dass der Antrieb zwar stimmt, aber zwischendurch
            // blockiert wird - ab etwa 2700µs ist ein Acknowledge-Fenster verloren.
            //
            // "Slow" ist die Zahl, die den Höchstwert überhaupt erst deutbar macht: einmal 15ms ist ein
            // Ereignis, hundertmal 2ms ist ein Defekt - im blossen Maximum sehen beide gleich aus. Und die
            // DIFFERENZ zwischen zwei Aufrufen sagt, wann es passiert: einmal "bcu" im Leerlauf, einmal
            // direkt nach der verdächtigen Aktion, und der Verursacher ist eingekreist.
            //
            // Der Durchschnitt kommt aus der Library und ist ab dem ERSTEN Tick gerechnet. Hier stand
            // einmal eine Rechnung gegen die Uptime - die zeigte dauerhaft zu niedrige Werte, die mit
            // wachsender Laufzeit scheinbar besser wurden, weil der Zähler erst bei begin() anfängt.
            // "Max run" ist die Zahl, die eine hohe IRQ-Priorität rechtfertigt: wer andere Interrupts
            // verdrängt, muss belegen können, dass er sie nur kurz aufhält. Sie schliesst den
            // Quittungs-Callback ein, denn der läuft mit im Tick.
            // "Max run" rechtfertigt die IRQ-Priorität: wer andere Interrupts verdrängt, muss belegen
            // können, dass er sie nur kurz aufhält. "ack" ist der Anteil des Quittungs-Callbacks daran -
            // der einzige unbegrenzte Teil des Ticks, denn er ist unser Code, nicht der der Library.
            // Liegen beide dicht beieinander, ist die Laufzeit unsere.
            // "Deferred" statt "slow": der Antrieb war nicht langsam, er wurde AUFGEHALTEN - und die Zahl
            // in Klammern ist die ZULETZT gemessene Verzögerung, nicht die schlimmste je gemessene. Der
            // Höchstwert sättigt: nach einem einzigen Flash-Schreibvorgang stünde dort für immer eine
            // fünfstellige Zahl, und er sagte danach nichts mehr über den aktuellen Zustand.
            // Hier standen nacheinander schon der rohe Tickzähler und der gemessene Abstand - beide sind
            // wieder raus. Der Zähler sagt für sich nichts, und der gemessene Abstand ist durch "Deferred"
            // vollständig gedeckt: ein Mittelwert über der Busgrenze setzt zwingend einzelne Abstände über
            // der Busgrenze voraus, der Zähler meldet also alles, was der Mittelwert melden würde - und
            // zusätzlich die einzelnen Aussetzer, die im Mittel untergehen.
            logInfo("BCU<Tick>", "Driver %s @ %uus | Deferred %u (last %uus) | Max run %uus (ack %uus)",
                    dll->getTPUart().hasTickDriver() ? "yes" : "no", dll->getTPUart().tickInterval(),
                    statistics.getTickDeferrals(), statistics.getTickLastDeferredUs(),
                    statistics.getTickDurationMaxUs(), statistics.getCheckAcknowledgeMaxUs());

            // Drei Zeilen statt einer, nach Fragen sortiert: was kam rein, was ging raus, wo klemmt es.
            // Die Zähler überschneiden sich absichtlich - ein mangels Platz verworfenes Telegramm steht
            // mit seinen Bytes auch in "frame", denn über den Bus kam es. Nicht aufsummieren.
            // KEIN Gleichheitszeichen zwischen "Bytes" und den beiden Kategorien - sie sind KEINE Zerlegung,
            // und hier stand einmal eines. In beide Richtungen falsch: "Bytes" zählt jedes gelesene Byte,
            // die Kategorien nur die einer ABGESCHLOSSENEN Sequenz. Was in RxState::FrameAck verbraucht
            // wird (das L_Data.con zum eigenen Telegramm, ein geschlucktes U_FrameState.ind), wird gelesen,
            // gehört aber absichtlich zu keiner Sequenz - es geht als Flag ans Telegramm. Umgekehrt steht
            // ein mangels Ringplatz verworfenes Telegramm mit seinen Bytes sowohl in "frame" als auch in
            // "Dropped". Die Zähler beantworten verschiedene Fragen; sie summieren sich nicht.
            logInfo("BCU<RX>", "Frames %u valid / %u invalid / %u repeated | Bytes %u (frame %u, ctrl %u) | Dropped %u B in %u resync",
                    statistics.getRxFrames(), statistics.getRxInvalidFrames(), statistics.getRxRepeatedFrames(),
                    statistics.getRxBytes(), statistics.getRxFrameBytes(), statistics.getRxControlBytes(),
                    statistics.getRxDroppedBytes(), statistics.getRxResyncs());

            // "No con" ist der Wachhund: gar keine Bestätigung, die BCU musste zurückgesetzt werden. Ein
            // NEGATIVES L_Data.con steht hier bewusst nicht - das sagt nur, dass auf dem Bus niemand
            // quittiert hat, und ist eine Aussage über den Bus, kein Fehler der Strecke.
            logInfo("BCU<TX>", "Frames %u | Bytes %u (%u ctrl) | Ack %u sent / %u suppressed | No con %u",
                    statistics.getTxFrames(), statistics.getTxBytes(), statistics.getTxControlBytes(),
                    statistics.getTxAcknowledges(), statistics.getTxAcknowledgesSuppressed(),
                    statistics.getTxConfirmTimeouts());

            // HÖCHSTSTÄNDE, und bewusst in derselben Reihenfolge und mit denselben Namen wie die
            // Überläufe in der Err-Zeile: dort steht, wie oft es zu spät war, hier, wie viel Luft noch ist.
            // Beim Interface gibt es keine Kapazität zu zeigen - wie groß dessen Empfangspuffer ist, weiß
            // nur die Plattform (RP2040 256 Byte per Vorgabe, ESP32 512), nicht diese Schicht.
            const unsigned rxSize = (unsigned)TPUart::TPUART_RX_QUEUE_SIZE;
            const unsigned txSize = (unsigned)TPUART_TX_BUFFER_SIZE;
            const unsigned ctrlSize = (unsigned)TPUart::TPUART_CTRL_QUEUE_SIZE;

            // Prozent voran, Bytes dahinter: der Anteil beantwortet "wie viel Luft ist noch" auf einen
            // Blick, die absoluten Zahlen bleiben für die Auslegung daneben stehen. Beim Interface gibt es
            // keinen Anteil - wie groß dessen Puffer ist, weiß nur die Plattform, nicht diese Schicht.
            logInfo("BCU<Peak>", "if %u B | rx %u%% (%u/%u B) | tx %u%% (%u/%u B) | ctrl %u%% (%u/%u B)",
                    statistics.getRxInterfacePeakBytes(),
                    statistics.getRxQueuePeakBytes() * 100 / rxSize, statistics.getRxQueuePeakBytes(), rxSize,
                    statistics.getTxQueuePeakBytes() * 100 / txSize, statistics.getTxQueuePeakBytes(), txSize,
                    statistics.getTxControlQueuePeakBytes() * 100 / ctrlSize, statistics.getTxControlQueuePeakBytes(), ctrlSize);

            // NUR FEHLER. Die Höchststände standen hier einmal mit drin - sie sind aber Füllstände und
            // beantworten die Auslegungsfrage ("wie viel Luft ist noch"), nicht "was ist schiefgegangen".
            // Sie stehen deshalb jetzt in der Zeile der Richtung, die sie messen.
            //
            // SC/RE/TE/PE/TW sind die Fehlerbits, die der Chip selbst meldet - dieselben Kürzel wie in der
            // TP-Error-Zeile.
            logInfo("BCU<Error>", "Overflow if %u / rx %u / tx %u / ctrl %u | Chip SC %u RE %u TE %u PE %u TW %u | Disconnects %u\n",
                    statistics.getRxInterfaceOverflows(), statistics.getRxQueueOverflows(),
                    statistics.getTxQueueOverflows(), statistics.getTxControlQueueOverflows(),
                    statistics.getChipSlaveCollisions(), statistics.getChipReceiveErrors(),
                    statistics.getChipTransmitErrors(), statistics.getChipProtocolErrors(),
                    statistics.getChipTemperatureWarnings(),
                    statistics.getConnectionLosses());

            return true;
        }
        else if (cmd.compare("bcu mon") == 0)
        {
            dll->monitor();
            return true;
        }
        else if (cmd.compare("bcu rst") == 0)
        {
            dll->reset();
            return true;
        }
        else if (cmd.compare("bcu poff") == 0)
        {
            dll->powerControl(false);
            return true;
        }
        else if (cmd.compare("bcu pon") == 0)
        {
            dll->powerControl(true);
            return true;
        }
        else if (cmd.compare("bcu debug") == 0)
        {
            if (_bcuDebug)
                _bcuDebug = false;
            else
                _bcuDebug = true;

            openknx.logger.logWithPrefix("BCU<Debug>", _bcuDebug ? "Enabled" : "Disabled");
            return true;
        }
#endif
        else if (openknx.time.processCommand(cmd, diagnoseKo))
        {
            return true;
        }
#ifdef ParamBASE_Latitude
        else if (openknx.sun.processCommand(cmd, diagnoseKo))
        {
            return true;
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

        // Magic byte for save data during firmware upgrade
        if (current == 0x7)
        {
            OPENKNX_LOGGER_DEVICE.write(0x7);
            openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_PROG)->forceOn();
            openknx.flash.save(true);
            OPENKNX_LOGGER_DEVICE.write(0x7);
            delay(10000);
            openknx.restart();
        }

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
            memset(prompt, 0, CONSOLE_INPUT_SIZE); // Reset Promptbuffer
        }

        if (current == '\b' && strlen(prompt) > 0)
            prompt[strlen(prompt) - 1] = 0x0;

        if (strlen(prompt) < CONSOLE_INPUT_SIZE && current >= 32 && current <= 126) // Max. printables chars allowed
            prompt[strlen(prompt)] = current;

        openknx.logger.printPrompt();
        _consoleCharLast = current;
    }

    void Console::showInformations()
    {
#ifdef OPENKNX_DUALCORE
        const char* cpuMode = openknx.usesDualCore() ? "Dual-Core" : "Single-Core";
#else
        const char* cpuMode = "Single-Core";
#endif

        logBegin();
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.logHeader("");
        openknx.logger.color(0);
        openknx.logger.log("");
        openknx.logger.log("        \x1B[90mOpen \x1B[32m#\x1B[0m           OpenKNX.de");
        openknx.logger.log("        \x1B[32m+----+\x1B[0m");
        openknx.logger.log("        \x1B[32m# \x1B[37mKNX\x1B[0m            wiki.openknx.de - forum.openknx.de");
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.logHeader("Information");
        openknx.logger.color(0);

        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("Device");
        openknx.logger.color(0);
#ifdef DEVICE_ID
        openknx.logger.logWithPrefix("ID", DEVICE_ID);
#endif
#ifdef DEVICE_NAME
        openknx.logger.logWithPrefix("Name", DEVICE_NAME);
#elif defined(HARDWARE_NAME)
        openknx.logger.logWithPrefix("Name", HARDWARE_NAME);
#endif
        openknx.logger.logWithPrefix("Serial number", openknx.info.humanSerialNumber().c_str());

        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("Firmware");
        openknx.logger.color(0);
        openknx.logger.logWithPrefix("Name", openknx.info.firmwareName().c_str());
        openknx.logger.logWithPrefix("Version", openknx.info.humanFirmwareVersion().c_str());
        openknx.logger.logWithPrefix("Number", openknx.info.humanFirmwareNumber().c_str());
#if MASK_VERSION == 0x07B0
        openknx.logger.logWithPrefixAndValues("KNX-Type", "TP (%04X)", MASK_VERSION);
#elif MASK_VERSION == 0x57B0
        openknx.logger.logWithPrefixAndValues("KNX-Type", "IP (%04X)", MASK_VERSION);
#elif MASK_VERSION == 0x091A
        openknx.logger.logWithPrefixAndValues("KNX-Type", "Router (%04X)", MASK_VERSION);
#else
        openknx.logger.logWithPrefixAndValues("KNX-Type", "%04X", MASK_VERSION);
#endif
        float cpuTemp = openknx.hardware.cpuTemperature();
        if (cpuTemp > 0)
            openknx.logger.logWithPrefixAndValues("CPU-Mode", "%s (Temperature %.1f °C)", cpuMode, cpuTemp);
        else
            openknx.logger.logWithPrefixAndValues("CPU-Mode", "%s", cpuMode);

        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("Programming");
        openknx.logger.color(0);
        openknx.logger.logWithPrefixAndValues("Address", "%s (%s)", openknx.info.humanIndividualAddress().c_str(), knx.configured() ? "Configured" : "Unconfigured");
        if (openknx.info.applicationNumber() > 0)
        {
            openknx.logger.logWithPrefix("Version", openknx.info.humanApplicationVersion().c_str());
            openknx.logger.logWithPrefix("Number", openknx.info.humanApplicationNumber().c_str());
        }
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("Runtime");
        openknx.logger.color(0);

        showMemory();

#ifdef OPENKNX_WATCHDOG
        if (openknx.watchdog.active())
            openknx.logger.logWithPrefixAndValues("Watchdog", "Running (%is)", openknx.watchdog.maxPeriod());
        else
            openknx.logger.logWithPrefixAndValues("Watchdog", "Disabled");
#else
        openknx.logger.logWithPrefixAndValues("Watchdog", "Unsupported");
#endif

        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->showInformations();

        openknx.logger.logDividingLine();
        openknx.logger.log("");
        logEnd();
    }

#ifdef OPENKNX_WATCHDOG
    void Console::showWatchdogResets(bool diagnoseKo /* = false */)
    {
    #ifdef BASE_KoDiagnose
        if (diagnoseKo)
        {
            if (openknx.watchdog.active())
                openknx.console.writeDiagnoseKo("WD ON (%ix)", openknx.watchdog.resets());
            else
                openknx.console.writeDiagnoseKo("WD OFF");
        }
    #endif
        if (openknx.watchdog.active())
            openknx.logger.logWithPrefixAndValues("Watchdog", "Running (%i Resets)", openknx.watchdog.resets());
        else
            openknx.logger.logWithPrefixAndValues("Watchdog", "Disabled", openknx.watchdog.resets());
    }
#endif

#if OPENKNX_LITTLE_FS
    void Console::showFilesystem()
    {
        logBegin();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.logHeader("Filesystem");

        openknx.logger.color(0);
        showFilesystemDirectory("/");
        openknx.logger.logDividingLine();
        logEnd();
    }

    void Console::showFilesystemDirectory(std::string path)
    {
        logBegin();
        openknx.logger.logWithPrefixAndValues("Filesystem", "%s", path.c_str());

        File rootDir = LittleFS.open(path.c_str(), "r");
        File directory = rootDir.openNextFile();
        while (directory)
        {
            std::string full = path + directory.name();
            if (directory.isDirectory())
                showFilesystemDirectory(full + "/");
            else
            {
                openknx.logger.logWithPrefixAndValues("Filesystem", "%s (%i bytes)", full.c_str(), directory.size());
            }
            directory = rootDir.openNextFile();
        }
        logEnd();
    }
#endif

    void Console::showVersions()
    {
        logBegin();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.logHeader("Versions");
        openknx.logger.color(0);

        openknx.logger.logWithPrefix("This Firmware", openknx.info.humanFirmwareVersion(true));
        openknx.logger.logWithPrefix("KNX", KNX_Version);
        openknx.logger.logWithPrefix(openknx.common.logPrefix(), MODULE_Common_Version);
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            if (openknx.modules.list[i]->version().empty()) continue;

            openknx.logger.logWithPrefix(openknx.modules.list[i]->name().c_str(), openknx.modules.list[i]->version().c_str());
        }
        openknx.logger.logDividingLine();
        openknx.logger.logWithPrefix("Buildtime", BUILD_DATETIME);
        openknx.logger.logDividingLine();
        logEnd();
    }

    void Console::showHelp()
    {
        logBegin();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.logHeader("Help");
        openknx.logger.color(0);
        openknx.logger.log("Command(s)               Description");
        printHelpLine("help, h", "Show this help");
        printHelpLine("info, i", "Show general information");
        printHelpLine("uptime, u", "Show uptime");
        printHelpLine("version, v", "Show compiled versions");
        printHelpLine("memory, mem", "Show memory usage");
        printHelpLine("mem 0xXXXXXXXX", "Show memory content (64byte) starting at 0xXXXXXXXX");
        printHelpLine("flash knx", "Show knx flash content");
        printHelpLine("flash openknx", "Show openknx flash content");
#if OPENKNX_LITTLE_FS
        printHelpLine("files, fs", "Show files on filesystem");
        printHelpLine("fs del <file>", "Delete a file");
        printHelpLine("fs dmp <file>", "Dump a file");
#endif
#ifdef OPENKNX_RUNTIME_STAT
        printHelpLine("runtime", "Show runtime statistics (Short statistic)");
        printHelpLine("runtime hist", "Show runtime histogram");
        printHelpLine("runtime full", "Show runtime statistics and histogram");
#endif
        printHelpLine("restart, r", "Restart the device");
        printHelpLine("prog, p", "Toggle the ProgMode");
        printHelpLine("save, s, w", "Save data in Flash");
        printHelpLine("sleep", "Sleep for up to 20 seconds");
        printHelpLine("fatal", "Trigger a FatalError");
        printHelpLine("powerloss", "Trigger a PowerLoss (SavePin)");
#ifdef OPENKNX_WATCHDOG
        printHelpLine("watchdog", "Show restart count by watchdog");
#endif
        printHelpLine("erase knx", "Erase knx parameters");
        printHelpLine("erase openknx", "Erase openknx module data");
#if OPENKNX_LITTLE_FS
        printHelpLine("erase files", "Erase filesystem");
#endif
#ifdef ARDUINO_ARCH_RP2040
        printHelpLine("erase all", "Erase all");
#endif
#ifdef ARDUINO_ARCH_RP2040
        printHelpLine("bootloader", "Reset into Bootloader Mode");
#endif
#ifndef ARDUINO_ARCH_SAMD
        printHelpLine("dwon <pin>", "Write digital pin to HIGH");
        printHelpLine("dwoff <pin>", "Write digital pin to LOW");
        printHelpLine("dw <pin> 0-1", "Write digital pin");
        printHelpLine("dr <pin>", "Read digital pin");
        printHelpLine("aw <pin> 0-4095", "Write analog pin");
        printHelpLine("ar <pin>", "Read analog pin");
#endif
#if MASK_VERSION == 0x07B0 || MASK_VERSION == 0x091A
        printHelpLine("bcu", "Show BCU status");
        printHelpLine("bcu mon", "Start BCU monitoring");
        printHelpLine("bcu rst", "Reset BCU");
#endif
#ifdef OPENKNX_TIME_DIGAGNOSTIC
        printHelpLine("tm ?", "Help for time related commands");
#else
        printHelpLine("tm", "Show time information");
    #ifdef OPENKNX_TIME_TESTCOMMAND
        printHelpLine("tm test", "Test some calculation)");
    #endif
#endif
#ifdef ParamBASE_Latitude
        printHelpLine("sun", "Shows sun information");
#endif

        for (uint8_t i = 0; i < openknx.modules.count; i++)
            openknx.modules.list[i]->showHelp();

        openknx.logger.logDividingLine();
        logEnd();
    }

    void Console::sleep()
    {
        openknx.logger.logWithValues("sleep %ims", sleepTime());
        delay(sleepTime());
    }

    uint32_t Console::sleepTime()
    {
#ifdef OPENKNX_WATCHDOG
        return MAX(openknx.watchdog.maxPeriod() + 1, 20000);
#else
        return 20000;
#endif
    }

    void Console::showUptime(bool diagnoseKo /* = false */)
    {
        std::string uptimeStr = openknx.logger.buildUptime();
#ifdef BASE_KoDiagnose
        if (diagnoseKo)
        {
            openknx.console.writeDiagnoseKo("%s", uptimeStr.c_str());
        }
#endif
        openknx.logger.logWithPrefixAndValues("Uptime", "%s", uptimeStr.c_str());
    }

    void Console::showMemory(bool diagnoseKo /* = false */)
    {

#ifdef BASE_KoDiagnose
        if (diagnoseKo)
        {
            openknx.console.writeDiagnoseKo("CUR %.3fKiB", ((float)freeMemory() / 1024));
            openknx.console.writeDiagnoseKo("MIN %.3fKiB", ((float)openknx.common.freeMemoryMin() / 1024));
        }
#endif
        openknx.logger.logWithPrefixAndValues("Free memory", "%.3f KiB (min. %.3f KiB)", ((float)freeMemory() / 1024), ((float)openknx.common.freeMemoryMin() / 1024));
#ifdef ARDUINO_ARCH_ESP32
    #if BOARD_HAS_PSRAM
        openknx.logger.logWithPrefixAndValues("Free PSRAM", "%.3f KiB (min. %.3f KiB)", ((float)ESP.getFreePsram() / 1024), ((float)ESP.getMinFreePsram() / 1024));
    #endif
    #ifdef OPENKNX_DUALCORE
        openknx.logger.logWithPrefixAndValues("Free stack size", "Loop0: %i bytes - Loop1: %i bytes", openknx.common.freeStackMin(), openknx.common.freeStackMin1());
    #else
        openknx.logger.logWithPrefixAndValues("Free stack size", "Loop0: %i bytes", openknx.common.freeStackMin());
    #endif
#endif
#ifdef ARDUINO_ARCH_RP2040

    #ifdef OPENKNX_DUALCORE
        if (openknx.common.freeStackMin() <= 0 || openknx.common.freeStackMin1() <= 0)
    #else
        if (openknx.common.freeStackMin() <= 0)
    #endif
            openknx.logger.color(31);

    #ifdef OPENKNX_DUALCORE
        openknx.logger.logWithPrefixAndValues("Free stack size", "Core0: %i bytes - Core1: %i bytes", openknx.common.freeStackMin(), openknx.common.freeStackMin1());
    #else
        openknx.logger.logWithPrefixAndValues("Free stack size", "Core0: %i bytes", openknx.common.freeStackMin());
    #endif
        openknx.logger.color(0);
#endif
    }

    void Console::showMemoryContent(uint8_t* start, uint32_t size)
    {
        const size_t lineLen = 16;
        uint8_t* end = start + size - (size % lineLen);

        logBegin();
        openknx.logger.logWithPrefixAndValues("Memory content", "Address 0x%08X - Size: 0x%04X (%d bytes)", start, size, size);
        for (uint8_t* linePtr = start; linePtr < end; linePtr += lineLen)
        {
            // normale output
            showMemoryLine(linePtr, lineLen, start);

            // skip repeated lines and show repetition count only
            int repeatCount = 0;
            while (linePtr + lineLen < end && memcmp(linePtr, linePtr + lineLen, lineLen) == 0)
            {
                repeatCount++;
                linePtr += lineLen;
            }
            if (repeatCount > 0)
            {
                openknx.logger.logWithPrefixAndValues("", "%ix (repetitions of previous line)", repeatCount);
            }
        }
        // incomplete last line (edge case)
        if (end != start + size)
        {
            showMemoryLine(end, (start + size) - end, start);
        }
        logEnd();
    }

    void Console::showMemoryLine(uint8_t* line, uint32_t length, uint8_t* memoryStart)
    {
        char prefix[24] = {};
        snprintf(prefix, 24, "0x%06X (0x%08X)", (uint)(line - memoryStart), (uint)line);
        openknx.logger.logHexWithPrefix(prefix, line, length);
    }

    void Console::printHelpLine(const char* command, const char* message)
    {
        // TODO Beautify
        openknx.logger.logWithPrefix(command, message);
    }

    void Console::erase(EraseMode mode)
    {
        openknx.watchdog.deactivate();

        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_PROG)->blinking();
        if (openknx.leds.getLed(Led::LedType::LED_TYPE_INFO1) != nullptr)
            openknx.leds.getLed(Led::LedType::LED_TYPE_INFO1)->off();
        if (openknx.leds.getLed(Led::LedType::LED_TYPE_INFO2) != nullptr)
            openknx.leds.getLed(Led::LedType::LED_TYPE_INFO2)->off();
        if (openknx.leds.getLed(Led::LedType::LED_TYPE_INFO3) != nullptr)
            openknx.leds.getLed(Led::LedType::LED_TYPE_INFO3)->off();

        if (mode == EraseMode::All || mode == EraseMode::KnxFlash)
        {
            openknx.logger.logWithPrefix("Erase", "KNX parameters");
            openknx.knxFlash.erase();
        }

        if (mode == EraseMode::All || mode == EraseMode::OpenKnxFlash)
        {
            openknx.logger.logWithPrefix("Erase", "OpenKNX save data");
            openknx.openknxFlash.erase();
        }

#if OPENKNX_LITTLE_FS
        if (mode == EraseMode::All || mode == EraseMode::Filesystem)
        {
            openknx.logger.logWithPrefix("Erase", "Format Filesystem");
            if (LittleFS.format())
            {
                openknx.logger.logWithPrefix("Erase", "Succesful");
            }
        }
#endif
#ifdef ARDUINO_ARCH_RP2040
        if (mode == EraseMode::All)
        {
            openknx.logger.logWithPrefix("Erase", "First bytes of Firmware");
            if (!__nukeFlash(0, 4096))
                openknx.logger.log("Fatal: nuke paramters invalid");
        }
#endif // ARDUINO_ARCH_RP2040

        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_PROG)->forceOn();
        openknx.logger.logWithPrefix("Erase", "Completed");
        delay(1000);
        openknx.restart();
    }

#ifdef ARDUINO_ARCH_RP2040
    void Console::resetToBootloader()
    {
        reset_usb_boot(0, 0);
    }
#endif // ARDUINO_ARCH_RP2040

#ifndef ARDUINO_ARCH_SAMD
    void Console::processPinCommand(const std::string& cmd)
    {
        auto _pos = cmd.find(' ');
        if (_pos != std::string::npos)
        {
            uint16_t pin;
            if (cmd.length() <= 7)
                pin = std::stoi(cmd.substr(_pos + 1));
            else
                pin = std::stoi(cmd.substr(_pos + 1), nullptr, 16);

            if (cmd.compare(0, 2, "dw") == 0 || cmd.compare(0, 2, "aw") == 0)
            {
                auto __pos = cmd.find(' ', _pos + 1);
                if (__pos != std::string::npos)
                {
                    int value = std::stoi(cmd.substr(__pos + 1));
                    if (cmd.compare(0, 2, "dw") == 0 && value <= HIGH)
                    {
                        openknx.gpio.digitalWrite((pin_size_t)pin, value);
                        openknx.logger.logWithPrefixAndValues("PinCommand", "Write pin %i to %i", pin, value);
                    }
                    else if (cmd.compare(0, 2, "aw") == 0 && value <= 4095)
                    {
                        analogWrite((pin_size_t)pin, value);
                        openknx.logger.logWithPrefixAndValues("PinCommand", "Write pin %i to %i", pin, value);
                    }
                }
            }
            else if (cmd.compare(0, 2, "dr") == 0)
            {
                openknx.logger.logWithPrefixAndValues("PinCommand", "Read pin %i: %i", pin, openknx.gpio.digitalRead((openknx_gpio_number_t)pin));
            }
            else if (cmd.compare(0, 2, "ar") == 0)
            {
                openknx.logger.logWithPrefixAndValues("PinCommand", "Read pin %i: %i", pin, analogRead((pin_size_t)pin));
            }
        }
    }
#endif
} // namespace OpenKNX