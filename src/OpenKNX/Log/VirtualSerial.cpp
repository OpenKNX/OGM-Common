#include "OpenKNX/Log/VirtualSerial.h"
#include "OpenKNX/Facade.h"
namespace OpenKNX
{
    namespace Log
    {
        VirtualSerial::VirtualSerial(const char* prefix, uint16_t reserveSize)
        {
            _prefix = prefix;
            // Prevent fragmentation
            _buffer.reserve(reserveSize);
        }
        int VirtualSerial::available()
        {
            return -1;
        };
        int VirtualSerial::read()
        {
            return -1;
        };
        int VirtualSerial::peek()
        {
            return 0;
        };
        size_t VirtualSerial::write(uint8_t byte)
        {
            if (byte == '\r') // skip \r
            {
                return 1;
            }
            else if (byte == '\n') // print the completed line
            {
                openknx.logger.log(_prefix, _buffer);
                _buffer.erase();
            }
            else
            {
                _buffer.append(1, static_cast<char>(byte));
            }

            return 1;
        };
    } // namespace Log
} // namespace OpenKNX