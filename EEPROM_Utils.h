/*
 * EEPROM Utils
 * Makes access to EEPROM easier and more practical
 * David Trotz
 * 04/27/2014
 */
#ifndef __EEPROM_UTILS_H__
#define __EEPROM_UTILS_H__

#include <Arduino.h>

class EEPROM_Utils {
public:
    static int32_t readInt32AtAddress(int address);
    static int64_t readInt64AtAddress(int address);
    static void writeInt32ToAddress(int address, int32_t value);
    static void writeInt64ToAddress(int address, int64_t value);
};

#endif