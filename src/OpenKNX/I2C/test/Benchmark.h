/**
 * OpenKNX I2C Wrapper - Comprehensive Benchmark Suite
 *
 * This file provides extensive benchmark and testing functionality when OPENKNX_I2C_BENCHMARK is defined.
 * Includes performance comparisons, stability tests, and protocol validation.
 *
 * @file        Benchmark.h
 * @version     1.0.0
 * @date        2025-10-30
 * @copyright   Copyright (c) 2025, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 */

#pragma once

#ifdef OPENKNX_I2C_BENCHMARK

    #include "OpenKNX/I2C/WireWrapper.h"
    #include <pico/stdlib.h>

namespace OpenKNX
{
    namespace I2C
    {
        class Benchmark
        {
          public:
            // ====================================================================
            // HELPER FUNCTIONS
            // ====================================================================

            /**
             * @brief Check if an I2C address is reserved
             */
            inline bool reserved_addr(uint8_t addr)
            {
                return (addr & 0x78) == 0 || (addr & 0x78) == 0x78; // Reserved addresses, below 0x08 and above 0x77 are reserved for special purposes
            }

            /**
             * @brief Print a formatted separator line
             */
            inline void print_separator(const char* title = nullptr)
            {
                openknx.logger.log("================================================");
                if (title)
                {
                    openknx.logger.log(title);
                    openknx.logger.log("================================================");
                }
            }

            // ====================================================================
            // BASIC BENCHMARKS
            // ====================================================================

            /**
             * @brief Benchmark throughput for an I2C bus
             *
             * @param wire I2C bus to test
             * @param name Bus name for logging
             * @param address Device address to test with
             * @param dataSize Size of data packet in bytes
             * @param iterations Number of iterations
             */
            inline void benchmarkThroughput(TwoWire& wire, const char* name, uint8_t address, uint16_t dataSize, uint16_t iterations)
            {
                openknx.logger.logWithValues("Benchmark %s: %d bytes x %d iterations", name, dataSize, iterations);

                uint8_t* data = new uint8_t[dataSize];
                if (!data)
                {
                    openknx.logger.log("  Memory allocation failed!");
                    return;
                }

                memset(data, 0xAA, dataSize);

                uint32_t start = micros();
                uint16_t errors = 0;

                for (int i = 0; i < iterations; i++)
                {
                    wire.beginTransmission(address);
                    wire.write(0x40); // Data mode (adjust for your device)
                    wire.write(data, dataSize);
                    uint8_t err = wire.endTransmission();
                    if (err != 0) errors++;
                }

                uint32_t duration = micros() - start;
                delete[] data;

                float bytesPerSec = ((float)(iterations * dataSize) * 1000000.0f) / duration;
                float kbps = (bytesPerSec * 8.0f) / 1000.0f;

                openknx.logger.logWithValues("  Duration:    %d µs", duration);
                openknx.logger.logWithValues("  Throughput:  %.1f KB/s (%.0f kbit/s)", bytesPerSec / 1024, kbps);
                openknx.logger.logWithValues("  Errors:      %d", errors);
            }

            /**
             * @brief Benchmark latency for single transfers
             *
             * @param wire I2C bus to test
             * @param name Bus name for logging
             * @param address Device address to test with
             */
            inline void benchmarkLatency(TwoWire& wire, const char* name, uint8_t address)
            {
                openknx.logger.logWithValues("Latency Test %s:", name);

                uint32_t minTime = UINT32_MAX;
                uint32_t maxTime = 0;
                uint32_t totalTime = 0;
                const int iterations = 100;

                for (int i = 0; i < iterations; i++)
                {
                    uint32_t start = micros();

                    wire.beginTransmission(address);
                    wire.write(0x00);
                    wire.write(0xAF);
                    wire.endTransmission();

                    uint32_t duration = micros() - start;

                    if (duration < minTime) minTime = duration;
                    if (duration > maxTime) maxTime = duration;
                    totalTime += duration;
                }

                float avgTime = (float)totalTime / iterations;

                openknx.logger.logWithValues("  Min: %d µs", minTime);
                openknx.logger.logWithValues("  Avg: %.1f µs", avgTime);
                openknx.logger.logWithValues("  Max: %d µs", maxTime);
            }

            // ====================================================================
            // BUS SCANNING
            // ====================================================================

            /**
             * @brief Scan I2C bus and display device map
             *
             * @param wire I2C bus to scan
             * @param name Bus name for logging
             * @return Number of devices found
             */
            inline uint8_t scanBusDetailed(TwoWire& wire, const char* name)
            {
                openknx.logger.logWithValues("Scanning %s for devices...", name);
                openknx.logger.log("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");

                uint8_t deviceCount = 0;
                char line[100];

                for (uint8_t addr = 0; addr < 0x80; addr++)
                {
                    if (addr % 16 == 0)
                    {
                        snprintf(line, sizeof(line), "%02X: ", addr);
                    }

                    if (reserved_addr(addr))
                    {
                        strncat(line, " . ", sizeof(line) - strlen(line) - 1);
                    }
                    else
                    {
                        wire.beginTransmission(addr);
                        uint8_t error = wire.endTransmission(false); // No stop condition on scan

                        char temp[5];
                        if (error == 0)
                        {
                            snprintf(temp, sizeof(temp), " %02X", addr);
                            deviceCount++;
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
                        line[0] = '\0'; // Reset line
                    }
                }

                openknx.logger.logWithValues("%s scan complete. Devices found: %d\n", name, deviceCount);
                return deviceCount;
            }

            // ====================================================================
            // SPEED COMPARISON
            // ====================================================================

            /**
             * @brief Compare performance at different speeds
             *
             * @param wire I2C bus to test
             * @param name Bus name for logging
             * @param address Device address
             * @param dataSize Data packet size
             */
            inline void benchmarkSpeedComparison(TwoWire& wire, const char* name, uint8_t address, uint16_t dataSize = 128)
            {
                print_separator("Speed Comparison Test");
                openknx.logger.logWithValues("Testing %s with %d byte packets", name, dataSize);

                uint32_t speeds[] = {100000, 200000, 400000, 600000, 800000};
                const char* speedNames[] = {"100 kHz", "200 kHz", "400 kHz", "600 kHz", "800 kHz"};

                uint8_t* data = new uint8_t[dataSize];
                if (!data)
                {
                    openknx.logger.log("Memory allocation failed!");
                    return;
                }
                memset(data, 0xAA, dataSize);

                openknx.logger.log("Speed    │ Duration  │ Throughput   │ Errors");
                openknx.logger.log("─────────┼───────────┼──────────────┼────────");

                for (int s = 0; s < 5; s++)
                {
                    wire.setClock(speeds[s]);
                    delay(50);

                    uint32_t start = micros();
                    uint16_t errors = 0;
                    const int iterations = 50;

                    for (int i = 0; i < iterations; i++)
                    {
                        wire.beginTransmission(address);
                        wire.write(0x40);
                        wire.write(data, dataSize);
                        uint8_t err = wire.endTransmission();
                        if (err != 0) errors++;
                    }

                    uint32_t duration = micros() - start;
                    float throughput = ((float)(iterations * dataSize) * 1000000.0f) / (duration * 1024.0f);

                    openknx.logger.logWithValues("%-8s │ %6d µs │ %7.1f KB/s │ %d",
                                                 speedNames[s], duration, throughput, errors);
                }

                delete[] data;
            }

            // ====================================================================
            // STABILITY TESTS
            // ====================================================================

            /**
             * @brief Test error rate at various speeds
             *
             * @param wire I2C bus to test
             * @param name Bus name for logging
             * @param address Device address
             */
            inline void benchmarkStability(TwoWire& wire, const char* name, uint8_t address)
            {
                print_separator("Stability Test (Error Rate)");
                openknx.logger.logWithValues("Testing %s with 50 transactions per speed", name);

                uint32_t testSpeeds[] = {100000, 200000, 400000, 600000, 800000, 1000000};
                const char* speedLabels[] = {"100k", "200k", "400k", "600k", "800k", "1MHz"};

                openknx.logger.log("Speed  │ Errors    │ Success Rate │ Status");
                openknx.logger.log("───────┼───────────┼──────────────|────────");

                for (int s = 0; s < 6; s++)
                {
                    wire.setClock(testSpeeds[s]);
                    delay(50);

                    uint16_t errors = 0;
                    const int iterations = 50;

                    for (int i = 0; i < iterations; i++)
                    {
                        wire.beginTransmission(address);
                        wire.write(0x00);
                        wire.write(0xAE);
                        uint8_t err = wire.endTransmission();
                        if (err != 0) errors++;
                    }

                    float successRate = ((iterations - errors) / (float)iterations) * 100.0f;
                    openknx.logger.logWithValues("%-6s │ %2d/%-2d     │ %5.1f%%       | %4s",
                                                 speedLabels[s], errors, iterations,
                                                 successRate,
                                                 errors == 0 ? " OK" : (errors < 5 ? "WARN" : "FAIL"));
                }
            }

            // ====================================================================
            // REPEATED START TEST
            // ====================================================================

            /**
             * @brief Test repeated start functionality
             *
             * @param wire I2C bus to test
             * @param name Bus name for logging
             * @param address Device address
             */
            inline void testRepeatedStart(TwoWire& wire, const char* name, uint8_t address)
            {
                print_separator("Repeated Start Test");
                openknx.logger.logWithValues("Testing %s with device 0x%02X\n", name, address);

                const int iterations = 20;
                uint16_t errors = 0;

                for (int i = 0; i < iterations; i++)
                {
                    // Write register address without STOP
                    wire.beginTransmission(address);
                    wire.write(0x00);                           // Register address
                    uint8_t err1 = wire.endTransmission(false); // NO STOP

                    if (err1 != 0)
                    {
                        errors++;
                        continue;
                    }

                    // Read with repeated start
                    uint8_t count = wire.requestFrom(address, (size_t)1, true);
                    if (count != 1)
                    {
                        errors++;
                        continue;
                    }

                    uint8_t value = wire.read();
                    (void)value; // Suppress unused warning
                }

                float successRate = ((iterations - errors) / (float)iterations) * 100.0f;
                openknx.logger.log("Repeated Start Results:");
                openknx.logger.logWithValues("  Iterations: %d", iterations);
                openknx.logger.logWithValues("  Errors:     %d", errors);
                openknx.logger.logWithValues("  Success:    %.1f%%", successRate);

                if (errors == 0)
                {
                    openknx.logger.log("  Status:     PASS");
                }
                else
                {
                    openknx.logger.log("  Status:     FAIL");
                }
            }

            // ====================================================================
            // DISPLAY UPDATE BENCHMARK
            // ====================================================================

            /**
             * @brief Benchmark typical display update scenario
             *
             * @param wire I2C bus to test
             * @param name Bus name for logging
             * @param address Display address (typically 0x3C)
             */
            inline void benchmarkDisplayUpdate(TwoWire& wire, const char* name, uint8_t address = 0x3C)
            {
                print_separator("Display Update Benchmark");
                openknx.logger.logWithValues("Testing %s with display at 0x%02X", name, address);

                // Typical display buffer size
                const uint16_t displayBufferSize = 1024;
                uint8_t* buffer = new uint8_t[displayBufferSize];
                if (!buffer)
                {
                    openknx.logger.log("Memory allocation failed!");
                    return;
                }

                memset(buffer, 0xAA, displayBufferSize);

                uint32_t speeds[] = {100000, 200000, 400000};
                const char* speedNames[] = {"100 kHz", "200 kHz", "400 kHz"};

                openknx.logger.log("Speed    │ FPS    │ Frame Time │ Throughput");
                openknx.logger.log("─────────┼────────┼────────────┼────────────");

                for (int s = 0; s < 3; s++)
                {
                    wire.setClock(speeds[s]);
                    delay(50);

                    const int iterations = 10;
                    uint32_t start = micros();

                    for (int i = 0; i < iterations; i++)
                    {
                        wire.beginTransmission(address);
                        wire.write(0x40); // Data mode

                        // Send in chunks (typical display behavior)
                        for (int chunk = 0; chunk < displayBufferSize; chunk += 32)
                        {
                            size_t chunkSize = (displayBufferSize - chunk) > 32 ? 32 : (displayBufferSize - chunk);
                            wire.write(buffer + chunk, chunkSize);
                        }

                        wire.endTransmission();
                    }

                    uint32_t duration = micros() - start;
                    float avgFrameTime = duration / (float)iterations;
                    float fps = 1000000.0f / avgFrameTime;
                    float throughput = (displayBufferSize * iterations * 1000000.0f) / (duration * 1024.0f);

                    openknx.logger.logWithValues("%-8s │ %5.1f  │ %7.0f µs │ %7.1f KB/s",
                                                 speedNames[s], fps, avgFrameTime, throughput);
                }

                delete[] buffer;
            }

            // ====================================================================
            // PIO vs HARDWARE COMPARISON
            // ====================================================================

    #if defined(OPENKNX_WIRE_PIO) && defined(OPENKNX_WIRE)
            /**
             * @brief Direct comparison between PIO and Hardware I2C
             */
            inline void comparePIOvsHardware(uint8_t address = 0x3C)
            {
                print_separator("PIO I2C vs Hardware I2C Comparison");

                openknx.logger.log("Configuration:");
                openknx.logger.log("  PIO I2C:      WIRE_PIO");
                openknx.logger.log("  Hardware I2C: WIRE");
                openknx.logger.logWithValues("  Test Address: 0x%02X", address);

                struct TestResult
                {
                    uint32_t duration_us;
                    uint16_t errors;
                };

                TestResult pioResults[5];
                TestResult wireResults[5];

                uint32_t speeds[] = {100000, 200000, 400000, 600000, 800000};
                const char* speedNames[] = {"100 kHz", "200 kHz", "400 kHz", "600 kHz", "800 kHz"};

                const uint16_t testSize = 128;
                const int iterations = 50;

                uint8_t* data = new uint8_t[testSize];
                if (!data)
                {
                    openknx.logger.log("Memory allocation failed!");
                    return;
                }
                memset(data, 0xAA, testSize);

                // Test PIO I2C
                openknx.logger.log("\nTesting PIO I2C...");
                for (int s = 0; s < 5; s++)
                {
                    WIRE_PIO.setClock(speeds[s]);
                    delay(50);

                    uint32_t start = micros();
                    uint16_t errors = 0;

                    for (int i = 0; i < iterations; i++)
                    {
                        WIRE_PIO.beginTransmission(address);
                        WIRE_PIO.write(0x40);
                        WIRE_PIO.write(data, testSize);
                        uint8_t err = WIRE_PIO.endTransmission();
                        if (err != 0) errors++;
                    }

                    uint32_t duration = micros() - start;
                    pioResults[s].duration_us = duration;
                    pioResults[s].errors = errors;

                    openknx.logger.logWithValues("  %s: %d µs, %d errors", speedNames[s], duration, errors);
                }

                // Test Hardware I2C
                openknx.logger.log("\nTesting Hardware I2C...");
                for (int s = 0; s < 5; s++)
                {
                    WIRE.setClock(speeds[s]);
                    delay(50);

                    uint32_t start = micros();
                    uint16_t errors = 0;

                    for (int i = 0; i < iterations; i++)
                    {
                        WIRE.beginTransmission(address);
                        WIRE.write(0x40);
                        WIRE.write(data, testSize);
                        uint8_t err = WIRE.endTransmission();
                        if (err != 0) errors++;
                    }

                    uint32_t duration = micros() - start;
                    wireResults[s].duration_us = duration;
                    wireResults[s].errors = errors;

                    openknx.logger.logWithValues("  %s: %d µs, %d errors", speedNames[s], duration, errors);
                }

                delete[] data;

                // Results table
                openknx.logger.log("");
                openknx.logger.log("╔════════════════════════════════════════════════════════════╗");
                openknx.logger.log("║              PERFORMANCE COMPARISON                        ║");
                openknx.logger.log("╚════════════════════════════════════════════════════════════╝");

                openknx.logger.log("Speed    │ PIO I2C    │ Hardware   │ Difference  │ Winner");
                openknx.logger.log("─────────┼────────────┼────────────┼─────────────┼─────────");

                for (int i = 0; i < 5; i++)
                {
                    float pioThroughput = (testSize * iterations * 1000000.0f) / (pioResults[i].duration_us * 1024.0f);
                    float wireThroughput = (testSize * iterations * 1000000.0f) / (wireResults[i].duration_us * 1024.0f);
                    float diff = ((pioThroughput - wireThroughput) / wireThroughput) * 100.0f;

                    openknx.logger.logWithValues("%-8s │ %7.1f KB/s │ %7.1f KB/s │ %+6.1f%%    │ ",
                                                 speedNames[i], pioThroughput, wireThroughput, diff);

                    if (diff > 0)
                    {
                        openknx.logger.log("PIO");
                    }
                    else
                    {
                        openknx.logger.log("HW");
                    }
                }

                openknx.logger.log("\n _/ Comparison complete");
            }
    #endif

            // ====================================================================
            // COMPREHENSIVE BENCHMARK SUITE
            // ====================================================================

            /**
             * @brief Run comprehensive benchmarks on a bus
             *
             * @param wire I2C bus to test
             * @param name Bus name for logging
             * @param address Device address to test with (default: 0x3C for displays)
             */
            inline void runBenchmark(TwoWire& wire, const char* name, uint8_t address = 0x3C)
            {
                openknx.logger.log("");
                openknx.logger.log("=================================================");
                openknx.logger.logWithValues("Benchmark: %s", name);
                openknx.logger.log("=================================================");

                // Test 1: Device scanning
                scanBusDetailed(wire, name);
                delay(500);

                // Test 2: Small packets throughput
                benchmarkThroughput(wire, name, address, 8, 500);
                delay(500);

                // Test 3: Medium packets throughput
                benchmarkThroughput(wire, name, address, 128, 200);
                delay(500);

                // Test 4: Large packets throughput
                benchmarkThroughput(wire, name, address, 256, 100);
                delay(500);

                // Test 5: Latency measurement
                benchmarkLatency(wire, name, address);
                delay(500);

                // Test 6: Speed comparison
                benchmarkSpeedComparison(wire, name, address, 128);
                delay(500);

                // Test 7: Stability testing
                benchmarkStability(wire, name, address);
                delay(500);

                // Test 8: Repeated start
                testRepeatedStart(wire, name, address);
                delay(500);

                // Test 9: Display update simulation
                benchmarkDisplayUpdate(wire, name, address);
                delay(500);

                openknx.logger.log("=================================================\n");
            }

            /**
             * @brief Run benchmarks on all configured buses
             */
            inline void runAllBenchmarks()
            {
                openknx.logger.log("\n");
                openknx.logger.log("╔════════════════════════════════════════════════╗");
                openknx.logger.log("║     OpenKNX I2C Wrapper - Benchmarks           ║");
                openknx.logger.log("╚════════════════════════════════════════════════╝");
                openknx.logger.log("");

    #ifdef OPENKNX_WIRE_PIO
                runBenchmark(WIRE_PIO, "WIRE_PIO");
                delay(1000);
    #endif

    #ifdef OPENKNX_WIRE1_PIO
                runBenchmark(WIRE1_PIO, "WIRE1_PIO");
                delay(1000);
    #endif

    #ifdef OPENKNX_WIRE
                runBenchmark(WIRE, "WIRE (Hardware)");
                delay(1000);
    #endif

    #ifdef OPENKNX_WIRE1
                runBenchmark(WIRE1, "WIRE1 (Hardware)");
                delay(1000);
    #endif

    #if defined(OPENKNX_WIRE_PIO) && defined(OPENKNX_WIRE)
                // Direct comparison if both are available
                comparePIOvsHardware();
    #endif

                openknx.logger.log("\n_/ All benchmarks complete\n");
            }
        }; // class PIOI2CWire
    } // namespace I2C
} // namespace OpenKNX

#endif // OPENKNX_I2C_BENCHMARK