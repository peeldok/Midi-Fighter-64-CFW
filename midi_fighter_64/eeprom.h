#ifndef _EEPROM_H_INCLUDED
#define _EEPROM_H_INCLUDED

#include <stdint.h>
extern uint8_t G_EE_MIDI_OUTPUT_MODE;
extern uint8_t G_EE_SLEEP_TIME;

void eeprom_write(uint16_t address, uint8_t data);
void eeprom_update(uint16_t address, uint8_t data);
uint8_t eeprom_read(uint16_t address);
void eeprom_factory_reset(void);
void eeprom_setup(void);

#endif
