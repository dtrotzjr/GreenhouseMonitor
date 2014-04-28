/*
 * EEPROM Utils
 * Makes access to EEPROM easier and more practical
 * David Trotz
 * 04/27/2014
 */
#include <Arduino.h>
#include <avr/eeprom.h>
#include "EEPROM_Utils.h"

int32_t readInt32AtAddressInEEPROM(int address) {
    byte b3 = eeprom_read_byte((const uint8_t*)address + 0);
    byte b2 = eeprom_read_byte((const uint8_t*)address + 1);
    byte b1 = eeprom_read_byte((const uint8_t*)address + 2);
    byte b0 = eeprom_read_byte((const uint8_t*)address + 3);
    return  ((int32_t)b3 << 24) | ((int32_t)b2 << 16) | 
            ((int32_t)b1 << 8)  | (int32_t)b0;
}

int64_t readInt64AtAddressInEEPROM(int address) {
    byte b7 = eeprom_read_byte((const uint8_t*)address + 0);
    byte b6 = eeprom_read_byte((const uint8_t*)address + 1);
    byte b5 = eeprom_read_byte((const uint8_t*)address + 2);
    byte b4 = eeprom_read_byte((const uint8_t*)address + 3);
    byte b3 = eeprom_read_byte((const uint8_t*)address + 4);
    byte b2 = eeprom_read_byte((const uint8_t*)address + 5);
    byte b1 = eeprom_read_byte((const uint8_t*)address + 6);
    byte b0 = eeprom_read_byte((const uint8_t*)address + 7);
    
    return  ((int32_t)b7 << 56) | ((int32_t)b6 << 48) | 
            ((int32_t)b5 << 40) | ((int32_t)b4 << 32) | 
            ((int32_t)b3 << 24) | ((int32_t)b2 << 16) | 
            ((int32_t)b1 << 8)  | (int32_t)b0;
}


void writeInt32ToAddressInEEPROM(int address, int32_t value) {
    eeprom_write_byte((uint8_t*)address, (byte)((value >> 24) & 0x000000FF));
    eeprom_write_byte((uint8_t*)address, (byte)((value >> 16) & 0x000000FF));
    eeprom_write_byte((uint8_t*)address, (byte)((value >>  8) & 0x000000FF));
    eeprom_write_byte((uint8_t*)address, (byte)(value         & 0x000000FF));
}

void writeInt64ToAddressInEEPROM(int address, int64_t value) {
    eeprom_write_byte((uint8_t*)address, (byte)((value >> 56) & 0x000000FF));
    eeprom_write_byte((uint8_t*)address, (byte)((value >> 48) & 0x000000FF));
    eeprom_write_byte((uint8_t*)address, (byte)((value >> 40) & 0x000000FF));
    eeprom_write_byte((uint8_t*)address, (byte)((value >> 32) & 0x000000FF));                
    eeprom_write_byte((uint8_t*)address, (byte)((value >> 24) & 0x000000FF));
    eeprom_write_byte((uint8_t*)address, (byte)((value >> 16) & 0x000000FF));
    eeprom_write_byte((uint8_t*)address, (byte)((value >>  8) & 0x000000FF));
    eeprom_write_byte((uint8_t*)address, (byte)(value         & 0x000000FF));
}