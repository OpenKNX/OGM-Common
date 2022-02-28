#pragma once

#include <Arduino.h>

// this function is a replacement for knx.read() and implements version checks for OpenKNX
// it ensures, that only correct applicatins can be loaded from flash
void knxRead(uint8_t openKnxId, uint8_t applicationNumber, uint8_t applicationVersion, uint8_t firmwareRevision);