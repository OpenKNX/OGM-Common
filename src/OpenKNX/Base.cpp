#include "OpenKNX/Base.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    const std::string Base::logPrefix()
    {
        return name();
    }

    void Base::init() {}

    void Base::setup(bool configured)
    {
        if (configured) setup();
    }

    void Base::setup() {}

    void Base::loop(bool configured)
    {
        if (configured) loop();
    }

    void Base::loop() {}

#ifdef OPENKNX_DUALCORE
    void Base::setup1(bool configured)
    {
        if (configured) setup1();
    }

    void Base::setup1() {}

    void Base::loop1(bool configured)
    {
        if (configured) loop1();
    }

    void Base::loop1() {}

#endif

#if (MASK_VERSION & 0x0900) != 0x0900 // Coupler do not have GroupObjects
    void Base::processInputKo(GroupObject &ko)
    {
    }
#endif

    bool Base::processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
    {
        return false;
    }

    bool Base::processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
    {
        return false;
    }
} // namespace OpenKNX
