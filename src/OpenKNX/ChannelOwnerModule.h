#pragma once
#include "OpenKNX/Module.h"
#include "OpenKNX/Channel.h"

namespace OpenKNX
{
    /*
     * Base class for modules with channels
     */
    class ChannelOwnerModule : public OpenKNX::Module
    {
      private:
        uint8_t _numberOfChannels;
        uint8_t _currentChannel = 0;
        OpenKNX::Channel** _pChannels = nullptr;

      public:
        /*
         * Constructor. 
         * Provide the number of the channels defined in the knxprod.h in the numberOfChannels parameter.
         */
        ChannelOwnerModule(uint8_t numberOfChannels = 0);
        ~ChannelOwnerModule();

        /* 
         * Factory function for the channels.
         * Override this function to create the channel objects.
         * Returning nullptr is allowed to skip creation of a channel.
         */
        virtual OpenKNX::Channel* createChannel(uint8_t _channelIndex /* this parameter is used in macros, do not rename */);

        /*
         * This implementation calls the factory function createChannel for all channels
         * If you override this function, don't forget to call this implementation to get the channels created.
         */
        virtual void setup() override;

        /*
         * This implementation calls the loop of all channels. 
         * If you override this function, don't forget to call this implemenation to get the loop function called in the channels.
         */
        virtual void loop(bool configured) override;
        
        /*
         * This implementation calls the loop of all channels. 
         * If you override this function, don't forget to call this implemenation to get the loop function called in the channels.
         */
        virtual void loop() override;

        /*
         * Returns the number of created channels.
         */
        uint8_t getNumberOfUsedChannels();
        /*
         * Returns the number of all channels provided in the construtor.
         */
        uint8_t getNumberOfChannels();

        /*
         * Returns the channel object if created, otherwise a nullptr
         */
        OpenKNX::Channel* getChannel(uint8_t channelIndex);

#ifdef OPENKNX_DUALCORE
        /*
         * This implementation calls the setup1 of all channels. 
         * If you override this function, don't forget to call this implemenation to get the setup1 function called in the channels.
         */
        virtual void setup1(bool configured) override;
 
        /*
         * This implementation calls the setup1 of all channels. 
         * If you override this function, don't forget to call this implemenation to get the setup1 function called in the channels.
         */
        virtual void setup1() override;

        /*
         * This implementation calls the loop of all channels. 
         * If you override this function, don't forget to call this implemenation to get the loop function called in the channels.
         */
        virtual void loop1(bool configured) override;

        /*
         * This implementation calls the loop of all channels. 
         * If you override this function, don't forget to call this implemenation to get the loop function called in the channels.
         */
        virtual void loop1() override;
#endif

#if (MASK_VERSION & 0x0900) != 0x0900 // Coupler do not have GroupObjects
        /*
         * This implementation calls the processInputKo of all channels. 
         * If you override this function, don't forget to call this implemenation to get the processInputKo function called in the channels.
         */
        virtual void processInputKo(GroupObject& ko) override;
#endif
    };
} // namespace OpenKNX