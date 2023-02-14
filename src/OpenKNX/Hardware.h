#pragma once
#include "HardwareDevices.h"
#include "knx.h"

// NCN5130: internal commands
#define U_RESET_REQ 0x01
#define U_STATE_REQ 0x02
#define U_SET_BUSY_REQ 0x03
#define U_QUIT_BUSY_REQ 0x04
#define U_BUSMON_REQ 0x05
#define U_SET_ADDRESS_REQ 0xF1 // different on TP-UART
#define U_SET_REPETITION_REQ 0xF2
#define U_L_DATA_OFFSET_REQ 0x08 //-0x0C
#define U_SYSTEM_STATE 0x0D
#define U_STOP_MODE_REQ 0x0E
#define U_EXIT_STOP_MODE_REQ 0x0F
#define U_ACK_REQ 0x10 //-0x17
#define U_CONFIGURE_REQ 0x18
#define U_INT_REG_WR_REQ 0x28 //-0x2B
#define U_INT_REG_RD_REQ 0x38 //-0x3B
#define U_POLLING_STATE_REQ 0xE0

// NCN5130: control services
#define U_RESET_IND 0x03
#define U_STATE_IND 0x07
#define SLAVE_COLLISION 0x80
#define RECEIVE_ERROR 0x40
#define TRANSMIT_ERROR 0x20
#define PROTOCOL_ERROR 0x10
#define TEMPERATURE_WARNING 0x08
#define U_FRAME_STATE_IND 0x13
#define U_FRAME_STATE_MASK 0x17
#define PARITY_BIT_ERROR 0x80
#define CHECKSUM_LENGTH_ERROR 0x40
#define TIMING_ERROR 0x20
#define U_CONFIGURE_IND 0x01
#define U_CONFIGURE_MASK 0x83
#define AUTO_ACKNOWLEDGE 0x20
#define AUTO_POLLING 0x10
#define CRC_CCITT 0x80
#define FRAME_END_WITH_MARKER 0x40
#define U_FRAME_END_IND 0xCB
#define U_STOP_MODE_IND 0x2B
#define U_SYSTEM_STAT_IND 0x4B

// NCN5130 write internal registers
#define U_INT_REG_WR_REQ_WD 0x28
#define U_INT_REG_WR_REQ_ACR0 0x29
#define U_INT_REG_WR_REQ_ACR1 0x2A
#define U_INT_REG_WR_REQ_ASR0 0x2B

// NCN5130 read internal registers
#define U_INT_REG_RD_REQ_WD 0x38
#define U_INT_REG_RD_REQ_ACR0 0x39
#define U_INT_REG_RD_REQ_ACR1 0x3A
#define U_INT_REG_RD_REQ_ASR0 0x3B

// NCN5130: Analog Control Register 0 - Bit values
#define ACR0_FLAG_V20VEN 0x40
#define ACR0_FLAG_DC2EN 0x20
#define ACR0_FLAG_XCLKEN 0x10
#define ACR0_FLAG_TRIGEN 0x08
#define ACR0_FLAG_V20VCLIMIT 0x04

namespace OpenKNX
{
    class Hardware
    {
      private:
        uint8_t feaatures = 0;

      public:
        // Send Command to BCU
        void sendCommandToBcu(const uint8_t* command, const uint8_t length, const char* debug);
        // Receive Response from BCU
        void receiveResponseFromBcu(uint8_t* response, const uint8_t length, const uint16_t wait = 100);
        // Validate Response
        bool validateResponse(const uint8_t* expected, const uint8_t* response, uint8_t length);
        // Turn off 5V rail from NCN5130 to save power for EEPROM write during knx save operation
        void deactivatePowerRail();
        // Turn on 5V rail from NCN5130 in case SAVE-Interrupt was false positive
        void activatePowerRail();
        // Stop KNX Communication
        void stopKnxMode();
        // Start KNX Communication
        void startKnxMode();
    };
} // namespace OpenKNX