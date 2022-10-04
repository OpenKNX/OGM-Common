#pragma once

#include <cstdint>
#include <stdio.h>
#include <stdarg.h>
#include <Arduino.h>
/*********************************************
 * Manage a part of EEPROM for persisted data
 * 
 * Create a class providing statring point and
 * size information. Different instances will
 * also prevent overlap
 * *******************************************/
// During EEPROM Write we have to delay 5 ms
#define EEPROM_WRITE_DELAY 5

class EepromManager
{
  private:
    static uint8_t mFiller[];
    bool mIsTransmission = false;
    bool mValidityChecked = false;
    bool mIsValidEEPROM = false;
    uint16_t mStartPage = 0;
    uint16_t mNumPages = 0;
    uint8_t* mMagicWord = 0;

    bool writeSession(bool iBegin);
    bool checkDataValid();

  public:
    EepromManager(uint16_t iStartPage, uint16_t iNumPages, uint8_t *iMagicWord);
    ~EepromManager();

    bool beginWriteSession();
    void endWriteSession();
    void beginPage(uint16_t iAddress);
    bool endPage();
    void write4Bytes(uint8_t *iData, uint8_t iLen);
    void prepareRead(uint16_t iAddress, uint8_t iLen);
    bool checkMagicWord(uint16_t iAddress);
    bool isValid();
};


