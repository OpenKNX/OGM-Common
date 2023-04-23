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
         * Called on incoming/changing GroupObject
         * @param GroupObject
         */
        virtual void processInputKo(GroupObject& ko);

        /*
         * Called on incoming function property command
         */
        virtual bool processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
        /*
         * Called on incoming function property state command
         */
        virtual bool processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
        /*
         * Called on diagnose request from diagnose KO
         * New multiline capability: As long as a response is generated, the diagnose
         * command is called up to 10 times (increasing count) to allow multiline output
         * Use this carefully to avoid high KNX bus load
         * @param input  - diagnose command
         * @param output - diagnose response
         * @param count  - counter in case of multiline processing
         * @return true if there is an output in diagnose response 
         */
        virtual bool processDiagnoseCommand(const char *input, char *output, uint8_t count);
    };
} // namespace OpenKNX
