#pragma once
#include "OpenKNX/Base.h"

namespace OpenKNX
{
    /*
     * Abstract class for Modules
     */
    class Module : public Base
    {
      public:
        /*
         * The version of module.
         *
         * Hint:
         * By returning an empty string, the module is hidden in the version output on the console.
         * In the case of modules within OAM, this is practical since the firmware version is already output.
         *
         * @return version
         */
        virtual const std::string version() = 0;

        /*
         * This method must returned the size for reservation space in flash storage.
         * @return size in bytes
         */
        virtual uint16_t flashSize();

        /*
         * Called when the module should write this data to flash
         * It must use the write helper of FlashStorage (openknx.flash.writeXXX).
         * Only the size of flashSize() is allowed to write. missing bytes will auto filled.
         */
        virtual void writeFlash();

        /*
         * Called after setup to load data from flash storage.
         * @param data pointer to data of module in flash, but the better way is to use read helper of FlashStorage (openknx.flash.readXXX)
         * @param size number of saved bytes in flash. if no data is saved, the size is 0 (e.g. for init)
         */
        virtual void readFlash(const uint8_t *data, const uint16_t size);

        /*
         * Called after the startup delay time are expired.
         */
        virtual void processAfterStartupDelay();

        /*
         * Called before the device will restart.
         * This happen when you reset device by knx or the after ETS has parameterized the application
         */
        virtual void processBeforeRestart();

        /*
         * Called before unload the group object tables of knx.
         * This happen when ETS will start parameterization
         */
        virtual void processBeforeTablesUnload();

        /*
         * Called if knx receives an function property command and no Property is declared in stack.
         * Can be invoked by script in ETS
         */
        virtual bool processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
        /*
         * Called if knx receives an function property state and no Property is declared in stack.
         * Can be invoked by script in ETS
         */
        virtual bool processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength);

        /**
         * This method is called if save/restore will be executed in context of a SAVE-Interrupt (power failure on KNX-Bus)
         * The method should be overridden if there is any hardware to be switched off to save power (i.e. custom LED's or sensors)
         * Everything what happens here should happen FAST, it makes no sense to spend more processing time on switching off that
         * the device would take to shorten the available save time.
         * The 5V power supply from NCN5120/5130 will be turned off as very first action during SAVE-processing. You need not care about this.
         */
        virtual void savePower();

        /**
         * This method is called if save/restore was executed in context of a SAVE-Interrupt (power failure on KNX-Bus) and all
         * data of all modules was successfully saved to flash.
         * This is for the (seldom) case that the SAVE-Interrupt was triggered due to a very short power break (< 100 ms), so that
         * there was no power loss for the processor and the rest of the hardware.
         * In this case you (if there was any action during powerOff()) you can revert this action and resume power for your devices.
         * At this point, the 5V supply from NCN5120/5130 was restored already, you might need there to initialize your hardware again.
         *
         * @return true, if you handled powerOn accordingly, false otherwise.
         * If any registered module returns false, there will be a reboot of the device to ensure full functionality.
         */
        virtual bool restorePower();

        /**
         * This method is called when a command is entered in the console or the diagnoseKo.
         * The first argument is the command, and the second argument indicates whether the call was made via diagnoseKo or the console.
         *
         * If a module feels responsible for this command, it returns true, and the processing will terminated.
         *
         * If a command is entered that requires an output, the module itself is responsible for handling it.
         * It must then determine based on the arguments whether to display the output on the console or send a message via diagnoseKo.
         */
        virtual bool processCommand(const std::string cmd, bool diagnoseKo);

        /**
         * This method prints out information over the command it can handle
         */
        virtual void showHelp();

        /**
         * This method prints out relevant informations of the module
         */
        virtual void showInformations();
    };
} // namespace OpenKNX