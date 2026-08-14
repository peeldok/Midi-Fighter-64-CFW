#ifndef _SYSEX_H_INCLUDED
#define _SYSEX_H_INCLUDED

#include <stdint.h>

#include "midi.h"

typedef void (*SysExFn)(uint16_t, uint8_t*);

#define sysex_install(cmd,fn) sysex_install_(cmd, (SysExFn)fn)
void sysex_install_ (uint8_t cmd, SysExFn fn);

void sysex_handle_3sc (MIDI_EventPacket_t* packet);

void sysex_handle_3e (MIDI_EventPacket_t* packet);

void sysex_handle_2e (MIDI_EventPacket_t* packet);

void sysex_handle_1e (MIDI_EventPacket_t* packet);

#endif
