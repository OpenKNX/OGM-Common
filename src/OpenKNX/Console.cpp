#include "OpenKNX/Console.h"
#include "OpenKNX/Facade.h"
#include "OpenKNX/Flash/Driver.h"
#include "OpenKNX/I2C/Console.h"
#include "OpenKNX/I2C/WireWrapper.h" // defines OPENKNX_I2C_USE_PIO when PIO I2C is configured
#include "OpenKNX/Led/Console.h"
#include "buildtime.h"

#ifdef ARDUINO_ARCH_ESP32
    #include "esp_heap_caps.h"
#endif

#if OPENKNX_LITTLE_FS
    #include "LittleFS.h"
#endif

#if defined(OPENKNX_IDF_INFO) && defined(ARDUINO_ARCH_ESP32)
    #include <esp_chip_info.h>
    #include <esp_pm.h>
    #include <esp_wifi.h>
#endif

namespace OpenKNX
{
    void Console::loop()
    {
        if (_disableConsole)
        {
            // Console is in use by an external (remote FTC) console session. Discard local input so it does not
            // pile up in the UART FIFO, and tell the user once -- in RED -- why the local console is unresponsive,
            // rate-limited so a held key / paste cannot spam. Without this the local console just looks dead.
            if (OPENKNX_LOGGER_DEVICE.available())
            {
                while (OPENKNX_LOGGER_DEVICE.available())
                    OPENKNX_LOGGER_DEVICE.read();
                if (millis() - _disableNoticeMs > 3000)
                {
                    _disableNoticeMs = millis();
                    if (_disableReason != nullptr)
                        logError("Console", "external console session active (%s) -- local input ignored, try again later", _disableReason);
                    else
                        logError("Console", "external console session active -- local input ignored, try again later");
                }
            }
            return;
        }
        if (OPENKNX_LOGGER_DEVICE.available())
            processSerialInput();
    }

    void Console::disableConsole(bool disable, const char* reason)
    {
        _disableConsole = disable;
        _disableReason = disable ? reason : nullptr; // reason lives in the caller (persistent buffer); cleared on re-enable
    }

    bool Console::submitLine(const char* line)
    {
        if (line == nullptr) return false;
#ifdef OPENKNX_FTC_CONSOLE
        if (_lineSink)
        {
            _lineSink(line);
            return true;
        }
#endif
        if (!processCommand(line))
            openknx.logger.logWithValues("%s: command not found", line);
        return false;
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
#ifdef KNX_HAS_TP
        TpUartDataLinkLayer* dll = KNX_TP_DLL;
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
    #ifdef OPENKNX_I2C_USE_PIO
        // i2c scan/introspection backend exists only on the PIO-I2C bus
        else if (!diagnoseKo && (cmd.compare(0, 4, "i2c ") == 0 || cmd.compare(0, 3, "i2c") == 0))
        {
            OpenKNX::I2C::Console::processCommand(cmd);
        }
    #endif
        else if (!diagnoseKo && (cmd.compare(0, 5, "leds ") == 0 || cmd.compare(0, 4, "leds") == 0 || cmd.compare(0, 4, "led ") == 0 || cmd.compare(0, 3, "led") == 0))
        {
            OpenKNX::Led::Console::processCommand(cmd);
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
        else if (!diagnoseKo && (cmd == "fs" || cmd == "files" || cmd == "file"))
        {
            showFilesystem();
        }
        else if (!diagnoseKo && (cmd == "fs ?" || cmd == "files ?"))
        {
            showFilesystemHelp();
        }
        else if (!diagnoseKo && (cmd == "fs df"))
        {
            showFilesystemUsage();
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
        else if (!diagnoseKo && (cmd.rfind("fs dmp ", 0) == 0 || cmd.rfind("fs cat ", 0) == 0) && cmd.length() > 7)
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
        else if (!diagnoseKo && (cmd.rfind("fs del ", 0) == 0 || cmd.rfind("fs rm ", 0) == 0))
        {
            size_t off = (cmd.rfind("fs del ", 0) == 0) ? 7 : 6; // "fs del " vs "fs rm "
            if (cmd.length() > off)
            {
                auto fileName = cmd.substr(off);
                if (fileName[0] != '/')
                    fileName = "/" + fileName;
                if (LittleFS.remove(fileName.c_str()))
                    logInfo("Filesystem", "File %s deleted", fileName.c_str());
                else
                    logError("Filesystem", "File %s not found", fileName.c_str());
            }
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
#ifdef KNX_HAS_TP
        else if (cmd.compare("bcu") == 0)
        {
            // Compact overview: two fixed lines (state+traffic, buffer+health counters) plus a
            // conditional third line (yellow) that only appears on genuine faults (NCN errors or
            // temperature warnings). Full framed report stays available via "bcu stat".
            auto& tp = dll->getTPUart();
            auto& st = tp.getStatistics();

            char line[128];

            // Line 1: BCU state in the log prefix, traffic in the body.
            char pfx[40];
            snprintf(pfx, sizeof(pfx), "BCU<Status: %s>", tp.getBcuStateInfo());
            // Compact units (OGM-Common Helper): the raw 32-bit counters run into 8-10 digits on a busy
            // bus and pushed this line past the console width. humanBytes() already carries its unit.
            snprintf(line, sizeof(line), "TX %s | RX %s (%s) | Discarded %s | Received %s | Load %s/s",
                     humanCount(st.getTxFrames()).c_str(), humanCount(st.getRxFrames()).c_str(),
                     humanBytes(st.getRxFrameBytes()).c_str(), humanBytes(st.getRxDiscardedBytes()).c_str(),
                     humanBytes(st.getRxReceivedBytes()).c_str(),
                     humanBytes(openknx.busLoad.currentBytesPerSec()).c_str());
            openknx.logger.logWithPrefix(pfx, line);

            // Line 2: buffer / health counters (always shown; health getters return 0 without TPUART_BCU_HEALTH).
            // Buffer/Await stay raw: they are momentary positions bounded by the buffer size, not counters.
            snprintf(line, sizeof(line),
                     "Buffer %u | Await %u | Repetitions %s | Overflow %s/%s/%s/%s | Resets %s | CON-rescues %s | Disconnects %s",
                     tp.getReceiver().getSearchBufferPosition(), tp.getReceiver().getAwaitBytes(),
                     humanCount(st.getRxRepetitions()).c_str(),
                     humanCountShort(st.getRxUartOverflow()).c_str(), humanCountShort(st.getRxSearchBufferOverflow()).c_str(),
                     humanCountShort(st.getRxFrameBufferOverflow()).c_str(), humanCountShort(st.getTxOverflowFrameBuffer()).c_str(),
                     humanCount(st.getBcuResets()).c_str(), humanCount(st.getBcuConRescues()).c_str(),
                     humanCount(st.getBcuDisconnects()).c_str());
            openknx.logger.logWithPrefix("BCU<Stat>", line);

            // Line 3: only on faults, in yellow (CONSOLE_HEADLINE_COLOR == ANSI yellow).
            const unsigned ncn = st.getBcuSlaveCollisions() + st.getBcuReceiveErrors() +
                                 st.getBcuTransmitErrors() + st.getBcuProtocolErrors();
            if (ncn > 0 || st.getBcuTempWarnings() > 0)
            {
                snprintf(line, sizeof(line), "NCN  SC %u | RE %u | TE %u | PE %u  |  Temp-Warn %u",
                         st.getBcuSlaveCollisions(), st.getBcuReceiveErrors(), st.getBcuTransmitErrors(),
                         st.getBcuProtocolErrors(), st.getBcuTempWarnings());
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.logWithPrefix("BCU<ERROR>", line);
                openknx.logger.color(0);
            }
            return true;
        }
        else if (cmd.compare("bcu ?") == 0)
        {
            printHelpLine("bcu", "Compact BCU status (+ERROR line on faults)");
            printHelpLine("bcu stat", "Full BCU / TPUart statistics table");
            printHelpLine("bcu mon", "Start/Stop BCU monitoring ('bus mon')");
            printHelpLine("bus mon", "Alias for 'bcu mon'");
            printHelpLine("bcu rst", "Reset BCU");
    #if defined(TPUART_API_LEVEL) && TPUART_API_LEVEL >= 2
            printHelpLine("bcu autoack on|off", "Chip acknowledges by itself (on) or host only (off)");
    #endif
    #ifdef TPUART_BCU_DEBUG
            printHelpLine("bcu dis", "Force BCU disconnect (test)");
    #endif
    #ifdef TPUART_BCU_MARKER
            printHelpLine("bcu marker on|off", "NCN frame-end MARKER (bench measurement)");
    #endif
            printHelpLine("bcu poff", "Bus power off");
            printHelpLine("bcu pon", "Bus power on");
            printHelpLine("bcu debug", "Toggle BCU debug logging");
            return true;
        }
        else if (cmd.compare("bcu stat") == 0)
        {
            auto& tp = dll->getTPUart();
            auto& st = tp.getStatistics();
            const int bcuBaud = tp.getBaudrate();

            // Framed, colored report. Inner width bounded so total stays <= 72 and never wraps.
            constexpr uint8_t IW = 66;

            /// @brief Colored/plain full-width divider "+<ch*IW>+".
            auto boxRule = [&](char ch) {
                char r[IW + 3];
                r[0] = '+';
                memset(r + 1, ch, IW);
                r[IW + 1] = '+';
                r[IW + 2] = 0;
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log(r);
                openknx.logger.color(0);
            };
            /// @brief Framed row; pads AND truncates to IW so the frame never breaks. col!=0 colors it.
            auto boxRow = [&](const char* s, uint8_t col) {
                char r[IW + 3];
                snprintf(r, sizeof(r), "|%-*.*s|", (int)IW, (int)IW, s);
                if (col) openknx.logger.color(col);
                openknx.logger.log(r);
                if (col) openknx.logger.color(0);
            };
            /// @brief Aligned key/value row, one or two pairs (l2 == nullptr -> single).
            auto kv = [&](const char* l1, const char* v1, const char* l2, const char* v2) {
                char line[IW + 1];
                if (l2)
                    snprintf(line, sizeof(line), "  %-12s: %-16s%-12s: %s", l1, v1, l2, v2);
                else
                    snprintf(line, sizeof(line), "  %-12s: %s", l1, v1);
                boxRow(line, 0);
            };

            char vBaud[12];
            char vRx[24], vDisc[16], vRecv[16], vLoad[16], vBuf[12], vAwait[12], vRep[12], vOv[28];
            snprintf(vRx, sizeof(vRx), "%s (%s)", humanCount(st.getRxFrames()).c_str(),
                     humanBytes(st.getRxFrameBytes()).c_str());
            snprintf(vDisc, sizeof(vDisc), "%s", humanBytes(st.getRxDiscardedBytes()).c_str());
            snprintf(vRecv, sizeof(vRecv), "%s", humanBytes(st.getRxReceivedBytes()).c_str());
            snprintf(vLoad, sizeof(vLoad), "%s/s", humanBytes(openknx.busLoad.currentBytesPerSec()).c_str());
            snprintf(vBuf, sizeof(vBuf), "%u", tp.getReceiver().getSearchBufferPosition());
            snprintf(vAwait, sizeof(vAwait), "%u", tp.getReceiver().getAwaitBytes());
            snprintf(vRep, sizeof(vRep), "%s", humanCount(st.getRxRepetitions()).c_str());
            snprintf(vOv, sizeof(vOv), "%s/%s/%s/%s", humanCountShort(st.getRxUartOverflow()).c_str(),
                     humanCountShort(st.getRxSearchBufferOverflow()).c_str(),
                     humanCountShort(st.getRxFrameBufferOverflow()).c_str(),
                     humanCountShort(st.getTxOverflowFrameBuffer()).c_str());
            char vTx[12];
            snprintf(vTx, sizeof(vTx), "%s", humanCount(st.getTxFrames()).c_str());

            boxRule('=');
            boxRow(" BCU / TPUart Statistics", CONSOLE_HEADLINE_COLOR);
            boxRule('-');
            boxRow(" Status", CONSOLE_HEADLINE_COLOR);
            if (bcuBaud > 0)
            {
                snprintf(vBaud, sizeof(vBaud), "%d", bcuBaud);
                kv("State", tp.getBcuStateInfo(), "Baud", vBaud);
            }
            else
                kv("State", tp.getBcuStateInfo(), nullptr, nullptr);
    #if defined(TPUART_API_LEVEL) && TPUART_API_LEVEL >= 2
            kv("Chip AutoACK", tp.isAutoAcknowledge() ? "ON" : "off",
               "Chip CRC", tp.isExtendedCRC() ? "CCITT" : "off");
    #endif
            boxRow(" Traffic", CONSOLE_HEADLINE_COLOR);
            kv("TX Frames", vTx, "RX Frames", vRx);
            kv("Discarded", vDisc, "Received", vRecv);
            kv("Load", vLoad, "Buffer", vBuf);
            kv("Await", vAwait, "Repetitions", vRep);
    #if defined(TPUART_API_LEVEL) && TPUART_API_LEVEL >= 2
            char vAckDrop[12];
            snprintf(vAckDrop, sizeof(vAckDrop), "%s", humanCount(tp.getTransmitter().droppedAcknowledges()).c_str());
            kv("Overflow", vOv, "ACK dropped", vAckDrop);
        #ifdef TPUART_BUSMON_INTEGRITY
            // Only measured when the driver collects the per-octet status; without it a row of zeroes
            // would read as "no errors" instead of "not measured".
            char vByteErr[26];
            snprintf(vByteErr, sizeof(vByteErr), "%s/%s/%s/%s",
                     humanCountShort(st.getRxByteFraming()).c_str(), humanCountShort(st.getRxByteParity()).c_str(),
                     humanCountShort(st.getRxByteBreak()).c_str(), humanCountShort(st.getRxByteOverrun()).c_str());
            kv("ByteErr FE/PE/BE/OE", vByteErr, nullptr, nullptr);
        #endif
    #else
            kv("Overflow", vOv, nullptr, nullptr);
    #endif
    #ifdef TPUART_BCU_HEALTH
            char vRes[12], vDisc2[12], vCon[12];
            snprintf(vRes, sizeof(vRes), "%s", humanCount(st.getBcuResets()).c_str());
            snprintf(vDisc2, sizeof(vDisc2), "%s", humanCount(st.getBcuDisconnects()).c_str());
            snprintf(vCon, sizeof(vCon), "%s", humanCount(st.getBcuConRescues()).c_str());
            boxRow(" Health", CONSOLE_HEADLINE_COLOR);
            kv("Resets", vRes, "Disconnects", vDisc2);
            kv("CON-rescues", vCon, nullptr, nullptr);

            char vSc[12], vRe[12], vTe[12], vPe[12], vTw[12];
            snprintf(vSc, sizeof(vSc), "%s", humanCount(st.getBcuSlaveCollisions()).c_str());
            snprintf(vRe, sizeof(vRe), "%s", humanCount(st.getBcuReceiveErrors()).c_str());
            snprintf(vTe, sizeof(vTe), "%s", humanCount(st.getBcuTransmitErrors()).c_str());
            snprintf(vPe, sizeof(vPe), "%s", humanCount(st.getBcuProtocolErrors()).c_str());
            snprintf(vTw, sizeof(vTw), "%s", humanCount(st.getBcuTempWarnings()).c_str());
            boxRow(" NCN Errors", CONSOLE_HEADLINE_COLOR);
            kv("Slave-Coll", vSc, "Recv-Err", vRe);
            kv("Xmit-Err", vTe, "Proto-Err", vPe);
            kv("Temp-Warn", vTw, nullptr, nullptr);
    #endif
            // The NCN rails use the extended TPUart API (getBcuType/getSystemState), which is absent
            // upstream. Gate on the product flag so OGM-Common still compiles against a stock TPUart
            // (section simply omitted there).
#ifdef TPUART_BCU_REGISTER_INFO
            // NCN supervisor rails (already polled via U_SystemStat, 1 Hz).
            if (tp.getBcuType() == TPUart::BCU_NCN5120)
            {
                auto& ss = tp.getSystemState();
                // requestState() is skipped in busmonitor, so the rails are whatever was last read --
                // power-up defaults if nothing ever was. Say so instead of printing an alarming LOW/FAIL.
                if (!ss.seen())
                {
                    boxRow(" NCN Rails", CONSOLE_HEADLINE_COLOR);
                    kv("State", "not read (no reply yet)", nullptr, nullptr);
                }
                else if (tp.isMonitoring())
                {
                    boxRow(" NCN Rails (stale, busmonitor)", CONSOLE_HEADLINE_COLOR);
                    kv("VBUS", ss.vbus() ? "ok" : "LOW", "VFILT", ss.vfilt() ? "ok" : "LOW");
                    kv("V20V", ss.v20v() ? "ok" : "LOW", "VDD2", ss.vdd2() ? "ok" : "LOW");
                    kv("XTAL", ss.xtal() ? "ok" : "FAIL", "Mode", ss.modeString());
                }
                else
                {
                    boxRow(" NCN Rails", CONSOLE_HEADLINE_COLOR);
                    kv("VBUS", ss.vbus() ? "ok" : "LOW", "VFILT", ss.vfilt() ? "ok" : "LOW");
                    kv("V20V", ss.v20v() ? "ok" : "LOW", "VDD2", ss.vdd2() ? "ok" : "LOW");
                    kv("XTAL", ss.xtal() ? "ok" : "FAIL", "Mode", ss.modeString());
                }
            }
#endif // TPUART_BCU_REGISTER_INFO
            boxRule('=');
            return true;
        }
        else if (cmd.compare("bcu mon") == 0 || cmd.compare("bus mon") == 0)
        {
#ifdef TPUART_BCU_REGISTER_INFO
            dll->toggleConsoleMonitor();  // start/stop the local console busmon (raw echo); coexists with an ETS busmon
#else
            dll->monitor();               // upstream fallback: monitor without console echo (start-only)
#endif
            return true;
        }
    #if defined(TPUART_API_LEVEL) && TPUART_API_LEVEL >= 2
        else if (cmd.compare(0, 12, "bcu autoack ") == 0)
        {
            // Off withholds U_SetAddress.req, which is what enables the chip's own acknowledge; the host
            // then decides alone. Exact words only -- a typo must not toggle the mode and reset the BCU.
            const std::string arg = cmd.substr(12);
            if (arg == "on" || arg == "off")
                dll->getTPUart().chipAutoAcknowledge(arg == "on");
            else
                openknx.logger.log("usage: bcu autoack on|off");
            return true;
        }
    #endif
        else if (cmd.compare("bcu rst") == 0)
        {
            dll->reset();
            return true;
        }
    #ifdef TPUART_BCU_DEBUG
        else if (cmd.compare("bcu dis") == 0)
        {
            dll->getTPUart().forceDisconnect();
            return true;
        }
    #endif
    #ifdef TPUART_BCU_MARKER
        // Bench measurement: NCN frame-end MARKER. The device stops delivering frames while it is on
        // (the parser does not know U_FrameEnd.ind/U_FrameState.ind yet); "off" resets the BCU.
        else if (cmd.compare("bcu marker on") == 0)
        {
            dll->getTPUart().markerMode(true);
            return true;
        }
        else if (cmd.compare("bcu marker off") == 0)
        {
            dll->getTPUart().markerMode(false);
            return true;
        }
    #endif
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
                if (submitLine(prompt))
                {
                    memset(prompt, 0, CONSOLE_INPUT_SIZE);
                    _consoleCharLast = current;
                    return;
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
    #ifdef DEVICE_HW_ID
        char deviceIdBuffer[128];
        sprintf(deviceIdBuffer, "%s (HW ID: 0x%04X)", DEVICE_ID, DEVICE_HW_ID);
        openknx.logger.logWithPrefix("ID", deviceIdBuffer);
    #else
        openknx.logger.logWithPrefix("ID", DEVICE_ID);
    #endif
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
        openknx.logger.logWithPrefixAndValues("KNX-Type", "%s (%04X)", KNX_DEVICE_TYPE, MASK_VERSION);
        openknx.logger.logWithPrefixAndValues("CPU-Mode", "%s", cpuMode);
        float cpuTemp = openknx.hardware.cpuTemperature();
        if (cpuTemp > 0)
            openknx.logger.logWithPrefixAndValues("CPU-Temp", "%.1f °C", cpuTemp);

#if defined(OPENKNX_IDF_INFO) && defined(ARDUINO_ARCH_ESP32)
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("IDF Platform");
        openknx.logger.color(0);

        // --- IDF version (runtime via --wrap or stock) ---
        // The --wrap proves this is our custom-built IDF, not the pre-built one.
        const char* idfVer = esp_get_idf_version();
        bool isCustomIdf = (strncmp(idfVer, "OpenKNX-IDF-", 12) == 0);
        openknx.logger.logWithPrefixAndValues("IDF", "%s (%s)", idfVer, isCustomIdf ? "Custom Build" : "Pre-built");

        // --- Chip info (runtime API) ---
        esp_chip_info_t chip;
        esp_chip_info(&chip);
        openknx.logger.logWithPrefixAndValues("Chip", "%d cores, rev %d.%d",
                                              chip.cores, chip.revision / 100, chip.revision % 100);

        // --- Flash size (runtime API via Arduino) ---
        openknx.logger.logWithPrefixAndValues("Flash", "%u MB", ESP.getFlashChipSize() / (1024 * 1024));

        // --- CPU frequency (runtime API) ---
        openknx.logger.logWithPrefixAndValues("CPU-Freq", "%u MHz", getCpuFrequencyMhz());

        // --- Power Management (runtime API) ---
        esp_pm_config_t pmCfg;
        if (esp_pm_get_configuration(&pmCfg) == ESP_OK)
            openknx.logger.logWithPrefixAndValues("PM", "Enabled (max %d / min %d MHz)",
                                                  pmCfg.max_freq_mhz, pmCfg.min_freq_mhz);
        else
            openknx.logger.logWithPrefix("PM", "Not configured");

        // --- WiFi TX power (runtime API, needs WiFi started) ---
        int8_t txPow = 0;
        if (esp_wifi_get_max_tx_power(&txPow) == ESP_OK)
            openknx.logger.logWithPrefixAndValues("WiFi-TX-Power", "%.2f dBm (0.25x%d)", txPow * 0.25f, txPow);
        else
            openknx.logger.logWithPrefix("WiFi-TX-Power", "N/A (WiFi not started)");

        // --- WiFi power-save mode (runtime API, needs WiFi started) ---
        wifi_ps_type_t psType;
        if (esp_wifi_get_ps(&psType) == ESP_OK)
        {
            const char* psName = "Unknown";
            switch (psType)
            {
                case WIFI_PS_NONE: psName = "None"; break;
                case WIFI_PS_MIN_MODEM: psName = "Min-Modem"; break;
                case WIFI_PS_MAX_MODEM: psName = "Max-Modem"; break;
                default: break;
            }
            openknx.logger.logWithPrefixAndValues("WiFi-PS", "%s", psName);
        }
        else
        {
            openknx.logger.logWithPrefix("WiFi-PS", "N/A (WiFi not started)");
        }
        // NOTE: CONFIG_* defines (RF-Cal, Brownout, BLE, Coex etc.) are NOT shown here
        // because they come from pre-built sdkconfig.h headers and do NOT reflect the
        // actual custom IDF build settings. The --wrap on esp_get_idf_version() is the
        // authoritative proof that this is a custom-built IDF.
#endif // OPENKNX_IDF_INFO

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

    void Console::showFilesystemUsage()
    {
        uint64_t total = 0, used = 0;
#if defined(ARDUINO_ARCH_ESP32)
        total = LittleFS.totalBytes();
        used = LittleFS.usedBytes();
#elif defined(ARDUINO_ARCH_RP2040)
        FSInfo info;
        if (LittleFS.info(info))
        {
            total = info.totalBytes;
            used = info.usedBytes;
        }
#endif
        uint64_t freeBytes = (total >= used) ? (total - used) : 0;
        uint8_t usedPct = total ? (uint8_t)((used * 100) / total) : 0;

        logBegin();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.logHeader("Filesystem df");
        openknx.logger.color(0);
        openknx.logger.logWithPrefixAndValues("Filesystem", "Total: %u bytes", (uint32_t)total);
        openknx.logger.logWithPrefixAndValues("Filesystem", "Used:  %u bytes (%u%%)", (uint32_t)used, usedPct);
        openknx.logger.logWithPrefixAndValues("Filesystem", "Free:  %u bytes", (uint32_t)freeBytes);
        openknx.logger.logDividingLine();
        logEnd();
    }

    void Console::showFilesystemHelp()
    {
        logBegin();
        openknx.logger.log("");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.logHeader("Filesystem");
        openknx.logger.color(0);
        printHelpLine("files, fs, file", "Show files on filesystem");
        printHelpLine("fs df", "Show usage (total / used / free)");
        printHelpLine("fs del, rm <file>", "Delete a file");
        printHelpLine("fs dmp, cat <file>", "Dump a file (hex)");
        openknx.logger.logDividingLine();
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
        printHelpLine("files, fs [?]", "Show files ('fs ?' for filesystem tools)");
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
#ifdef OPENKNX_I2C_USE_PIO
        printHelpLine("i2c", "I2C bus commands. Use 'i2c' for help");
#endif
        printHelpLine("leds", "LED control. Use 'leds' for help");
#ifdef KNX_HAS_TP
        printHelpLine("bcu", "Compact BCU status. Use 'bcu ?' for help");
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
        // Heap detail: free / minimum-ever-free / largest contiguous block. A healthy 'free'
        // with a tiny 'largest' still means allocations fail (fragmentation). The DMA pool
        // feeds EMAC/Wi-Fi/SPI -- a starved DMA pool is exactly the "no mem for receive buffer"
        // failure. On the classic ESP32 the DMA pool equals the default pool, so the DMA line
        // is only printed when it actually differs (diverges near exhaustion / on S3/PSRAM).
        size_t heapFree = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        size_t heapMin = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
        size_t heapLargest = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        openknx.logger.logWithPrefixAndValues("Heap", "free %.3f / min %.3f / largest %.3f KiB",
                                              ((float)heapFree / 1024), ((float)heapMin / 1024), ((float)heapLargest / 1024));
        size_t dmaFree = heap_caps_get_free_size(MALLOC_CAP_DMA);
        size_t dmaMin = heap_caps_get_minimum_free_size(MALLOC_CAP_DMA);
        size_t dmaLargest = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
        if (dmaFree != heapFree || dmaMin != heapMin || dmaLargest != heapLargest)
            openknx.logger.logWithPrefixAndValues("Heap DMA", "free %.3f / min %.3f / largest %.3f KiB",
                                                  ((float)dmaFree / 1024), ((float)dmaMin / 1024), ((float)dmaLargest / 1024));
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
    } // processPinCommand
#endif // ARDUINO_ARCH_SAMD
} // namespace OpenKNX
