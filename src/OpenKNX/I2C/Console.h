#pragma once
/**
 * @file        Console.h
 * @brief       I2C Console Commands (Header-Only)
 * @details     Provides console interface for I2C bus management, scanning, statistics and benchmarking
 * @version     1.0.0
 * @date        2025-11-25
 * @copyright   Copyright (c) 2025 OpenKNX
 */

#include "OpenKNX/defines.h"
#include "OpenKNX/Facade.h"
#include <string>
#include <vector>

#ifdef OPENKNX_I2C_BENCHMARK
#include "OpenKNX/I2C/test/Benchmark.h"
#endif

namespace OpenKNX
{
    namespace I2C
    {
        /**
         * @brief I2C Console Command Handler (Header-Only Implementation)
         */
        class Console
        {
          public:
            /**
             * @brief Process I2C console commands
             * @param cmd The complete command string (e.g., "i2c status", "i2c scan wire_pio")
             */
            static inline void processCommand(const std::string& cmd);

          private:
            static inline void showHelp();
            static inline void showStatus();
            static inline void showInfo(const std::string& busName);
            static inline void showStats();
            static inline void scanBus(const std::string& busName);
            static inline void scanBusImpl(TwoWire& wire, const char* name);
            
#ifdef OPENKNX_I2C_BENCHMARK
            static inline void runBenchmark(const std::string& benchType, const std::string& busName, uint8_t addr);
#endif

            static inline std::vector<std::string> tokenize(const std::string& str);
        };

        // ============================================================================
        // IMPLEMENTATION
        // ============================================================================

        inline void Console::processCommand(const std::string& cmd)
        {
            std::string args = (cmd.length() > 4) ? cmd.substr(4) : "";

            // Trim whitespace
            auto trimStart = args.find_first_not_of(" ");
            if (trimStart != std::string::npos)
            {
                auto trimEnd = args.find_last_not_of(" ");
                args = args.substr(trimStart, trimEnd - trimStart + 1);
            }
            else
                args = "";

            // Show help if empty or "?"
            if (args.empty() || args == "?")
            {
                showHelp();
                return;
            }

            // Tokenize command
            std::vector<std::string> tokens = tokenize(args);
            if (tokens.empty())
            {
                openknx.logger.logWithPrefix("i2c", "Error: Missing command");
                openknx.logger.logWithPrefix("i2c", "Type 'i2c ?' for help");
                return;
            }

            std::string command = tokens[0];

            // Dispatch commands
            if (command == "status")
            {
                showStatus();
            }
            else if (command == "stats")
            {
                showStats();
            }
            else if (command == "info")
            {
                std::string busName = (tokens.size() > 1) ? tokens[1] : "";
                showInfo(busName);
            }
            else if (command == "scan")
            {
                std::string busName = (tokens.size() > 1) ? tokens[1] : "all";
                scanBus(busName);
            }
#ifdef OPENKNX_I2C_BENCHMARK
            else if (command == "bench")
            {
                if (tokens.size() < 2)
                {
                    openknx.logger.logWithPrefix("i2c", "Error: Missing benchmark type");
                    openknx.logger.logWithPrefix("i2c", "Usage: i2c bench <type> [bus] [addr]");
                    return;
                }
                std::string benchType = tokens[1];
                std::string busName = (tokens.size() > 2) ? tokens[2] : "all";
                uint8_t addr = (tokens.size() > 3) ? (uint8_t)strtol(tokens[3].c_str(), NULL, 0) : 0x3C;
                runBenchmark(benchType, busName, addr);
            }
#endif
            else
            {
                openknx.logger.logWithPrefixAndValues("i2c", "Error: Unknown command '%s'", command.c_str());
                openknx.logger.logWithPrefix("i2c", "Type 'i2c ?' for help");
            }
        }

        inline void Console::showHelp()
        {
            openknx.logger.begin();
            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("================================ Help: I2C Control ==============================");
            openknx.logger.color(0);
            openknx.logger.log("Command(s)               Description");
            openknx.console.printHelpLine("i2c status", "Show all configured I2C buses");
            openknx.console.printHelpLine("i2c info [bus]", "Show system info or bus details");
            openknx.console.printHelpLine("i2c scan [bus]", "Scan bus for devices (all or specific)");
#ifdef OPENKNX_PIO_I2C_DMA
            openknx.console.printHelpLine("i2c stats", "Show DMA transfer statistics");
#endif
#ifdef OPENKNX_I2C_BENCHMARK
            openknx.console.printHelpLine("i2c bench <type> [bus] [addr]", "Run benchmarks");
#endif
            
            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("Available Buses:");
            openknx.logger.color(0);
#ifdef OPENKNX_WIRE_PIO
            openknx.console.printHelpLine("  wire_pio", "WIRE_PIO (PIO I2C)");
#endif
#ifdef OPENKNX_WIRE1_PIO
            openknx.console.printHelpLine("  wire1_pio", "WIRE1_PIO (PIO I2C)");
#endif
#ifdef OPENKNX_WIRE
            openknx.console.printHelpLine("  wire", "WIRE (Hardware I2C)");
#endif
#ifdef OPENKNX_WIRE1
            openknx.console.printHelpLine("  wire1", "WIRE1 (Hardware I2C)");
#endif
            openknx.console.printHelpLine("  all", "All configured buses");

#ifdef OPENKNX_I2C_BENCHMARK
            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("Benchmark Types:");
            openknx.logger.color(0);
            openknx.console.printHelpLine("  quick", "Quick performance test");
            openknx.console.printHelpLine("  full", "Full benchmark suite");
            openknx.console.printHelpLine("  speed", "Speed comparison");
            openknx.console.printHelpLine("  stability", "Error rate testing");
#endif

            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("Examples:");
            openknx.logger.color(0);
            openknx.console.printHelpLine("  i2c status", "Show all I2C buses");
            openknx.console.printHelpLine("  i2c info", "Show complete I2C system info");
            openknx.console.printHelpLine("  i2c scan all", "Scan all buses");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("---------------------------------------------------------------------------------");
            openknx.logger.log(" WARNING: Commands may interfere with I2C operations! Test systems only.");
            openknx.logger.log("---------------------------------------------------------------------------------");
            openknx.logger.color(0);
            openknx.logger.end();
        }

        inline void Console::showStatus()
        {
            openknx.logger.begin();
            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("I2C Bus Configuration");
            openknx.logger.color(0);

            int busCount = 0;

#ifdef OPENKNX_WIRE_PIO
            busCount++;
            openknx.logger.log("");
            openknx.logger.logWithValues("[%d] WIRE_PIO - GPIO%d/GPIO%d @ %dkHz", busCount, 
                OPENKNX_WIRE_PIO_SDA_PIN, OPENKNX_WIRE_PIO_SCL_PIN, OPENKNX_WIRE_PIO_CLOCK / 1000);
#ifdef OPENKNX_DEBUG
            openknx.logger.logWithValues("    PIO%d/SM%d, Offset:0x%02X", 
                WIRE_PIO.getPioIndex(), WIRE_PIO.getStateMachineNumber(), WIRE_PIO.getOffset());
#ifdef OPENKNX_PIO_I2C_DMA
            if (WIRE_PIO.getInstance() && WIRE_PIO.getInstance()->_dma_available)
                openknx.logger.logWithValues("    DMA: TX=%d RX=%d", 
                    WIRE_PIO.getInstance()->_dma_tx, WIRE_PIO.getInstance()->_dma_rx);
#endif
#endif
#endif

#ifdef OPENKNX_WIRE1_PIO
            busCount++;
            openknx.logger.log("");
            openknx.logger.logWithValues("[%d] WIRE1_PIO - GPIO%d/GPIO%d @ %dkHz", busCount,
                OPENKNX_WIRE1_PIO_SDA_PIN, OPENKNX_WIRE1_PIO_SCL_PIN, OPENKNX_WIRE1_PIO_CLOCK / 1000);
#ifdef OPENKNX_DEBUG
            openknx.logger.logWithValues("    PIO%d/SM%d, Offset:0x%02X",
                WIRE1_PIO.getPioIndex(), WIRE1_PIO.getStateMachineNumber(), WIRE1_PIO.getOffset());
#ifdef OPENKNX_PIO_I2C_DMA
            if (WIRE1_PIO.getInstance() && WIRE1_PIO.getInstance()->_dma_available)
                openknx.logger.logWithValues("    DMA: TX=%d RX=%d",
                    WIRE1_PIO.getInstance()->_dma_tx, WIRE1_PIO.getInstance()->_dma_rx);
#endif
#endif
#endif

#ifdef OPENKNX_WIRE
            busCount++;
            openknx.logger.log("");
            openknx.logger.logWithValues("[%d] WIRE (HW I2C0) - GPIO%d/GPIO%d @ %dkHz", busCount,
                OPENKNX_WIRE_SDA_PIN, OPENKNX_WIRE_SCL_PIN, OPENKNX_WIRE_CLOCK / 1000);
#endif

#ifdef OPENKNX_WIRE1
            busCount++;
            openknx.logger.log("");
            openknx.logger.logWithValues("[%d] WIRE1 (HW I2C1) - GPIO%d/GPIO%d @ %dkHz", busCount,
                OPENKNX_WIRE1_SDA_PIN, OPENKNX_WIRE1_SCL_PIN, OPENKNX_WIRE1_CLOCK / 1000);
#endif

            openknx.logger.log("");
            openknx.logger.logWithValues("Total: %d bus%s", busCount, busCount == 1 ? "" : "es");
            openknx.logger.log("");
            openknx.logger.end();
        }

        inline void Console::showInfo(const std::string& busName)
        {
            // Complete system info if no bus specified
            if (busName.empty())
            {
                openknx.logger.begin();
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("I2C System Information");
                openknx.logger.color(0);

                int totalBuses = 0;
#ifdef OPENKNX_WIRE_PIO
                totalBuses++;
#endif
#ifdef OPENKNX_WIRE1_PIO
                totalBuses++;
#endif
#ifdef OPENKNX_WIRE
                totalBuses++;
#endif
#ifdef OPENKNX_WIRE1
                totalBuses++;
#endif

                openknx.logger.log("");
                openknx.logger.log("System Status:");
                openknx.logger.logWithValues("  Total Buses:     %d", totalBuses);
                
#ifdef OPENKNX_PIO_I2C_DMA
                int dmaEnabled = 0;
#ifdef OPENKNX_WIRE_PIO
                if (WIRE_PIO.getInstance() && WIRE_PIO.getInstance()->_dma_available) dmaEnabled++;
#endif
#ifdef OPENKNX_WIRE1_PIO
                if (WIRE1_PIO.getInstance() && WIRE1_PIO.getInstance()->_dma_available) dmaEnabled++;
#endif
                openknx.logger.logWithValues("  DMA Enabled:     %d/%d buses", dmaEnabled, totalBuses);
#endif

#ifdef OPENKNX_DEBUG
                openknx.logger.log("");
                openknx.logger.log("Available Resources:");
                
#ifdef ARDUINO_ARCH_RP2040
                int pioUsed = 0;
#ifdef OPENKNX_WIRE_PIO
                pioUsed++;
#endif
#ifdef OPENKNX_WIRE1_PIO
                pioUsed++;
#endif
#ifdef OPENKNX_SERIALLED_ENABLE
                pioUsed += 3;
#endif
                openknx.logger.logWithValues("  PIO State Machines: %d available / 8 total", 8 - pioUsed);
                
                int dmaUsed = 0;
#ifdef OPENKNX_PIO_I2C_DMA
#ifdef OPENKNX_WIRE_PIO
                if (WIRE_PIO.getInstance() && WIRE_PIO.getInstance()->_dma_available) dmaUsed += 2;
#endif
#ifdef OPENKNX_WIRE1_PIO
                if (WIRE1_PIO.getInstance() && WIRE1_PIO.getInstance()->_dma_available) dmaUsed += 2;
#endif
#endif
#ifdef OPENKNX_SERIALLED_ENABLE
                dmaUsed += 3;
#endif
                openknx.logger.logWithValues("  DMA Channels:       %d available / 12 total", 12 - dmaUsed);
#endif
#endif

                openknx.logger.log("");
                openknx.logger.log("Physical Buses:");

#ifdef OPENKNX_WIRE_PIO
                openknx.logger.log("");
                openknx.logger.logWithValues("  [1] WIRE_PIO - GPIO%d/GPIO%d @ %dkHz", 
                    OPENKNX_WIRE_PIO_SDA_PIN, OPENKNX_WIRE_PIO_SCL_PIN, OPENKNX_WIRE_PIO_CLOCK / 1000);
#ifdef OPENKNX_DEBUG
                openknx.logger.logWithValues("      Hardware: PIO%d/SM%d, Offset:0x%02X", 
                    WIRE_PIO.getPioIndex(), WIRE_PIO.getStateMachineNumber(), WIRE_PIO.getOffset());
#ifdef OPENKNX_PIO_I2C_DMA
                if (WIRE_PIO.getInstance() && WIRE_PIO.getInstance()->_dma_available)
                    openknx.logger.logWithValues("      DMA: TX=Ch%d, RX=Ch%d", 
                        WIRE_PIO.getInstance()->_dma_tx, WIRE_PIO.getInstance()->_dma_rx);
#endif
#endif
#endif

#ifdef OPENKNX_WIRE1_PIO
                openknx.logger.log("");
                openknx.logger.logWithValues("  [2] WIRE1_PIO - GPIO%d/GPIO%d @ %dkHz", 
                    OPENKNX_WIRE1_PIO_SDA_PIN, OPENKNX_WIRE1_PIO_SCL_PIN, OPENKNX_WIRE1_PIO_CLOCK / 1000);
#ifdef OPENKNX_DEBUG
                openknx.logger.logWithValues("      Hardware: PIO%d/SM%d, Offset:0x%02X", 
                    WIRE1_PIO.getPioIndex(), WIRE1_PIO.getStateMachineNumber(), WIRE1_PIO.getOffset());
#ifdef OPENKNX_PIO_I2C_DMA
                if (WIRE1_PIO.getInstance() && WIRE1_PIO.getInstance()->_dma_available)
                    openknx.logger.logWithValues("      DMA: TX=Ch%d, RX=Ch%d", 
                        WIRE1_PIO.getInstance()->_dma_tx, WIRE1_PIO.getInstance()->_dma_rx);
#endif
#endif
#endif

#ifdef OPENKNX_WIRE
                openknx.logger.log("");
                openknx.logger.logWithValues("  [3] WIRE (HW I2C0) - GPIO%d/GPIO%d @ %dkHz", 
                    OPENKNX_WIRE_SDA_PIN, OPENKNX_WIRE_SCL_PIN, OPENKNX_WIRE_CLOCK / 1000);
#endif

#ifdef OPENKNX_WIRE1
                openknx.logger.log("");
                openknx.logger.logWithValues("  [4] WIRE1 (HW I2C1) - GPIO%d/GPIO%d @ %dkHz", 
                    OPENKNX_WIRE1_SDA_PIN, OPENKNX_WIRE1_SCL_PIN, OPENKNX_WIRE1_CLOCK / 1000);
#endif

#ifdef OPENKNX_PIO_I2C_DMA
                openknx.logger.log("");
                openknx.logger.log("Statistics:");
                
#ifdef OPENKNX_WIRE_PIO
                if (WIRE_PIO.getInstance() && WIRE_PIO.getInstance()->_dma_available)
                {
                    uint32_t total = WIRE_PIO.getInstance()->_dma_write_count + WIRE_PIO.getInstance()->_dma_read_count + 
                                     WIRE_PIO.getInstance()->_blocking_write_count + WIRE_PIO.getInstance()->_blocking_read_count;
                    uint32_t dma_total = WIRE_PIO.getInstance()->_dma_write_count + WIRE_PIO.getInstance()->_dma_read_count;
                    
                    openknx.logger.logWithValues("  WIRE_PIO Transfers:  %u total", total);
                    if (total > 0)
                        openknx.logger.logWithValues("  DMA Usage:           %u%% (%u DMA / %u blocking)", 
                            (dma_total * 100) / total, dma_total, total - dma_total);
#ifdef OPENKNX_DEBUG
                    openknx.logger.logWithValues("    DMA Writes:        %u", WIRE_PIO.getInstance()->_dma_write_count);
                    openknx.logger.logWithValues("    DMA Reads:         %u", WIRE_PIO.getInstance()->_dma_read_count);
                    openknx.logger.logWithValues("    Blocking Writes:   %u", WIRE_PIO.getInstance()->_blocking_write_count);
                    openknx.logger.logWithValues("    Blocking Reads:    %u", WIRE_PIO.getInstance()->_blocking_read_count);
#endif
                }
#endif

#ifdef OPENKNX_WIRE1_PIO
                if (WIRE1_PIO.getInstance() && WIRE1_PIO.getInstance()->_dma_available)
                {
                    uint32_t total = WIRE1_PIO.getInstance()->_dma_write_count + WIRE1_PIO.getInstance()->_dma_read_count + 
                                     WIRE1_PIO.getInstance()->_blocking_write_count + WIRE1_PIO.getInstance()->_blocking_read_count;
                    uint32_t dma_total = WIRE1_PIO.getInstance()->_dma_write_count + WIRE1_PIO.getInstance()->_dma_read_count;
                    
                    openknx.logger.logWithValues("  WIRE1_PIO Transfers: %u total", total);
                    if (total > 0)
                        openknx.logger.logWithValues("  DMA Usage:           %u%% (%u DMA / %u blocking)", 
                            (dma_total * 100) / total, dma_total, total - dma_total);
#ifdef OPENKNX_DEBUG
                    openknx.logger.logWithValues("    DMA Writes:        %u", WIRE1_PIO.getInstance()->_dma_write_count);
                    openknx.logger.logWithValues("    DMA Reads:         %u", WIRE1_PIO.getInstance()->_dma_read_count);
                    openknx.logger.logWithValues("    Blocking Writes:   %u", WIRE1_PIO.getInstance()->_blocking_write_count);
                    openknx.logger.logWithValues("    Blocking Reads:    %u", WIRE1_PIO.getInstance()->_blocking_read_count);
#endif
                }
#endif
#endif

                openknx.logger.log("");
                openknx.logger.end();
                return;
            }

            // Bus-specific info
            openknx.logger.begin();
            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);

#ifdef OPENKNX_WIRE_PIO
            if (busName == "wire_pio")
            {
                openknx.logger.log("WIRE_PIO Details");
                openknx.logger.color(0);
                openknx.logger.log("");
                openknx.logger.log("Bus Configuration:");
                openknx.logger.log("  Type:      PIO I2C");
                openknx.logger.logWithValues("  Pins:      SDA=GPIO%d, SCL=GPIO%d", OPENKNX_WIRE_PIO_SDA_PIN, OPENKNX_WIRE_PIO_SCL_PIN);
                openknx.logger.logWithValues("  Clock:     %d kHz", OPENKNX_WIRE_PIO_CLOCK / 1000);
#ifdef OPENKNX_DEBUG
                openknx.logger.log("");
                openknx.logger.log("Hardware Resources:");
                openknx.logger.logWithValues("  PIO:       PIO%d", WIRE_PIO.getPioIndex());
                openknx.logger.logWithValues("  SM:        %d", WIRE_PIO.getStateMachineNumber());
                openknx.logger.logWithValues("  Offset:    0x%02X", WIRE_PIO.getOffset());
#ifdef OPENKNX_PIO_I2C_DMA
                if (WIRE_PIO.getInstance() && WIRE_PIO.getInstance()->_dma_available)
                {
                    openknx.logger.log("");
                    openknx.logger.log("DMA Configuration:");
                    openknx.logger.logWithValues("  TX Channel: %d", WIRE_PIO.getInstance()->_dma_tx);
                    openknx.logger.logWithValues("  RX Channel: %d", WIRE_PIO.getInstance()->_dma_rx);
                }
#endif
#endif
                openknx.logger.log("");
                openknx.logger.end();
                return;
            }
#endif

#ifdef OPENKNX_WIRE1_PIO
            if (busName == "wire1_pio")
            {
                openknx.logger.log("WIRE1_PIO Details");
                openknx.logger.color(0);
                openknx.logger.log("");
                openknx.logger.log("Bus Configuration:");
                openknx.logger.log("  Type:      PIO I2C");
                openknx.logger.logWithValues("  Pins:      SDA=GPIO%d, SCL=GPIO%d", OPENKNX_WIRE1_PIO_SDA_PIN, OPENKNX_WIRE1_PIO_SCL_PIN);
                openknx.logger.logWithValues("  Clock:     %d kHz", OPENKNX_WIRE1_PIO_CLOCK / 1000);
#ifdef OPENKNX_DEBUG
                openknx.logger.log("");
                openknx.logger.log("Hardware Resources:");
                openknx.logger.logWithValues("  PIO:       PIO%d", WIRE1_PIO.getPioIndex());
                openknx.logger.logWithValues("  SM:        %d", WIRE1_PIO.getStateMachineNumber());
                openknx.logger.logWithValues("  Offset:    0x%02X", WIRE1_PIO.getOffset());
#ifdef OPENKNX_PIO_I2C_DMA
                if (WIRE1_PIO.getInstance() && WIRE1_PIO.getInstance()->_dma_available)
                {
                    openknx.logger.log("");
                    openknx.logger.log("DMA Configuration:");
                    openknx.logger.logWithValues("  TX Channel: %d", WIRE1_PIO.getInstance()->_dma_tx);
                    openknx.logger.logWithValues("  RX Channel: %d", WIRE1_PIO.getInstance()->_dma_rx);
                }
#endif
#endif
                openknx.logger.log("");
                openknx.logger.end();
                return;
            }
#endif

            openknx.logger.color(0);
            openknx.logger.logWithPrefixAndValues("i2c", "Error: Unknown bus '%s'", busName.c_str());
            openknx.logger.end();
        }

        inline void Console::showStats()
        {
#ifdef OPENKNX_PIO_I2C_DMA
            openknx.logger.begin();
            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("I2C DMA Statistics");
            openknx.logger.color(0);

#ifdef OPENKNX_WIRE_PIO
            if (WIRE_PIO.getInstance() && WIRE_PIO.getInstance()->_dma_available)
            {
                openknx.logger.log("");
                openknx.logger.log("WIRE_PIO:");
#ifdef OPENKNX_DEBUG
                openknx.logger.logWithValues("  DMA Writes:  %u", WIRE_PIO.getInstance()->_dma_write_count);
                openknx.logger.logWithValues("  DMA Reads:   %u", WIRE_PIO.getInstance()->_dma_read_count);
                openknx.logger.logWithValues("  Blocking Wr: %u", WIRE_PIO.getInstance()->_blocking_write_count);
                openknx.logger.logWithValues("  Blocking Rd: %u", WIRE_PIO.getInstance()->_blocking_read_count);
#endif
                uint32_t total = WIRE_PIO.getInstance()->_dma_write_count + WIRE_PIO.getInstance()->_dma_read_count + 
                                 WIRE_PIO.getInstance()->_blocking_write_count + WIRE_PIO.getInstance()->_blocking_read_count;
                if (total > 0)
                {
                    uint32_t dma_total = WIRE_PIO.getInstance()->_dma_write_count + WIRE_PIO.getInstance()->_dma_read_count;
                    openknx.logger.logWithValues("  DMA Usage:   %u%%", (dma_total * 100) / total);
                }
            }
            else
            {
                openknx.logger.log("");
                openknx.logger.log("WIRE_PIO: DMA not available");
            }
#endif

#ifdef OPENKNX_WIRE1_PIO
            if (WIRE1_PIO.getInstance() && WIRE1_PIO.getInstance()->_dma_available)
            {
                openknx.logger.log("");
                openknx.logger.log("WIRE1_PIO:");
#ifdef OPENKNX_DEBUG
                openknx.logger.logWithValues("  DMA Writes:  %u", WIRE1_PIO.getInstance()->_dma_write_count);
                openknx.logger.logWithValues("  DMA Reads:   %u", WIRE1_PIO.getInstance()->_dma_read_count);
                openknx.logger.logWithValues("  Blocking Wr: %u", WIRE1_PIO.getInstance()->_blocking_write_count);
                openknx.logger.logWithValues("  Blocking Rd: %u", WIRE1_PIO.getInstance()->_blocking_read_count);
#endif
                uint32_t total = WIRE1_PIO.getInstance()->_dma_write_count + WIRE1_PIO.getInstance()->_dma_read_count + 
                                 WIRE1_PIO.getInstance()->_blocking_write_count + WIRE1_PIO.getInstance()->_blocking_read_count;
                if (total > 0)
                {
                    uint32_t dma_total = WIRE1_PIO.getInstance()->_dma_write_count + WIRE1_PIO.getInstance()->_dma_read_count;
                    openknx.logger.logWithValues("  DMA Usage:   %u%%", (dma_total * 100) / total);
                }
            }
            else
            {
                openknx.logger.log("");
                openknx.logger.log("WIRE1_PIO: DMA not available");
            }
#endif

            openknx.logger.log("");
            openknx.logger.end();
#else
            openknx.logger.logWithPrefix("i2c", "DMA statistics not available (OPENKNX_PIO_I2C_DMA not defined)");
#endif
        }

        inline void Console::scanBusImpl(TwoWire& wire, const char* name)
        {
            openknx.logger.begin();
            openknx.logger.logWithValues("Scanning %s...", name);
            openknx.logger.log("");
            openknx.logger.log("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");

            int found = 0;
            char line[100];

            for (uint8_t addr = 0; addr < 0x80; addr++)
            {
                if (addr % 16 == 0)
                    snprintf(line, sizeof(line), "%02X: ", addr);

                // Skip reserved addresses
                if ((addr & 0x78) == 0 || (addr & 0x78) == 0x78)
                {
                    strncat(line, " . ", sizeof(line) - strlen(line) - 1);
                }
                else
                {
                    wire.beginTransmission(addr);
                    uint8_t error = wire.endTransmission(false);

                    char temp[5];
                    if (error == 0)
                    {
                        snprintf(temp, sizeof(temp), " %02X", addr);
                        found++;
                    }
                    else
                    {
                        snprintf(temp, sizeof(temp), " --");
                    }
                    strncat(line, temp, sizeof(line) - strlen(line) - 1);
                }

                if (addr % 16 == 15)
                {
                    openknx.logger.log(line);
                    line[0] = '\0';
                }
            }

            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.logWithValues("Found %d device(s) on %s", found, name);
            openknx.logger.color(0);
            openknx.logger.log("");
            openknx.logger.end();
        }

        inline void Console::scanBus(const std::string& busName)
        {
            if (busName == "all")
            {
                openknx.logger.begin();
                openknx.logger.log("");
                openknx.logger.color(CONSOLE_HEADLINE_COLOR);
                openknx.logger.log("Scanning all I2C buses");
                openknx.logger.color(0);
                openknx.logger.end();

#ifdef OPENKNX_WIRE_PIO
                scanBusImpl(WIRE_PIO, "WIRE_PIO");
#endif
#ifdef OPENKNX_WIRE1_PIO
                scanBusImpl(WIRE1_PIO, "WIRE1_PIO");
#endif
#ifdef OPENKNX_WIRE
                scanBusImpl(WIRE, "WIRE");
#endif
#ifdef OPENKNX_WIRE1
                scanBusImpl(WIRE1, "WIRE1");
#endif
                return;
            }

#ifdef OPENKNX_WIRE_PIO
            if (busName == "wire_pio")
            {
                scanBusImpl(WIRE_PIO, "WIRE_PIO");
                return;
            }
#endif
#ifdef OPENKNX_WIRE1_PIO
            if (busName == "wire1_pio")
            {
                scanBusImpl(WIRE1_PIO, "WIRE1_PIO");
                return;
            }
#endif
#ifdef OPENKNX_WIRE
            if (busName == "wire")
            {
                scanBusImpl(WIRE, "WIRE");
                return;
            }
#endif
#ifdef OPENKNX_WIRE1
            if (busName == "wire1")
            {
                scanBusImpl(WIRE1, "WIRE1");
                return;
            }
#endif

            openknx.logger.logWithValues("Error: Unknown bus '%s'", busName.c_str());
        }

#ifdef OPENKNX_I2C_BENCHMARK
        inline void Console::runBenchmark(const std::string& benchType, const std::string& busName, uint8_t addr)
        {
            OpenKNX::I2C::Benchmark bench;

            if (benchType == "quick")
            {
                auto quickBench = [addr, &bench](TwoWire& wire, const char* name) {
                    openknx.logger.logWithPrefixAndValues("i2c", "Quick benchmark: %s (addr 0x%02X)", name, addr);
                    bench.benchmarkThroughput(wire, name, addr, 128, 100);
                    bench.benchmarkLatency(wire, name, addr);
                };

                if (busName == "all")
                {
#ifdef OPENKNX_WIRE_PIO
                    quickBench(WIRE_PIO, "WIRE_PIO");
#endif
#ifdef OPENKNX_WIRE1_PIO
                    quickBench(WIRE1_PIO, "WIRE1_PIO");
#endif
#ifdef OPENKNX_WIRE
                    quickBench(WIRE, "WIRE");
#endif
#ifdef OPENKNX_WIRE1
                    quickBench(WIRE1, "WIRE1");
#endif
                }
#ifdef OPENKNX_WIRE_PIO
                else if (busName == "wire_pio")
                    quickBench(WIRE_PIO, "WIRE_PIO");
#endif
#ifdef OPENKNX_WIRE1_PIO
                else if (busName == "wire1_pio")
                    quickBench(WIRE1_PIO, "WIRE1_PIO");
#endif
                else
                    openknx.logger.logWithPrefixAndValues("i2c", "Error: Unknown bus '%s'", busName.c_str());
            }
            else if (benchType == "full")
            {
                if (busName == "all")
                {
                    openknx.logger.logWithPrefix("i2c", "Running full benchmark on all buses...");
                    bench.runAllBenchmarks();
                }
                else
                {
#ifdef OPENKNX_WIRE_PIO
                    if (busName == "wire_pio")
                        bench.runBenchmark(WIRE_PIO, "WIRE_PIO", addr);
#endif
#ifdef OPENKNX_WIRE1_PIO
                    if (busName == "wire1_pio")
                        bench.runBenchmark(WIRE1_PIO, "WIRE1_PIO", addr);
#endif
                }
            }
            else
            {
                openknx.logger.logWithPrefixAndValues("i2c", "Error: Unknown benchmark type '%s'", benchType.c_str());
            }
        }
#endif

        inline std::vector<std::string> Console::tokenize(const std::string& str)
        {
            std::vector<std::string> tokens;
            size_t pos = 0;
            while (pos < str.length())
            {
                while (pos < str.length() && str[pos] == ' ')
                    pos++;
                if (pos >= str.length()) break;

                size_t tokenStart = pos;
                while (pos < str.length() && str[pos] != ' ')
                    pos++;
                tokens.push_back(str.substr(tokenStart, pos - tokenStart));
            }
            return tokens;
        }

    } // namespace I2C
} // namespace OpenKNX
