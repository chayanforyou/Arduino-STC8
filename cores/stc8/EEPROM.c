#include "EEPROM.h"
#include "variant.h"

#define IAP_CMD_IDLE  0x00
#define IAP_CMD_READ  0x01
#define IAP_CMD_WRITE 0x02
#define IAP_CMD_ERASE 0x03
#define IAP_ENABLE    0x80

static void iap_idle(void)
{
    IAP_CONTR = 0;
    IAP_CMD   = IAP_CMD_IDLE;
    IAP_TRIG  = 0;
    IAP_ADDRH = 0xFF;
    IAP_ADDRL = 0xFF;
}

static uint8_t eeprom_read(uint16_t address) __reentrant
{
    uint8_t dat;

    IAP_CONTR = IAP_ENABLE;
    IAP_TPS   = (uint8_t)(F_CPU / 1000000UL);
    IAP_CMD   = IAP_CMD_READ;
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

    dat = IAP_DATA;
    iap_idle();
    return dat;
}

static void eeprom_write(uint16_t address, uint8_t value) __reentrant
{
    uint8_t ea_state = READ_BIT(IE, 7);
    CLEAR_BIT(IE, 7);

    IAP_CONTR = IAP_ENABLE;
    IAP_TPS   = (uint8_t)(F_CPU / 1000000UL);
    IAP_CMD   = IAP_CMD_WRITE;
    IAP_ADDRH = (uint8_t)(address >> 8);
    IAP_ADDRL = (uint8_t)(address & 0xFF);
    IAP_DATA  = value;

    IAP_TRIG = 0x5A;
    IAP_TRIG = 0xA5;
    __asm
        nop
        nop
        nop
        nop
    __endasm;

    iap_idle();

    if (ea_state) {
        SET_BIT(IE, 7);
    }
}

static void eeprom_erase_sector(uint16_t address) __reentrant
{
    uint8_t ea_state = READ_BIT(IE, 7);
    CLEAR_BIT(IE, 7);

    IAP_CONTR = IAP_ENABLE;
    IAP_TPS   = (uint8_t)(F_CPU / 1000000UL);
    IAP_CMD   = IAP_CMD_ERASE;
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

    iap_idle();

    if (ea_state) {
        SET_BIT(IE, 7);
    }
}

static void eeprom_update(uint16_t address, uint8_t value) __reentrant
{
    if (eeprom_read(address) != value)
    {
        eeprom_write(address, value);
    }
}

static void eeprom_read_block(uint16_t address, void *dest, uint16_t size) __reentrant
{
    uint8_t *ptr = (uint8_t *)dest;
    while (size--)
    {
        *ptr++ = eeprom_read(address++);
    }
}

static void eeprom_write_block(uint16_t address, const void *src, uint16_t size) __reentrant
{
    const uint8_t *ptr = (const uint8_t *)src;
    while (size--)
    {
        eeprom_update(address++, *ptr++);
    }
}

EEPROM_t EEPROM = {
    .read        = eeprom_read,
    .write       = eeprom_write,
    .update      = eeprom_update,
    .eraseSector = eeprom_erase_sector,
    .readBlock   = eeprom_read_block,
    .writeBlock  = eeprom_write_block,
};
