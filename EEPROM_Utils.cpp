/*
 * EEPROM Utils
 * Makes access to EEPROM easier and more practical
 * David Trotz
 * 04/27/2014
 */
#include <Arduino.h>
#include <EEPROM.h>
#include "EEPROM_Utils.h"

int32_t EEPROM_Utils::readInt32AtAddress(int address) {
    byte b3 = EEPROM.read(address + 0);
    byte b2 = EEPROM.read(address + 1);
    byte b1 = EEPROM.read(address + 2);
    byte b0 = EEPROM.read(address + 3);
    return  ((int32_t)b3 << 24) | ((int32_t)b2 << 16) | 
            ((int32_t)b1 << 8)  | (int32_t)b0;
}

int64_t EEPROM_Utils::readInt64AtAddress(int address) {
    byte b7 = EEPROM.read(address + 0);
    byte b6 = EEPROM.read(address + 1);
    byte b5 = EEPROM.read(address + 2);
    byte b4 = EEPROM.read(address + 3);
    byte b3 = EEPROM.read(address + 4);
    byte b2 = EEPROM.read(address + 5);
    byte b1 = EEPROM.read(address + 6);
    byte b0 = EEPROM.read(address + 7);
    
    return  ((int32_t)b7 << 56) | ((int32_t)b6 << 48) | 
            ((int32_t)b5 << 40) | ((int32_t)b4 << 32) | 
            ((int32_t)b3 << 24) | ((int32_t)b2 << 16) | 
            ((int32_t)b1 << 8)  | (int32_t)b0;
}


void EEPROM_Utils::writeInt32ToAddress(int address, int32_t value) {
    EEPROM.write(address, (byte)((value >> 24) & 0x000000FF));
    EEPROM.write(address, (byte)((value >> 16) & 0x000000FF));
    EEPROM.write(address, (byte)((value >>  8) & 0x000000FF));
    EEPROM.write(address, (byte)(value         & 0x000000FF));
}

void EEPROM_Utils::writeInt64ToAddress(int address, int64_t value) {
    EEPROM.write(address, (byte)((value >> 56) & 0x000000FF));
    EEPROM.write(address, (byte)((value >> 48) & 0x000000FF));
    EEPROM.write(address, (byte)((value >> 40) & 0x000000FF));
    EEPROM.write(address, (byte)((value >> 32) & 0x000000FF));                
    EEPROM.write(address, (byte)((value >> 24) & 0x000000FF));
    EEPROM.write(address, (byte)((value >> 16) & 0x000000FF));
    EEPROM.write(address, (byte)((value >>  8) & 0x000000FF));
    EEPROM.write(address, (byte)(value         & 0x000000FF));
}