#ifndef EEPROM_SETTINGS_H
#define	EEPROM_SETTINGS_H

#include <stdint.h>

void settings_save(uint16_t voltage, uint16_t current);
void settings_load(uint16_t *voltage, uint16_t *current);

#endif	/* EEPROM_SETTINGS_H */
