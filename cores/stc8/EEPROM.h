#ifndef _EEPROM_H_
#define _EEPROM_H_

#include <stdint.h>
#include <stdbool.h>
#include "Arduino.h"

#define EEPROM_SECTOR_SIZE 512

typedef struct
{
    uint8_t (*read)(uint16_t address) __reentrant;
    void    (*write)(uint16_t address, uint8_t value) __reentrant;
    void    (*update)(uint16_t address, uint8_t value) __reentrant;
    void    (*eraseSector)(uint16_t address) __reentrant;
    void    (*readBlock)(uint16_t address, void *dest, uint16_t size) __reentrant;
    void    (*writeBlock)(uint16_t address, const void *src, uint16_t size) __reentrant;
} EEPROM_t;

extern EEPROM_t EEPROM;

#endif // _EEPROM_H_
