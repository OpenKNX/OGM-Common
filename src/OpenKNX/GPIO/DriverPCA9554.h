#pragma once
#include "GPIO.h"
#include "Libs/PCA95xx.h"

namespace OpenKNX
{
    namespace GPIO
    {
        class DriverPCA9554 : public Base
        {
          private:
            PCA95XX* _pca = nullptr;

          public:
            DriverPCA9554(uint8_t i2cAddr, TwoWire* wire);
            virtual ~DriverPCA9554();
            virtual int init() override;

            virtual void GPIOpinMode(uint8_t pin, int mode, bool preset, int status) override;
            virtual void GPIOdigitalWrite(uint8_t pin, int status) override;
            virtual bool GPIOdigitalRead(uint8_t pin) override;

            virtual inline const bool isInitialized() { return _initState == GPIOInitState::OK; }
            virtual inline void setInitState(const GPIOInitState state) { _initState = state; }
            virtual inline GPIOInitState getInitState() { return _initState; }

          private:
            GPIOInitState _initState = GPIOInitState::Uninitialized;
        };
    } // namespace GPIO
} // namespace OpenKNX