#pragma once
#include "OpenKNX/Logger.h"
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
        void log(const char* output, ...);

        /*
         * Wrapper for openknx.logHex. The name() will be used as prefix.
         */
        void logHex(const uint8_t* data, size_t size);

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
         * Called at startup (before startup delay)
         * Useful for init hardware
         */
        virtual void setup();

        /*
         * Module logic
         */
        virtual void loop();

        /*
         * Module logic for second core
         */
        virtual void loop2();

        /*
         * Called on incomming/changing GroupObject
         * @param GroupObject
         */
        virtual void processInputKo(GroupObject& ko);
    };
} // namespace OpenKNX
