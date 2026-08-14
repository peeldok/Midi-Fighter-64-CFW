#include <avr/io.h>
#include <avr/interrupt.h>

#include "midi.h"
#include "display.h"
#include "eeprom.h"
#include "constants.h"

uint8_t G_EE_MIDI_OUTPUT_MODE;
uint8_t G_EE_SLEEP_TIME;

void eeprom_write(uint16_t address, uint8_t data)
{

    while(EECR & (1<<EEPE)) {}

    EEAR = address & 0x0fff;
    EEDR = data;

    cli();

    EECR |= (1<<EEMPE);

    EECR |= (1<<EEPE);

    sei();
}

void eeprom_update(uint16_t address, uint8_t data)
{
    if (eeprom_read(address) != data) {
        eeprom_write(address, data);
    }
}

uint8_t eeprom_read(uint16_t address)
{

    while(EECR & (1<<EEPE)) {}

    EEAR = address;

    EECR |= (1<<EERE);

    return EEDR;
}

void eeprom_setup(void)
{

    if (eeprom_read(EE_EEPROM_VERSION) != EEPROM_LAYOUT) {
        eeprom_factory_reset();
    }

    G_EE_MIDI_CHANNEL = eeprom_read(EE_MIDI_CHANNEL);
    G_EE_MIDI_VELOCITY = eeprom_read(EE_MIDI_VELOCITY);
	G_EE_MIDI_OUTPUT_MODE = eeprom_read(EE_MIDI_OUTPUT_MODE);
    G_EE_SLEEP_TIME = eeprom_read(EE_SLEEP_TIME);
}

void eeprom_factory_reset(void)
{

    eeprom_write(EE_EEPROM_VERSION, EEPROM_LAYOUT);

    eeprom_write(EE_MIDI_CHANNEL, G_EE_MIDI_CHANNEL = 2);
    eeprom_write(EE_MIDI_VELOCITY, G_EE_MIDI_VELOCITY = 127);
	eeprom_write(EE_COMBOS_ENABLE, 0x01);
	eeprom_write(EE_MIDI_OUTPUT_MODE, G_EE_MIDI_OUTPUT_MODE = MIDI_OUTPUT_MODE_NOTES_ONLY);
	eeprom_write(EE_FOUR_BANKS_MODE, 0x00);
	eeprom_write(EE_TILT_MODE, 0x02);
	eeprom_write(EE_TILT_MASK, 0xF1);
	eeprom_write(EE_ANIMATIONS, 0x04);
 	eeprom_write(EE_TILT_SENSITIVITY, 0x1E);
 	eeprom_write(EE_PITCH_SENSITIVITY, 0x7F);
 	eeprom_write(EE_TILT_RANGE, 0x46);
 	eeprom_write(EE_PITCH_RANGE, 0x3C);
 	eeprom_write(EE_TILT_DEADZONE, 0x0C);
 	eeprom_write(EE_PITCH_DEADZONE, 0x7F);
 	eeprom_write(EE_TILT_AXIS, 0x00);
	eeprom_write(EE_PICK_SENSITIVITY, 0x40);
	eeprom_write(EE_SIDE_BANK, 0x00);
	eeprom_write(EE_ROTATION, 0x00);
	eeprom_write(EE_ROTATION_MAGIC, 0xA5);
	eeprom_write(EE_SLEEP_TIME, G_EE_SLEEP_TIME = 0x3C);
    eeprom_write(EE_CH4_MAGIC0, 0x00);
    eeprom_write(EE_CH4_MAGIC1, 0x00);
    eeprom_write(EE_LED_BRIGHTNESS, 180U);
    eeprom_write(EE_LED_BRIGHTNESS_MAGIC, EE_LED_BRIGHTNESS_MAGIC_VALUE);

    for (uint16_t i=0; i<NUM_BUTTONS*2; i++) {
        for (uint8_t j=0; j<3; j++) {
            eeprom_write(
                EE_COLORS_IDLE+(i*3+j),
                default_color[i < NUM_BUTTONS? COLORID_OFF : COLORID_WHITE][j]
            );
            eeprom_write(
                EE_COLORS_ACTIVE+(i*3+j),
                default_color[i < NUM_BUTTONS? COLORID_BLUE : COLORID_GREEN][j]
            );
        }
    }
}
