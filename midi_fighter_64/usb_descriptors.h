#ifndef _USB_DESCRIPTORS_H_INCLUDED
#define _USB_DESCRIPTORS_H_INCLUDED

#include <stdint.h>
#include <avr/pgmspace.h>

#define MIDI_STREAM_OUT_EPNUM 1
#define MIDI_STREAM_IN_EPNUM 2

const uint8_t *usb_descriptor_get(uint8_t type, uint8_t index, uint16_t *size);

#endif
