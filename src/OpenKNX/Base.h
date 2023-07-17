#pragma once
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/defines.h"
#include <knx.h>

namespace OpenKNX
{
    /*
     * Abstract Base class for classes "Module" and "Channel"
     */
    class Base
    {
      protected:
        /*
         * Wrapper for openknx.log. The name() will be used as prefix.
         */
        void log(const char *output, ...);

        /*
         * Wrapper for openknx.logHex. The name() will be used as prefix.
         */
        void logHex(const uint8_t *data, size_t size);

        /*
         * Get a Pointer to a prefix for Log
         * THe point need to be delete[] after usage!
         */
        virtual const std::string logPrefix();

      public:
        /*
         * The name. Used for "log" methods.
         * @return name
         */
        virtual const std::string name();

        /*
         * Called after knx.init() before setup() called.
         * Will be executed regardless of knx.configured state
         */
        virtual void init();

        /*
         * Called during startup after initialization of all modules is completed.
         * Useful for init interrupts on core0
         */
        virtual void setup(bool configured);
        virtual void setup();

        /*
         * Module logic for core0
         */
        virtual void loop(bool configured);
        virtual void loop();

#ifdef OPENKNX_DUALCORE
        /*
         * Called during startup after setup() completed
         * Useful for init interrupts on core1
         */
        virtual void setup1(bool configured);
        virtual void setup1();

        /*
         * Module logic for core1
         */
        virtual void loop1(bool configured);
        virtual void loop1();
#endif

#if (MASK_VERSION & 0x0900) != 0x0900 // Coupler do not have GroupObjects
        /*
         * Called on incoming/changing GroupObject
         * @param GroupObject
         */
        virtual void processInputKo(GroupObject &ko);
#endif

        /*
         * Called on incoming function property command
         */

        virtual bool processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
        /*
         * Called on incoming function property state command
         */

        virtual bool processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    };
} // namespace OpenKNX
