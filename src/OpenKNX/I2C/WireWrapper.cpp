#include "WireWrapper.h"
#include "OpenKNX.h"

//
// Global I2C Bus Instance Definitions
//
/*
 * PIO I2C Wire Instance 1
 */
#ifdef OPENKNX_WIRE_PIO
OpenKNX::I2C::PIOI2CWire WIRE_PIO( // Create WIRE_PIO instance
    OPENKNX_WIRE_PIO_SDA_PIN,      // Initialize SDA pin
    OPENKNX_WIRE_PIO_SCL_PIN,      // Initialize SCL pin
    OPENKNX_WIRE_PIO_CLOCK);       // Initialize clock speed
#endif

/*
 * PIO I2C Wire Instance 2
 */
#ifdef OPENKNX_WIRE1_PIO
OpenKNX::I2C::PIOI2CWire WIRE1_PIO( // Create WIRE1_PIO instance
    OPENKNX_WIRE1_PIO_SDA_PIN,      // Initialize SDA pin
    OPENKNX_WIRE1_PIO_SCL_PIN,      // Initialize SCL pin
    OPENKNX_WIRE1_PIO_CLOCK);       // Initialize clock speed
#endif

/*
 * Hardware I2C Wire Instance 1
 */
#ifdef OPENKNX_WIRE
TwoWire& WIRE = Wire; // Reference to global Wire instance
#endif

/*
 * Hardware I2C Wire Instance 2
 */
#ifdef OPENKNX_WIRE1
TwoWire& WIRE1 = Wire1; // Reference to global Wire1 instance
#endif

namespace OpenKNX
{
    namespace I2C
    {
        WireWrapper::WireWrapper() {}  // Constructor
        WireWrapper::~WireWrapper() {} // Destructor

        /*
         * @brief Initialize all configured I2C buses
         * Initializes:
         * - PIO I2C buses (if defined)
         * - Hardware I2C buses (if defined)
         */
        void WireWrapper::init()
        {
            if (_initialized)
            {
                logWarningP("I2C Wrapper already initialized!");
                return;
            }

            logInfoP("Initializing I2C buses...");

#ifdef OPENKNX_WIRE_PIO // WIRE_PIO - PIO I2C 1
            logInfoP("WIRE_PIO: SDA=%d, SCL=%d @ %d Hz (PIO I2C)",
                     OPENKNX_WIRE_PIO_SDA_PIN,
                     OPENKNX_WIRE_PIO_SCL_PIN,
                     OPENKNX_WIRE_PIO_CLOCK);
            WIRE_PIO.begin(OPENKNX_WIRE_PIO_SDA_PIN,
                           OPENKNX_WIRE_PIO_SCL_PIN,
                           OPENKNX_WIRE_PIO_CLOCK);
            logInfoP("WIRE_PIO initialized.");
#endif // OPENKNX_WIRE_PIO

#ifdef OPENKNX_WIRE1_PIO // WIRE1_PIO - PIO I2C 2
            logInfoP("WIRE1_PIO: SDA=%d, SCL=%d @ %d Hz (PIO I2C)",
                     OPENKNX_WIRE1_PIO_SDA_PIN,
                     OPENKNX_WIRE1_PIO_SCL_PIN,
                     OPENKNX_WIRE1_PIO_CLOCK);
            WIRE1_PIO.begin(OPENKNX_WIRE1_PIO_SDA_PIN,
                            OPENKNX_WIRE1_PIO_SCL_PIN,
                            OPENKNX_WIRE1_PIO_CLOCK);
            logInfoP("WIRE1_PIO initialized.");
#endif // OPENKNX_WIRE1_PIO

#ifdef OPENKNX_WIRE // WIRE - Hardware I2C 1
            logInfoP("WIRE: SDA=%d, SCL=%d @ %d Hz (Hardware I2C)",
                     OPENKNX_WIRE_SDA_PIN,
                     OPENKNX_WIRE_SCL_PIN,
                     OPENKNX_WIRE_CLOCK);
    #ifdef ARDUINO_ARCH_RP2040
            WIRE.setSDA(OPENKNX_WIRE_SDA_PIN);
            WIRE.setSCL(OPENKNX_WIRE_SCL_PIN);
    #elif defined(ESP32)
                // ESP32 uses begin(SDA, SCL)
    #endif
            WIRE.begin();
            WIRE.setClock(OPENKNX_WIRE_CLOCK);
            logInfoP("WIRE initialized.");
#endif // OPENKNX_WIRE

#ifdef OPENKNX_WIRE1 // WIRE1 - Hardware I2C 2
            logInfoP("WIRE1: SDA=%d, SCL=%d @ %d Hz (Hardware I2C)",
                     OPENKNX_WIRE1_SDA_PIN,
                     OPENKNX_WIRE1_SCL_PIN,
                     OPENKNX_WIRE1_CLOCK);
    #ifdef ARDUINO_ARCH_RP2040
            WIRE1.setSDA(OPENKNX_WIRE1_SDA_PIN);
            WIRE1.setSCL(OPENKNX_WIRE1_SCL_PIN);
    #elif defined(ESP32)
                // ESP32 uses begin(SDA, SCL)
    #endif

            WIRE1.begin();
            WIRE1.setClock(OPENKNX_WIRE1_CLOCK);
            logInfoP("WIRE1 initialized.");
#endif // OPENKNX_WIRE1
            _initialized = true;
            logInfoP("I2C initialization complete");

#ifdef OPENKNX_I2C_SCAN_ON_INIT // Scan all buses on init - optional and only for debugging/development/testing
            scanAllBuses();
#endif
        }

        /*
         * @brief Periodic I2C Wrapper tasks
         * Currently reserved for future features (e.g., error monitoring, bus recovery)
         */
        void WireWrapper::loop() {} // Currently empty

        /*
         * @brief Scan an I2C bus for devices
         * @param wire Reference to the I2C bus to scan
         * @param name Name of the bus (for logging)
         */
        void WireWrapper::scanBus(TwoWire& wire, const char* name)
        {
            logInfoP("Scanning %s for devices...", name);

            uint8_t deviceCount = 0;

            for (uint8_t addr = 0x08; addr < 0x78; addr++)
            {
                wire.beginTransmission(addr);
                uint8_t error = wire.endTransmission(false); // Do not send stop condition on scan

                if (error == 0)
                {
                    logInfoP("  Device found at 0x%02X", addr);
                    deviceCount++;
                }
            }

            if (deviceCount == 0)
            {
                logInfoP("  No devices found on %s", name);
            }
            else
            {
                logInfoP("  Found %d device(s) on %s", deviceCount, name);
            }
        }

        /*
         * @brief Scan all configured I2C buses
         */
        void WireWrapper::scanAllBuses()
        {
            logInfoP("Scanning all I2C buses...");

#ifdef OPENKNX_WIRE_PIO
            scanBus(WIRE_PIO, "WIRE_PIO");
#endif

#ifdef OPENKNX_WIRE1_PIO
            scanBus(WIRE1_PIO, "WIRE1_PIO");
#endif

#ifdef OPENKNX_WIRE
            scanBus(WIRE, "WIRE");
#endif

#ifdef OPENKNX_WIRE1
            scanBus(WIRE1, "WIRE1");
#endif
        }

#ifdef OPENKNX_I2C_BENCHMARK
        /*
         * @brief Run performance benchmarks on all buses - ToDo
         */
        void WireWrapper::runBenchmarks()
        {
            logInfoP("Running I2C benchmarks...");

            // Placeholder for benchmark implementation
            // Can be extended with throughput, latency tests, etc.

    #ifdef OPENKNX_WIRE_PIO
            logInfoP("WIRE_PIO benchmark: TODO");
    #endif

    #ifdef OPENKNX_WIRE1_PIO
            logInfoP("WIRE1_PIO benchmark: TODO");
    #endif

    #ifdef OPENKNX_WIRE
            logInfoP("WIRE benchmark: TODO");
    #endif

    #ifdef OPENKNX_WIRE1
            logInfoP("WIRE1 benchmark: TODO");
    #endif
        }
#endif
    } // namespace I2C
} // namespace OpenKNX
