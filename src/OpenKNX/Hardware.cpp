#include "OpenKNX/Common.h"

namespace OpenKNX
{
    void Hardware::sendCommandToBcu(const uint8_t* command, const uint8_t length, const char* debug)
    {
        if (!knx.platform().knxUart())
            return;

        if (debug != nullptr)
            openknx.logger.log(LogLevel::Debug, "Hardware<BCU>", "    Send command %s to BCU", debug);

        // send system state command and interpret answer
        knx.platform().knxUart()->flush();
        knx.platform().knxUart()->write(command, length);
    }

    void Hardware::receiveResponseFromBcu(uint8_t* response, const uint8_t length, const uint16_t wait /* = 100 */)
    {
        uint8_t i = 0;
        uint32_t waitTimer = millis();
        while (!delayCheck(waitTimer, wait))
        {
            if (knx.platform().knxUart()->available())
            {
                response[i] = knx.platform().knxUart()->read();
                i++;
            }
        }
    }

    bool Hardware::validateResponse(const uint8_t* expected, const uint8_t* response, const uint8_t length)
    {
        for (uint8_t i = 0; i < length; i++)
        {
            if (expected[0] != response[0])
            {
                openknx.logger.log(LogLevel::Error, "Hardware<BCU>", "FAILED - received unexpected response: ");
                openknx.logHex("Hardware<BCU>", response, length);
                return false;
            }
        }

        return true;
    }

    void Hardware::deactivatePowerRail()
    {
        openknx.log("Hardware<BCU>", "  Switching off 5V / 20V rail of BCU");
        uint8_t command[] = {U_INT_REG_WR_REQ_ACR0, ACR0_FLAG_XCLKEN | ACR0_FLAG_V20VCLIMIT};
        sendCommandToBcu(command, 2, "U_INT_REG_WR_REQ_ACR0");
        // sendCommandToBCU("READ_ACR0 (Analog control register 0)", U_INT_REG_RD_REQ_ACR0, ACR0_FLAG_XCLKEN | ACR0_FLAG_V20VCLIMIT);
    }

    void Hardware::activatePowerRail()
    {
        openknx.log("Hardware<BCU>", "  Switching on 5V rail of BCU");
        uint8_t command[] = {U_INT_REG_WR_REQ_ACR0, ACR0_FLAG_DC2EN | ACR0_FLAG_V20VEN | ACR0_FLAG_XCLKEN | ACR0_FLAG_V20VCLIMIT};
        sendCommandToBcu(command, 2, "U_INT_REG_WR_REQ_ACR0");
    }

    void Hardware::stopKnxMode()
    {
        openknx.log("Hardware<BCU>", "  Stop KNX Mode...");
        uint8_t command[] = {U_STOP_MODE_REQ};
        sendCommandToBcu(command, 1, "STOP_MODE");

        uint8_t response[2] = {};
        receiveResponseFromBcu(response, 2); // Receive 2 bytes

        uint8_t expected[] = {U_STOP_MODE_IND};
        validateResponse(expected, response, 1); // Check first Byte
    }

    void Hardware::startKnxMode()
    {
        openknx.log("Hardware<BCU>", "  Start KNX Mode...");
        uint8_t command[] = {U_EXIT_STOP_MODE_REQ};
        sendCommandToBcu(command, 1, "EXIT_STOP_MODE"); // U_RESET_IND

        uint8_t response[1] = {};
        receiveResponseFromBcu(response, 1); // Receive 1 byte

        uint8_t expected[] = {U_RESET_IND};
        validateResponse(expected, response, 1); // Check first Byte
    }

} // namespace OpenKNX