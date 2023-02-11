#include "../Helper.h"
#include "knxprod.h"
#include <knx.h>
#include <string>

namespace OpenKNX
{

    struct Information
    {
        uint16_t _applicationNumber = 0;
        uint8_t _applicationVersion = 0;
        uint8_t _firmwareRevision = 0;

        uint16_t applicationNumber()
        {
            return _applicationNumber;
        }

        void applicationNumber(uint16_t applicationNumber)
        {
            _applicationNumber = applicationNumber;
        }

        std::string humanApplicationNumber()
        {
            char* buffer = new char[10];
            sprintf(buffer, "0x%04X", applicationNumber());

            std::string returnValue(buffer);
            delete[] buffer;
            return returnValue;
        }

        uint16_t applicationVersion()
        {
            return _applicationVersion;
        }

        void applicationVersion(uint8_t applicationVersion)
        {
            _applicationVersion = applicationVersion;
        }

        std::string humanApplicationVersion()
        {
            char* buffer = new char[10];
            sprintf(buffer, "%i.%i", ((applicationVersion() & 0xF0) >> 4), (applicationVersion() & 0x0F));

            std::string returnValue(buffer);
            delete[] buffer;
            return returnValue;
        }

        uint16_t firmwareNumber()
        {
            return (MAIN_OpenKnxId << 8) + MAIN_ApplicationNumber;
        }

        std::string humanFirmwareNumber()
        {
            char* buffer = new char[10];
            sprintf(buffer, "0x%04X", firmwareNumber());

            std::string returnValue(buffer);
            delete[] buffer;
            return returnValue;
        }

        uint16_t firmwareVersion()
        {
            return ((_firmwareRevision & 0x1F) << 11) | ((MAIN_ApplicationVersion & 0xF0) << 2) | (MAIN_ApplicationVersion & 0x0F);
        }

        void firmwareRevision(uint8_t firmwareRevision)
        {
            _firmwareRevision = firmwareRevision;
        }

        std::string humanFirmwareVersion()
        {
            char* buffer = new char[14];
            sprintf(buffer, "%i.%i.%i", ((firmwareVersion() & 0x03C0) >> 6), (firmwareVersion() & 0x000F), ((firmwareVersion() & 0xF800) >> 11));

            std::string returnValue(buffer);
            delete[] buffer;
            return returnValue;
        }

        uint16_t individualAddress()
        {
            return knx.individualAddress();
        }

        std::string humanIndividualAddress()
        {
            char* buffer = new char[14];
            sprintf(buffer, "%i.%i.%i", ((knx.individualAddress() & 0xF000) >> 12), ((knx.individualAddress() & 0x0F00) >> 8), (knx.individualAddress() & 0x00FF));

            std::string returnValue(buffer);
            delete[] buffer;
            return returnValue;
        }
    };

} // namespace OpenKNX