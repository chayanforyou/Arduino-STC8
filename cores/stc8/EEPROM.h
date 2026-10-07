#ifndef _EEPROM_H_
#define _EEPROM_H_

#include <stdint.h>
#include <stdbool.h>
#include "Arduino.h"

// Erase works on a whole 512-byte sector and sets every byte to 0xFF.
// A write can only change bits from 1 to 0, so rewriting a byte that is
// not 0xFF usually needs eraseSector() first (which wipes the whole sector).
#define EEPROM_SECTOR_SIZE 512

typedef struct
{
  uint8_t (*read)(uint16_t address) __reentrant;
  // write/update/eraseSector/writeBlock return false if the data could not
  // be stored (address out of range, bits need an erase, or IAP failure)
  bool (*write)(uint16_t address, uint8_t value) __reentrant;
  bool (*update)(uint16_t address, uint8_t value) __reentrant;
  bool (*eraseSector)(uint16_t address) __reentrant;
  void (*readBlock)(uint16_t address, void *dest, uint16_t size) __reentrant;
  bool (*writeBlock)(uint16_t address, const void *src, uint16_t size) __reentrant;
  uint16_t (*length)(void);
} EEPROM_t;

// EEPROM object instance (stored in Flash ROM)
extern const EEPROM_t EEPROM;

#endif  // _EEPROM_H_
