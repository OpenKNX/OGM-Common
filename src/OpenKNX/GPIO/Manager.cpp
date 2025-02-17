#include "GPIO.h"
#include "OpenKNX.h"
#include "DriverEmbedded.h"
#include "DriverTCA9555.h"
#include "DriverTCA6408.h"

namespace OpenKNX
{
    namespace GPIO
    {
        #ifdef OPENKNX_GPIO_NUM
        const OPENKNX_GPIO_T GPIO_TYPES[OPENKNX_GPIO_NUM+1] = {OPENKNX_GPIO_T_EMBEDDED, OPENKNX_GPIO_TYPES};
        const uint16_t GPIO_ADDRS[OPENKNX_GPIO_NUM+1] = {0, OPENKNX_GPIO_ADDRS};
        const uint8_t GPIO_INTS[OPENKNX_GPIO_NUM+1] = {0, OPENKNX_GPIO_INTS};
        #else
        #define OPENKNX_GPIO_NUM 0
        const OPENKNX_GPIO_T GPIO_TYPES[1] = {OPENKNX_GPIO_T_EMBEDDED};
        #endif
        Base* GPIOExpanders[OPENKNX_GPIO_NUM+1];

        Manager::Manager()
        {
        }

        Manager::~Manager()
        {
        }

        void Manager::init()
        {
            #if OPENKNX_GPIO_NUM > 0
            OPENKNX_GPIO_WIRE.setSDA(OPENKNX_GPIO_SDA);
            OPENKNX_GPIO_WIRE.setSCL(OPENKNX_GPIO_SCL);
            OPENKNX_GPIO_WIRE.begin();
            OPENKNX_GPIO_WIRE.setClock(OPENKNX_GPIO_CLOCK);
            #endif

            for(int i = 0; i < OPENKNX_GPIO_NUM+1; i++)
            {
                switch(GPIO_TYPES[i])
                {
                    case OPENKNX_GPIO_T_EMBEDDED:
                    {
                        GPIOExpanders[i] = new DriverEmbedded();
                    }
                    break;
                    #if OPENKNX_GPIO_NUM > 0
                    case OPENKNX_GPIO_T_TCA9555:
                    {
                        GPIOExpanders[i] = new DriverTCA9555(GPIO_ADDRS[i], &OPENKNX_GPIO_WIRE);
                        int statuscode = GPIOExpanders[i]->init();
                        if(statuscode)
                        {
                            logErrorP("no connection to GPIO Expander %u with address %u (Errorcode: %u)", i, GPIO_ADDRS[i], statuscode);
                        }
                        else
                        {
                            logInfoP("connected to GPIO Expander %u with address %u", i, GPIO_ADDRS[i]);
                        }
                    }
                    break;
                    case OPENKNX_GPIO_T_TCA6408:
                    {
                        GPIOExpanders[i] = new DriverTCA6408(GPIO_ADDRS[i], &OPENKNX_GPIO_WIRE);
                        int statuscode = GPIOExpanders[i]->init();
                        if(statuscode)
                        {
                            logErrorP("no connection to GPIO Expander %u with address %u (Errorcode: %u)", i, GPIO_ADDRS[i], statuscode);
                        }
                        else
                        {
                            logInfoP("connected to GPIO Expander %u with address %u", i, GPIO_ADDRS[i]);
                        }
                    }
                    break;
                    #endif
                    default:
                        ;
                        logErrorP("GPIO_TYPE %u not found", GPIO_TYPES[i]);
                }
            }
        }

        void Manager::loop()
        {

        }

        std::string Manager::logPrefix()
        {
            return openknx.logger.buildPrefix("GPIOHAL", 0);
        }

        void Manager::pinMode(uint16_t pin, int mode, bool preset, int status)
        {
            int8_t localpin = pin & 0xff;
            uint8_t expander = pin >> 8;
            if(expander > OPENKNX_GPIO_NUM)
            {
                logErrorP("GPIOModule::pinMode: invalid pin id %u", pin);
                return;
            }
            GPIOExpanders[expander]->GPIOpinMode(localpin, mode, preset, status);
        }

        void Manager::digitalWrite(uint16_t pin, int status)
        {
            int8_t localpin = pin & 0xff;
            uint8_t expander = pin >> 8;
            if(expander > OPENKNX_GPIO_NUM)
            {
                logErrorP("GPIOModule::digitalWrite: invalid pin id %u", pin);
                return;
            }
            GPIOExpanders[expander]->GPIOdigitalWrite(localpin, status);
        }

        bool Manager::digitalRead(uint16_t pin)
        {
            int8_t localpin = pin & 0xff;
            uint8_t expander = pin >> 8;
            if(expander > OPENKNX_GPIO_NUM)
            {
                logErrorP("GPIOModule::digitalRead: invalid pin id %u", pin);
                return 0;
            }
            return GPIOExpanders[expander]->GPIOdigitalRead(localpin);
        }
    }
}