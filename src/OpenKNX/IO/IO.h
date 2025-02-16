#pragma once
#include "OpenKNX.h"
#include "hardware.h"



#ifndef OPENKNX_GPIO_CLOCK
#define OPENKNX_GPIO_CLOCK 50000
#endif

namespace OpenKNX
{
    namespace IO
    {
        class iGPIOExpander
        {
        public:
            virtual int init() = 0;
            virtual void GPIOpinMode(uint8_t pin, int mode, bool preset, int status) = 0;
            virtual void GPIOdigitalWrite(uint8_t pin, int status) = 0;
            virtual bool GPIOdigitalRead(uint8_t pin) = 0;
        };



        enum OPENKNX_GPIO_T
        {
        OPENKNX_GPIO_T_MCU = 0,
        OPENKNX_GPIO_T_TCA9555 = 1,
        OPENKNX_GPIO_T_TCA6408 = 2
        };


        class GPIOHAL
        {
        public:
            GPIOHAL();
            ~GPIOHAL();

            void init();
            void loop();

            std::string logPrefix();

            void pinMode(uint16_t pin, int mode, bool preset=false, int status=0);
            void digitalWrite(uint16_t pin, int status);
            bool digitalRead(uint16_t pin);

        private:

        };
    }
}