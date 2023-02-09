#pragma once
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

      public:
        /*
         * The name. Used for "log" methods.
         * @return name
         */
        virtual const char* name();

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
         * Called on incomming/changing GroupObject
         * @param GroupObject
         */
        virtual void processInputKo(GroupObject& ko);
    };
} // namespace OpenKNX