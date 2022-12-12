#include "FlashUserData.h"

#include <string.h>
#include "knx.h"
// #include "knx/bits.h"

FlashUserData::FlashUserData()
{}

FlashUserData::~FlashUserData()
{}

void FlashUserData::readFlash()
{
    println("read UserData from flash...");

    uint8_t* flashStart = knx.platform().getNonVolatileMemoryStart();
    size_t flashSize = knx.platform().getNonVolatileMemorySize();
    size_t userdataSize = 0;
#ifdef USERDATA_SAVE_SIZE
    userdataSize = USERDATA_SAVE_SIZE;
#endif
    if (flashStart == nullptr || userdataSize == 0)
    {
        println("no flash for UserData available;");
        return;
    }
    flashStart += flashSize;
    const uint8_t* buffer = flashStart;
    // printHex("RESTORED ", flashStart, _metadataSize);

    // uint16_t metadataBlockSize = alignToPageSize(_metadataSize);

    // _freeList = new MemoryBlock(flashStart + metadataBlockSize, flashSize - metadataBlockSize);

    // uint16_t apiVersion = 0;
    // buffer = popWord(apiVersion, flashStart);

    // uint16_t manufacturerId = 0;
    // buffer = popWord(manufacturerId, buffer);

    // uint16_t version = 0;
    // buffer = popWord(version, buffer);

    IFlashUserData* next = _flashUserDataClass;
    while (next)
    {
        buffer = next->restore(buffer);
        print(next->name());
        print(" (");
        print(buffer -flashStart);
        println(" bytes)");
        next = next->next();
    }
    println("restored UserData");
}

void FlashUserData::writeFlash()
{
    // first get the necessary size of the writeBuffer
    uint16_t writeBufferSize = _metadataSize;
    IFlashUserData* next = _flashUserDataClass;
    while (next)
    {
        writeBufferSize = MAX(writeBufferSize, next->saveSize());
        next = next->next();
    }
    uint8_t buffer[writeBufferSize];
    // uint8_t* flashStart = knx.platform().getNonVolatileMemoryStart();
    size_t flashSize = knx.platform().getNonVolatileMemorySize();
    uint32_t flashPos = flashSize; //relative address
    uint8_t* bufferPos = buffer;

    if (_metadataSize > 0) 
    {
        // currently no metadata necessary, this is just an example how we would write them
        // bufferPos = pushWord(_deviceObject.apiVersion, bufferPos);
        // bufferPos = pushWord(_deviceObject.manufacturerId(), bufferPos);
        // bufferPos = pushByteArray(_deviceObject.hardwareType(), LEN_HARDWARE_TYPE, bufferPos);
        // bufferPos = pushWord(_deviceObject.version(), bufferPos);

        flashPos = writeFlash(flashPos, bufferPos - buffer, buffer);
    }
    println("saving FlashUserData... ");
    next = _flashUserDataClass;
    while (next)
    {
        bufferPos = next->save(buffer);
        print(next->name());
        print(" (size req: ");
        print(next->saveSize());
        print(", act: ");
        print(bufferPos - buffer);
        println(")");
        flashPos = writeFlash(flashPos, bufferPos - buffer, buffer);
        next = next->next();
    }
    saveFlash();
    println("UserData written to flash");
}

void FlashUserData::saveFlash()
{
    knx.platform().commitNonVolatileMemory();
}

void FlashUserData::addFlashUserDataClass(IFlashUserData* obj)
{
    _flashUserDataClass = obj;
}

uint32_t FlashUserData::writeFlash(uint32_t relativeAddress, size_t size, uint8_t* data)
{
    return knx.platform().writeNonVolatileMemory(relativeAddress, data, size);
}

// uint16_t FlashUserData::alignToPageSize(size_t size)
// {
//     size_t pageSize = 4; //_platform.flashPageSize(); // align to 32bit for now, as aligning to flash-page-size causes side effects in programming
//     // pagesize should be a multiply of two
//     return (size + pageSize - 1) & (-1*pageSize);
// }

