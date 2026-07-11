#pragma once
/**
 * @file        Wire1Lock.h
 * @brief       Shared mutex for the display-shared Wire1 (OPENKNX_GPIO_WIRE) I2C bus
 * @version     0.0.1
 * @date        2026-07-11
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 *
 * On REG2 the SSD1306 display and the PCA9557 (front LEDs + buttons) share one I2C master;
 * concurrent transactions corrupt each other (e.g. PCA9557 CONFIG gets rewritten -> LED pin
 * flips back to INPUT). On ESP32 the LED soft-PWM flush runs in a separate esp_timer task, so
 * every Wire1 transaction must be serialized. Non-recursive: take once per top-level transaction.
 * On RP2040/non-ESP the flush stays in the main loop, so the lock is a compile-time no-op.
 */

#ifdef ARDUINO_ARCH_ESP32
    #include "freertos/FreeRTOS.h"
    #include "freertos/semphr.h"
#endif

namespace OpenKNX
{
    namespace I2C
    {
#ifdef ARDUINO_ARCH_ESP32

        /**
         * @brief Lazily-created, process-wide mutex for the Wire1 bus.
         */
        inline SemaphoreHandle_t wire1Mutex()
        {
            static SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
            return mutex;
        }

        /**
         * @brief Take the Wire1 mutex (blocking). Only called from task/main-loop context, never an ISR.
         */
        inline void wire1Take()
        {
            SemaphoreHandle_t m = wire1Mutex();
            if (m != nullptr)
                xSemaphoreTake(m, portMAX_DELAY);
        }

        /**
         * @brief Release the Wire1 mutex.
         */
        inline void wire1Give()
        {
            SemaphoreHandle_t m = wire1Mutex();
            if (m != nullptr)
                xSemaphoreGive(m);
        }

        /**
         * @brief Scoped RAII guard: takes the Wire1 mutex on construction, releases on scope exit.
         */
        class Wire1Guard
        {
          public:
            Wire1Guard() { wire1Take(); }
            ~Wire1Guard() { wire1Give(); }

            Wire1Guard(const Wire1Guard&) = delete;
            Wire1Guard& operator=(const Wire1Guard&) = delete;
        };

#else // !ARDUINO_ARCH_ESP32

        // No concurrent flush timer -> lock is a no-op.
        inline void wire1Take() {}
        inline void wire1Give() {}

        class Wire1Guard
        {
          public:
            Wire1Guard() {}
            ~Wire1Guard() {}

            Wire1Guard(const Wire1Guard&) = delete;
            Wire1Guard& operator=(const Wire1Guard&) = delete;
        };

#endif

    } // namespace I2C
} // namespace OpenKNX

// Scoped Wire1 lock for the enclosing block (no-op guard on non-ESP).
#define OPENKNX_WIRE1_LOCK() OpenKNX::I2C::Wire1Guard __openknx_wire1_guard
