/*
 * EEPROM Utils
 * Makes access to EEPROM easier and more practical
 * David Trotz
 * 04/27/2014
 */
#ifndef __EEPROM_UTILS_H__
#define __EEPROM_UTILS_H__

#include <Arduino.h>

int32_t readInt32AtAddressInEEPROM(int address);
int64_t readInt64AtAddressInEEPROM(int address);
void writeInt32ToAddressInEEPROM(int address, int32_t value);
void writeInt64ToAddressInEEPROM(int address, int64_t value);

#endif