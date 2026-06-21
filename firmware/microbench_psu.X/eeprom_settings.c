#include "eeprom_settings.h"
#include "mcc_generated_files/nvm/nvm.h"

// EEPROM layout
#define EEPROM_MAGIC_ADDR    0x00
#define EEPROM_MAGIC_VALUE   0xAB
#define EEPROM_VOLTAGE_HI    0x01
#define EEPROM_VOLTAGE_LO    0x02
#define EEPROM_CURRENT_HI    0x03
#define EEPROM_CURRENT_LO    0x04

void settings_save(uint16_t voltage, uint16_t current) {
	NVM_UnlockKeySet(UNLOCK_KEY);
	EEPROM_Write(EEPROM_START_ADDRESS + EEPROM_VOLTAGE_HI, voltage >> 8);
	while (NVM_IsBusy());
	EEPROM_Write(EEPROM_START_ADDRESS + EEPROM_VOLTAGE_LO, voltage & 0xFF);
	while (NVM_IsBusy());
	EEPROM_Write(EEPROM_START_ADDRESS + EEPROM_CURRENT_HI, current >> 8);
	while (NVM_IsBusy());
	EEPROM_Write(EEPROM_START_ADDRESS + EEPROM_CURRENT_LO, current & 0xFF);
	while (NVM_IsBusy());
	EEPROM_Write(EEPROM_START_ADDRESS + EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
	while (NVM_IsBusy());
	NVM_UnlockKeyClear();
}

void settings_load(uint16_t *voltage, uint16_t *current) {
	if (EEPROM_Read(EEPROM_START_ADDRESS + EEPROM_MAGIC_ADDR) != EEPROM_MAGIC_VALUE) {
		return; // no saved data -- keep defaults
	}
	*voltage = ((uint16_t)EEPROM_Read(EEPROM_START_ADDRESS + EEPROM_VOLTAGE_HI) << 8)
	         |  (uint16_t)EEPROM_Read(EEPROM_START_ADDRESS + EEPROM_VOLTAGE_LO);
	*current = ((uint16_t)EEPROM_Read(EEPROM_START_ADDRESS + EEPROM_CURRENT_HI) << 8)
	         |  (uint16_t)EEPROM_Read(EEPROM_START_ADDRESS + EEPROM_CURRENT_LO);
}
