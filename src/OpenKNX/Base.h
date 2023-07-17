#pragma once
#include "OpenKNX/Log/Logger.h"
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
         * Called at startup (before startup delay), after knx.init
         * will be executed regardless of knx.configured state
         */
        virtual void init();

        /*
         * Called at startup (before startup delay)
         * Useful for init hardware
         */
        virtual void setup();

        /*
         * Called at startup for second core (before startup delay), after knx.init
         * will be executed regardless of knx.configured state
         */
        virtual void init1();

        /*
         * Called at startup for second core (before startup delay)
         * Useful for init hardware
         */
        virtual void setup1();

        /*
         * Module logic
         */
        virtual void loop();

        /*
         * Module logic for second core
         */
        virtual void loop1();

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
