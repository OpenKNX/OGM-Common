#pragma once
// #include "../Helper.h"
#include "knxprod.h"
#include <knx.h>
#include <string>

namespace OpenKNX
{

    struct Information
    {
        uint32_t _serialNumber = 0;
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
            char buffer[5] = {};
            sprintf(buffer, "%04X", applicationNumber());
            return std::string(buffer);
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
            char buffer[10] = {};
            sprintf(buffer, "%i.%i", ((applicationVersion() & 0xF0) >> 4), (applicationVersion() & 0x0F));
            return std::string(buffer);
        }

        std::string firmwareName()
        {
#ifdef FIRMWARE_NAME
            return std::string(FIRMWARE_NAME);
#else
            return std::string(MAIN_OrderNumber);
#endif
        }

        /**
         * Get the firmware number
         * @result a 2 byte value consisting of MAIN_OpenKnxId (uint8_t) in
         * higher byte followed ny MAIN_ApplicationNumber (uint8_t) in lower byte.
         */
        uint16_t firmwareNumber()
        {
            return (MAIN_OpenKnxId << 8) + MAIN_ApplicationNumber;
        }

        std::string humanFirmwareNumber()
        {
            char buffer[5] = {};
            sprintf(buffer, "%04X", firmwareNumber());
            return std::string(buffer);
        }

        /**
         * Get firmware version, shown in ETS as '[revision] major.minor'
         * @result a 2 byte value with bits 'rrrr_rMMM MMmm_mmmm' of
         * - r: 5 bit revision
         * - M: 5 bit major
         * - m: 6 bit minor
         */
        uint16_t firmwareVersion()
        {
            return ((_firmwareRevision & 0x1F) << 11) | ((MAIN_ApplicationVersion & 0xF0) << 2) | (MAIN_ApplicationVersion & 0x0F);
        }

        void firmwareRevision(uint8_t firmwareRevision)
        {
            _firmwareRevision = firmwareRevision;
        }

        std::string humanFirmwareVersion(bool withHash = false)
        {
            char buffer[20] = {};
            sprintf(buffer, withHash ? "%i.%i.%i+%s" : "%i.%i.%i", ((firmwareVersion() & 0x03C0) >> 6), (firmwareVersion() & 0x000F), ((firmwareVersion() & 0xF800) >> 11), MAIN_Version);
            return std::string(buffer);
        }

        uint16_t individualAddress()
        {
            return knx.individualAddress();
        }

        std::string humanIndividualAddress()
        {
            char buffer[14] = {};
            sprintf(buffer, "%i.%i.%i", ((knx.individualAddress() & 0xF000) >> 12), ((knx.individualAddress() & 0x0F00) >> 8), (knx.individualAddress() & 0x00FF));
            return std::string(buffer);
        }

        void serialNumber(uint32_t serialNumber)
        {
            _serialNumber = serialNumber;
        }

        uint32_t serialNumber()
        {
            return _serialNumber;
        }

        std::string humanSerialNumber()
        {
            char buffer[14] = {};
            sprintf(buffer, "00FA:%08lX", _serialNumber);
            return std::string(buffer);
        }
    };

} // namespace OpenKNX