#include "HardwareDevices.h"
#include "EepromManager.h"
#include "OpenKNX.h"
#include <Wire.h>
#ifdef WATCHDOG
#include <Adafruit_SleepyDog.h>
#endif

uint8_t boardHardware = 0;

void ledInfo(bool iOn)
{
#ifdef INFO_LED_PIN
    digitalWrite(INFO_LED_PIN, INFO_LED_PIN_ACTIVE_ON == iOn);
#endif
}

void ledProg(bool iOn)
{
#ifdef PROG_LED_PIN
    digitalWrite(PROG_LED_PIN, PROG_LED_PIN_ACTIVE_ON == iOn);
#endif
}

void deactivatePowerRail()
{
    // turn off known LED's
    ledProg(false);
    ledInfo(false);
    // init knx-uart to be in control mode
    initUart();
    openknx.log("PowerRail", "Stop UART KNX communication...");
    sendUartCommand("STOP_MODE", U_STOP_MODE_REQ, U_STOP_MODE_IND);
    openknx.log("PowerRail", "Switching off 5V / 20V rail...");
    // turn off 5V and 20V rail
    uint8_t lBuffer[] = {U_INT_REG_WR_REQ_ACR0, ACR0_FLAG_XCLKEN | ACR0_FLAG_V20VCLIMIT};
    // get rid of knx reference
    Serial1.write(lBuffer, 2);
    // sendUartCommand("READ_ACR0 (Analog control register 0)", U_INT_REG_RD_REQ_ACR0, ACR0_FLAG_XCLKEN | ACR0_FLAG_V20VCLIMIT);
}

void activatePowerRail()
{
    openknx.log("PowerRail", "Switching on 5V rail...");
    // turn on 5V and 20V rail
    uint8_t lBuffer[] = {U_INT_REG_WR_REQ_ACR0, ACR0_FLAG_DC2EN | ACR0_FLAG_V20VEN | ACR0_FLAG_XCLKEN | ACR0_FLAG_V20VCLIMIT};
    initUart();
    Serial1.write(lBuffer, 2);
    // give all sensors some time to init
    delay(100);
    openknx.log("PowerRail", "Start UART KNX communication...");
    sendUartCommand("EXIT_STOP_MODE", U_EXIT_STOP_MODE_REQ, U_RESET_IND);
}

void fatalError(uint8_t iErrorCode, const char* iErrorText)
{
    deactivatePowerRail(); // Disable PowerRail

    const uint16_t lDelay = 200;
    // #ifdef WATCHDOG
    //     Watchdog.disable();
    // #endif
    for (;;)
    {
        // we repeat the message on serial bus, so we can get it even
        // if we connect USB later
        openknx.log("fatalError", "%d: %s", iErrorCode, iErrorText);
        ledInfo(true);
        delay(lDelay);
        // number of red blinks during a yellow blink is the error code
        for (uint8_t i = 0; i < iErrorCode; i++)
        {
            ledProg(true);
            delay(lDelay);
            ledProg(false);
            delay(lDelay);
#ifdef WATCHDOG
        Watchdog.reset();
#endif
        }
        ledInfo(false);
        delay(lDelay * 5);
    }
}

// call this BEFORE Wire.begin()
// it clears I2C Bus, calls Wire.begin() and checks which board hardware is available
bool boardCheck()
{
    bool lResult = checkUartExistence();

#ifndef NO_I2C
    // first we clear I2C-Bus
    Wire.end(); // in case, Wire.begin() was called before
    uint8_t lI2c = 0;
    // lI2c = clearI2cBus(); // clear the I2C bus first before calling Wire.begin()
    if (lI2c != 0)
    {
        // we try to turn off power for the attached sensors or Hardware. Does not work on all devices
        deactivatePowerRail();
        delay(5000);
        activatePowerRail();
        lI2c = clearI2cBus();
    }
    switch (lI2c)
    {
        case 1:
            openknx.log("I2C", "SCL clock line held low\n");
            break;
        case 2:
            openknx.log("I2C", "SCL clock line held low by slave clock stretch\n");
            break;
        case 3:
            openknx.log("I2C", "SDA data line held low\n");
            break;
        default:
            openknx.log("I2C", "I2C bus cleared successfully\n");
            Wire.begin();
            lResult = true;
            break;
    }

    if (!lResult)
    {
        fatalError(FATAL_I2C_BUSY, "Failed to initialize I2C-Bus");
    }
#ifdef I2C_EEPROM_DEVICE_ADDRESSS
    // we check here Hardware we rely on
    openknx.log("I2C", "Checking EEPROM existence... ");
    // check for I2C ack
    Wire.beginTransmission(I2C_EEPROM_DEVICE_ADDRESSS);
    lResult = (Wire.endTransmission() == 0);
    if (lResult)
        boardHardware |= BOARD_HW_EEPROM;
    printResult(lResult);
#endif

#ifdef I2C_1WIRE_DEVICE_ADDRESSS
#if COUNT_1WIRE_BUSMASTER >= 1
#ifdef SENSORMODULE
    // check for I2C ack
    openknx.log("I2C", "Checking 1-Wire existence... ");
    Wire.beginTransmission(I2C_1WIRE_DEVICE_ADDRESSS);
    lResult = (Wire.endTransmission() == 0);
    if (lResult)
        boardHardware |= BOARD_HW_ONEWIRE;
    printResult(lResult);
#endif
#ifdef WIREGATEWAY
    // check for I2C ack
    openknx.log("I2C", "Checking 1-Wire existence 0x19 ... ");
    Wire.beginTransmission(I2C_1WIRE_DEVICE_ADDRESSS + 1);
    lResult = (Wire.endTransmission() == 0);
    if (lResult)
        boardHardware |= BOARD_HW_ONEWIRE;
    printResult(lResult);
#endif
#endif
#if COUNT_1WIRE_BUSMASTER >= 2
    // check for I2C ack
    openknx.log("I2C", "Checking 1-Wire existence 0x1A... ");
    Wire.beginTransmission(I2C_1WIRE_DEVICE_ADDRESSS + 2);
    lResult = (Wire.endTransmission() == 0);
    if (lResult)
        boardHardware |= BOARD_HW_ONEWIRE;
    printResult(lResult);
#endif
#if COUNT_1WIRE_BUSMASTER == 3
    // check for I2C ack
    openknx.log("I2C", "Checking 1-Wire existence 0x1B... ");
    Wire.beginTransmission(I2C_1WIRE_DEVICE_ADDRESSS + 3);
    lResult = (Wire.endTransmission() == 0);
    if (lResult)
        boardHardware |= BOARD_HW_ONEWIRE;
    printResult(lResult);
#endif
#endif

#ifdef I2C_RGBLED_DEVICE_ADDRESS
    openknx.log("I2C", "Checking LED driver existence... ");
    // check for I2C ack
    Wire.beginTransmission(I2C_RGBLED_DEVICE_ADDRESS);
    lResult = (Wire.endTransmission() == 0);
    if (lResult)
        boardHardware |= BOARD_HW_LED;
    printResult(lResult);
#endif
    // lResult = checkUartExistence();
#endif // NO_I2C
    return lResult;
}

bool checkUartExistence()
{
    openknx.log("Helper", "Checking UART existence...\n");
    bool lResult = false;
    initUart();
    // send system state command and interpret answer
    uint8_t lResp = sendUartCommand("SYSTEM_STATE", U_SYSTEM_STATE, U_SYSTEM_STAT_IND, 1);
    lResult = (lResp & 3) == 3;
    printResult(lResult);
    if (lResult)
        boardHardware |= BOARD_HW_NCN5130;
    return lResult;
}

bool initUart()
{
    Serial1.end();
    Serial1.begin(19200, SERIAL_8E1);
    for (uint16_t lCount = 0; !Serial1 && lCount < 1000; lCount++)
        ;
    if (!Serial1)
    {
        openknx.log("Helper", "initUart() failed, something is going completely wrong!");
        return false;
    }
    return true;
}

uint8_t sendUartCommand(const char* iInfo, uint8_t iCmd, uint8_t iResp, uint8_t iLen /* = 0 */)
{
    openknx.log("Helper", "    Send command %s (%02X)... ", iInfo, iCmd);
    // send system state command and interpret answer
    Serial1.write(iCmd);

    int lResp = 0;
    uint32_t lUartResponseDelay = millis();
    while (!delayCheck(lUartResponseDelay, 100))
    {
        lResp = Serial1.read();
        if (lResp == iResp)
        {
            openknx.log("Helper", "OK - received expected response (%02X)\n", lResp);
            if (iLen == 1)
                lResp = Serial1.read();
            break;
        }
    }
    return lResp;
}

bool boardWithOneWire()
{
    return (boardHardware & BOARD_HW_ONEWIRE);
}

bool boardWithLed()
{
    return (boardHardware & BOARD_HW_LED);
}

bool boardWithEEPROM()
{
    return (boardHardware & BOARD_HW_EEPROM);
}

bool boardWithNCN5130()
{
    return (boardHardware & BOARD_HW_NCN5130);
}

/**
 * This routine turns off the I2C bus and clears it
 * on return SCA and SCL pins are tri-state inputs.
 * You need to call Wire.begin() after this to re-enable I2C
 * This routine does NOT use the Wire library at all.
 *
 * returns 0 if bus cleared
 *         1 if SCL held low.
 *         2 if SDA held low by slave clock stretch for > 2sec
 *         3 if SDA held low after 20 clocks.
 */
uint8_t clearI2cBus()
{
#if defined(TWCR) && defined(TWEN)
    TWCR &= ~(_BV(TWEN)); // Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
#endif
    pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
    pinMode(SCL, INPUT_PULLUP);

    // delay(2500); // Wait 2.5 secs. This is strictly only necessary on the first power
    // up of the DS3231 module to allow it to initialize properly,
    // but is also assists in reliable programming of FioV3 boards as it gives the
    // IDE a chance to start uploaded the program
    // before existing sketch confuses the IDE by sending Serial data.

    boolean SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
    if (SCL_LOW)
    {             // If it is held low Arduno cannot become the I2C master.
        return 1; // I2C bus error. Could not clear SCL clock line held low
    }

    boolean SDA_LOW = (digitalRead(SDA) == LOW); // vi. Check SDA input.
    int clockCount = 20;                         // > 2x9 clock

    while (SDA_LOW && (clockCount > 0))
    { //  vii. If SDA is Low,
        clockCount--;
        // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
        pinMode(SCL, INPUT);        // release SCL pullup so that when made output it will be LOW
        pinMode(SCL, OUTPUT);       // then clock SCL Low
        delayMicroseconds(10);      //  for >5uS
        pinMode(SCL, INPUT);        // release SCL LOW
        pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
        // do not force high as slave may be holding it low for clock stretching.
        delayMicroseconds(10); //  for >5uS
        // The >5uS is so that even the slowest I2C devices are handled.
        SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
        int counter = 20;
        while (SCL_LOW && (counter > 0))
        { //  loop waiting for SCL to become High only wait 2sec.
            counter--;
            delay(100);
            SCL_LOW = (digitalRead(SCL) == LOW);
        }
        if (SCL_LOW)
        {             // still low after 2 sec error
            return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
        }
        SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
    }
    if (SDA_LOW)
    {             // still low
        return 3; // I2C bus error. Could not clear. SDA data line held low
    }

    // else pull SDA line low for Start or Repeated Start
    pinMode(SDA, INPUT);  // remove pullup.
    pinMode(SDA, OUTPUT); // and then make it LOW i.e. send an I2C Start or Repeated start control.
    // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
    /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
    delayMicroseconds(10);      // wait >5uS
    pinMode(SDA, INPUT);        // remove output low
    pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
    delayMicroseconds(10);      // x. wait >5uS
    pinMode(SDA, INPUT);        // and reset pins as tri-state inputs which is the default state on reset
    pinMode(SCL, INPUT);
    return 0; // all ok
}
