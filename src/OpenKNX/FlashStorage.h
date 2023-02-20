#pragma once
#include "knx.h"
#include "knx/bits.h"
#include <string>
// #include <stddef.h>

#ifndef FLASH_DATA_WRITE_LIMIT
#define FLASH_DATA_WRITE_LIMIT 180000 // 3 Minutes delay
#endif

#define FLASH_DATA_FILLBYTE 0xFF



/*
 * The data-structure is optimized for fast sequential writing, to maximize 
 * the chance of writing completely after detection of power loss.
 * Partial writing should be detectable.
 * Aligned to end of (usable) flash, as we do want to maximize otherwhise 
 * usable space and NOT use a fixed starting position.
 * 
 * Definition of Data-Structure (Format v1):
 * - Numeric values are given in big-endian byte-order
 * - Values are defined as unsigned integers of given size
 *   (as not explicitly defined otherwhise)
 *
 * Global Structure, Sizing and Layout: 
 * > ............................................ available flash storage --->| the_end
 * >                      |<- DATA[?] ->|<------------- META[12] ------------>| 
 * >                      |<------- CHECKSUM_INPUT ------->| 
 * > FLASH_STORAGE_DATA :=  DATA[$SIZE] ; APP[4] ; SIZE[2] ; CHK[2] ; INIT[4] |
 * Note: Size is defined in [bytes]
 * 
 * Overview:
 * - DATA uint8_t[]: dynamic-sized for the module-data (size is defined in SIZE)
 * - META: fixed-sized to ensure robust restore of module-data
 *   - APP  uint8_t[4]: device/firmware info
 *   - SIZE uint16_t  : size (and indirect position) definition for DATA
 *   - CHK  uint8_t[2]: checksum
 *   - INIT uint8_t[4]: the magic word for format detection
 * 
 * A more detailed description is following below on defines related to fields.
 */


//
// ==== FLASH_STORAGE_DATA - META ====
//

/**
> APP := FW_NUMBER[2] ; FW_VERSION[2]
> FW_NUMBER := MAIN_OpenKnxId ; MAIN_ApplicationNumber

FW_NUMBER contains MAIN_OpenKnxId and the ETS Firmware "Application Version"
FWE_VERSION contains the number shown in ETS as "[revision] major.minor"
*/
#define FLASH_DATA_APP_LEN 4


/**
uint16_t SIZE define the length and position of DATA:
&DATA := &META - SIZE

2 bytes for length is sufficient, as >64K needs to much time to write.
*/
#define FLASH_DATA_SIZE_LEN 2


/**
uint16_t CHK contains a checksum over all bytes of CHECKSUM_INPUT

The checksum should detect partial writes, or partes of DATA overwritten.
*/
#define FLASH_DATA_CHK_LEN 2





/** 
INIT contains the Magic-Word including a format version number.
&INIT = (_startAddress + _flashSize) - FLASH_DATA_INIT_LEN
The value is fixed for current implementation.
>        |MAGIC_BYTES|  VERSION  |
> INIT :=  'O' ; 'K' ; 'V' ; 0x01
Which is expected as bytes-sequence: 4F 4B 56 01

DATA *must* *not* be processed without an exact match of INIT!
Othere values of INIT indicates:
a) no data written before
b) data written with an incompatible structure
   Note for the Future:
       There is the idea to increase version number in last byte in this case, 
       but there is no guaranteed or definition yet.
*/
#define FLASH_DATA_INIT 1330337281 

/**
Intro for Identification (and possible versioning later).
4 byte fix length
*/
#define FLASH_DATA_INIT_LEN 4



/** Overall fixed-size of the non-module-data part */
#define FLASH_DATA_META_LEN (FLASH_DATA_APP_LEN + FLASH_DATA_SIZE_LEN + FLASH_DATA_CHK_LEN + FLASH_DATA_INIT_LEN)



//
// ==== FLASH_STORAGE_DATA - DATA ====
//

/*
 * Structure of module data:
 * > ............................................ available flash storage --->| the_end
 * >                      |<------------- DATA[SIZE] ------------->|<- META ->| 
 * >                      |<-- MODULE -->| ........ |<-- MODULE -->|
 * > DATA := MODULE+
 * Note: Number of modules is not (directly) limited by this structure,
 *       but indirectly by datatype of SIZE and MOD_META
 * 
 * Structure of single module data:
 * > MODULE	  := MOD_META[3] ; MOD_DATA[MOD_SIZE]
 * > MOD_META := MOD_ID[1] ; MOD_SIZE[2]
 * 
 * MOD_ID	:= uint8_t
 *   Identification of ModuleType as in knxprod.
 *   Allows different ordering of modules in storage.
 * 
 * MOD_SIZE	:= uint16_t
 *   Define the length of MOD_DATA and position of following MODULE-block:
 *   &(MODULE[i+1]) := &(MODULE[i]) + sizeof(MOD_META) + MOD_SIZE
 * 
 * MOD_DATA	:= uint8_t[$MOD_SIZE}
 *   Content ist defined by the module (referenced in MOD_ID) only.
 *   Module must not write >MOD_SIZE bytes
 *   Module may write <MOD_SIZE bytes,
 *     unused bytes will be filled to ensure defined DATA and resulting CHECKSUM
 * 
 * Assertion:
 *   &MODULE[last] + sizeof(MOD_META) + MODULE[last].MOD_SIZE == &META
 * DATA and MODULE entries must not overlap with HEAD
 * Unused space after last MODULE would result in trying to process a non-existing module
 * and should result in wrong CHECKSUM.
 */

#define FLASH_DATA_MODULE_ID_LEN 1

// TODO check using #define FLASH_DATA_MODULE_SIZE_LEN FLASH_DATA_SIZE_LEN






namespace OpenKNX
{
    /**
     * OpenKnx FlashStorage-Handling for (Runtime) Data owned by Modules
     * 
     * Introduced with OpenKnx-Commons v2 to allow flexible extension.
     * 
     * Scope/Exclusion: ETS-Parametrization is NOT part of this data. Only e.g. values of Logic-Module, 
     *                  or calibration-data of sensors which should be saved on power loss.
     */
    class FlashStorage
    {
      public:
        /**
         * TODO extend documentation
         * 
         * Steps for reading:
         * 1) check for expected INIT
         * 2) check APP for matching version
         * 3) read the size of data
         * 4) validate checksum
         * 5) read data (for all modules)
         */
        void load();

        /**
         * TODO extend documentation
         * 
         * Steps for writing:
         * 1) get required data-size for all modules
         * 2) calculate overall data-size and start-position
         * 3) write DATA: data and fill unused requested space (for all modules)
         * 4) write APP
         * 5) write SIZE
         * 6) write CHK
         * 7) write INIT
         */
        void save(bool force = false);

        void write(uint8_t *buffer, uint16_t size = 1);
        void write(uint8_t value, uint16_t size);
        void writeByte(uint8_t value);
        void writeWord(uint16_t value);
        void writeInt(uint32_t value);
        uint8_t *read(uint16_t size = 1);
        uint8_t readByte();
        uint16_t readWord();
        uint32_t readInt();
        uint16_t firmwareVersion();

      private:
        bool *loadedModules;
        uint8_t *_flashStart;
        uint16_t _flashSize = 0;
        uint32_t _lastWrite = 0;
        uint16_t _lastFirmwareNumber = 0;
        uint16_t _lastFirmwareVersion = 0;
        uint16_t _checksum = 0;
        uint32_t _currentWriteAddress = 0;
        uint8_t *_currentReadAddress = 0;
        uint32_t _maxWriteAddress = 0;
        void writeFilldata();
        void readData();
        void initUnloadedModules();
        uint16_t calcChecksum(uint8_t *data, uint16_t size);
        bool verifyChecksum(uint8_t *data, uint16_t size);
        std::string logPrefix();
    };
} // namespace OpenKNX