#include "EEPROM.h"
#include "variant.h"

#define IAP_CMD_IDLE 0x00
#define IAP_CMD_READ 0x01
#define IAP_CMD_WRITE 0x02
#define IAP_CMD_ERASE 0x03
#define IAP_ENABLE 0x80
#define IAP_CMD_FAIL 0x10  // IAP_CONTR.4, set by hardware when a command fails

// IAP_TPS = F_CPU in MHz, rounded to nearest (datasheet 15.1)
#define IAP_TPS_VALUE ((uint8_t)((F_CPU + 500000UL) / 1000000UL))

static void iap_idle(void) {
  IAP_CONTR = 0;
  IAP_CMD = IAP_CMD_IDLE;
  IAP_TRIG = 0;
  IAP_ADDRH = 0xFF;
  IAP_ADDRL = 0xFF;
}

// Run one IAP command (IAP_DATA must already hold the byte for a write).
// Returns false if the hardware reported CMD_FAIL.
static bool iap_exec(uint8_t cmd, uint16_t address) {
  bool ok;
  uint8_t ea_state = READ_BIT(IE, 7);
  CLEAR_BIT(IE, 7);

  IAP_CONTR = IAP_ENABLE;  // Also clears CMD_FAIL
  IAP_TPS = IAP_TPS_VALUE;
  IAP_CMD = cmd;
  IAP_ADDRH = (uint8_t)(address >> 8);
  IAP_ADDRL = (uint8_t)(address & 0xFF);

  IAP_TRIG = 0x5A;
  IAP_TRIG = 0xA5;
    __asm
        nop
        nop
        nop
        nop
    __endasm;

  ok = !(IAP_CONTR & IAP_CMD_FAIL);
  iap_idle();

  if (ea_state) {
    SET_BIT(IE, 7);
  }
  return ok;
}

static uint8_t eeprom_read(uint16_t address) __reentrant {
  if (address >= EEPROM_SIZE || !iap_exec(IAP_CMD_READ, address)) {
    return 0xFF;
  }
  return IAP_DATA;
}

static bool eeprom_write(uint16_t address, uint8_t value) __reentrant {
  if (address >= EEPROM_SIZE) {
    return false;
  }

  // Flash can only clear bits (1 -> 0); setting a bit needs eraseSector()
  if ((eeprom_read(address) & value) != value) {
    return false;
  }

  IAP_DATA = value;
  if (!iap_exec(IAP_CMD_WRITE, address)) {
    return false;
  }
  return eeprom_read(address) == value;
}

static bool eeprom_erase_sector(uint16_t address) __reentrant {
  if (address >= EEPROM_SIZE) {
    return false;
  }
  return iap_exec(IAP_CMD_ERASE, address);
}

static bool eeprom_update(uint16_t address, uint8_t value) __reentrant {
  if (address < EEPROM_SIZE && eeprom_read(address) == value) {
    return true;
  }
  return eeprom_write(address, value);
}

static void eeprom_read_block(uint16_t address, void *dest, uint16_t size) __reentrant {
  uint8_t *ptr = (uint8_t *)dest;
  while (size--) {
    *ptr++ = eeprom_read(address++);
  }
}

static bool eeprom_write_block(uint16_t address, const void *src, uint16_t size) __reentrant {
  const uint8_t *ptr = (const uint8_t *)src;
  while (size--) {
    if (!eeprom_update(address++, *ptr++)) {
      return false;
    }
  }
  return true;
}

static uint16_t eeprom_length(void) {
  return EEPROM_SIZE;
}

// EEPROM object instance (stored in Flash ROM)
const EEPROM_t EEPROM = {
  .read = eeprom_read,
  .write = eeprom_write,
  .update = eeprom_update,
  .eraseSector = eeprom_erase_sector,
  .readBlock = eeprom_read_block,
  .writeBlock = eeprom_write_block,
  .length = eeprom_length,
};
