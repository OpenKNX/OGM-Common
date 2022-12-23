#pragma once
#include <stdint.h>

/**
 * Interface for classes that can save and restore data to/from Flash memory. 
 */
class IFlashUserData
{
  public:
    /**
     * This method is called when the object should save its state to the buffer.
     *  
     * @param buffer The buffer the object should save its state to.
     * 
     * @return The buffer plus the size of the object state. The next object will use this value as 
     * the start of its buffer.
     */
    virtual uint8_t* save(uint8_t* buffer)
    {
        return buffer;
    }
    
    /**
     * This method is called when the object should restore its state from the buffer.
     *  
     * @param buffer The buffer the object should restore its state from.
     * 
     * @return The buffer plus the size of the object state. The next object will use this value as 
     * the start of its buffer.
     */
    virtual const uint8_t* restore(const uint8_t* buffer)
    {
        return buffer;
    }
    
    /**
     * This method is used to calculate maximum needed buffer size for SAVE processing
     * 
     * @return The maximum number of bytes the object needs to save its state. 
     */
    virtual uint16_t saveSize()
    {
        return 0;
    }


    /**
     * This method is called to fetch the next IFlashUserData class, which wants to persist data
     * The default implementation does in most cases the right thing (the next class is usually known).
     * You should override this only if you want to influence the SAVE execution somehow.
     * 
     * @return The next IFlashUserData class which wants to persist data. 
     */
    virtual IFlashUserData* next()
    {
        return _next;
    }

    /**
     * This method sets the next IFlashUserData class in the chain of objects to persist data
     * If you override next()-getter, you should also override next(*obj)-setter to ensure, that none
     * of the objects in chain is missed.
     * 
     * @param The next IFlashUserData object to be called after this one
    */
    virtual void next(IFlashUserData *obj)
    {
        _next = obj;
    }

    /**
     * This method is called if save/restore will be executed in context of a SAVE-Interrupt (power failure on KNX-Bus)
     * The method should be overridden if there is any hardware to be switched off to save power (i.e. custom LED's or sensors)
     * Everything what happens here should happen FAST, it makes no sense to spend more processing time on switching off that 
     * the device would take to shorten the available save time.
     * The 5V power supply from NCN5120/5130 will be turned off as very first action during SAVE-processing. You need not care about this.
    */
    virtual void powerOff()
    {
        // do nothing as default;
    }

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
    virtual bool powerOn()
    {
        return false; //default: mark as not handled, will lead to a reboot.
    }

    /**
     * @return optional and just for debugging: name of the user data class to be listed if it is stored/restored 
     */
    virtual const char* name()
    {
        return "";
    }

  private:
    IFlashUserData* _next = 0;
};