#ifndef _MIDI_H_INCLUDED
#define _MIDI_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include "native_usb.h"

#include "constants.h"

extern uint8_t G_EE_MIDI_CHANNEL;
extern uint8_t G_EE_MIDI_VELOCITY;
extern uint8_t g_midi_note_off_counter[NUM_BUTTONS];
extern bool midi_clock_enabled;

void midi_stream_note_ch(const uint8_t channel, const uint8_t note, const bool onoff);
uint8_t midi_64_key_to_note(const uint8_t keynum);

void midi_stream_raw_cc(const uint8_t channel,
                        const uint8_t cc,
                        const uint8_t value);

void midi_stream_sysex (const uint16_t length, uint8_t* data);
void midi_stream_sysex_cable(const uint16_t length, uint8_t* data, const uint8_t cable);
void midi_set_sysex_response_cable(const uint8_t cable);

void midi_clock(void);


#endif
