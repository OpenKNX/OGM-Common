#ifdef ARDUINO_ARCH_SAMD

#include "OpenKNX/Flash/Samd.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    namespace Flash
    {
        Samd::Samd(uint32_t startOffset, uint32_t size, std::string id)
        {
            const uint32_t pageSizes[] = {8, 16, 32, 64, 128, 256, 512, 1024};
            _id = id;
            _startOffset = startOffset;
            _size = size;
            _sectorSize = pageSizes[NVMCTRL->PARAM.bit.PSZ] * 4;
            _maxSize = pageSizes[NVMCTRL->PARAM.bit.PSZ] * NVMCTRL->PARAM.bit.NVMP;

            validateParameters();
        }

        uint8_t *Samd::flash()
        {
            return (uint8_t *)_startOffset;
        }

        void Samd::eraseSector(uint16_t sector)
        {
            if (!needEraseSector(sector))
            {
                logTraceP("skip erase sector, because already erased");
                return;
            }

            logTraceP("erase sector %i", sector);
            NVMCTRL->ADDR.reg = ((uint32_t)_startOffset + (sector * _sectorSize)) / 2;
            NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
            while (!NVMCTRL->INTFLAG.bit.READY)
            {}
        }

        void Samd::writeSector()
        {
            if (!needWriteSector())
            {
                logTraceP("skip write sector, because no changes");
                return;
            }

            if (needEraseForBuffer())
                eraseSector(_bufferSector);

            logTraceP("write sector %i", _bufferSector);
            //logHexTraceP(_buffer, _sectorSize);
            volatile uint32_t *src_addr = (volatile uint32_t *)_buffer;
            volatile uint32_t *dst_addr = (volatile uint32_t *)(flash() + (_bufferSector * _sectorSize));

            // Disable automatic page write
            NVMCTRL->CTRLB.bit.MANW = 1;

            uint16_t size = _sectorSize / 4;

            // Do writes in pages
            while (size)
            {
                // Execute "PBC" Page Buffer Clear
                NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
                while (NVMCTRL->INTFLAG.bit.READY == 0)
                {}

                // Fill page buffer
                for (uint16_t i = 0; i < (_sectorSize / 16) && size; i++)
                {
                    *dst_addr = *src_addr;
                    src_addr++;
                    dst_addr++;
                    size--;
                }

                // Execute "WP" Write Page
                NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
                while (NVMCTRL->INTFLAG.bit.READY == 0)
                {}
            }
        }
    } // namespace Flash
} // namespace OpenKNX
#endif