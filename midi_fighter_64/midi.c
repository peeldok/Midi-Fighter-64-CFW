#include "constants.h"
#include "midi.h"
#include "led.h"
#include "rotation.h"

bool midi_clock_enabled = false;
uint8_t ticks = 0;

static uint8_t g_sysex_response_cable = 0;

uint8_t G_EE_MIDI_CHANNEL = 14;
uint8_t G_EE_MIDI_VELOCITY = 74;

uint8_t g_midi_note_off_counter[NUM_BUTTONS];

static inline void midi_send_event_both(MIDI_EventPacket_t* event, uint8_t cin)
{
    event->Event = (uint8_t)((0U << 4) | (cin & 0x0F));
    MIDI_Device_SendEventPacket(event);
    event->Event = (uint8_t)((1U << 4) | (cin & 0x0F));
    MIDI_Device_SendEventPacket(event);
}

void midi_set_sysex_response_cable(const uint8_t cable)
{
    g_sysex_response_cable = cable & 0x01;
}

void midi_stream_note_ch(const uint8_t channel,
						 const uint8_t pitch,
                         const bool onoff)

{

    uint8_t command = ((onoff)? 0x90 : 0x80);

    MIDI_EventPacket_t midi_event;
    midi_event.Data1       = command | (channel & 0x0f);
    midi_event.Data2       = pitch & 0x7f;
    midi_event.Data3       = G_EE_MIDI_VELOCITY & 0x7f;

    midi_send_event_both(&midi_event, (uint8_t)(command >> 4));
}

void midi_stream_raw_cc(const uint8_t channel,
                        const uint8_t cc,
                        const uint8_t value)
{
    const uint8_t command = 0xb0;
    MIDI_EventPacket_t midi_event;
    midi_event.Data1       = command | (channel & 0x0f);
    midi_event.Data2       = cc & 0x7f;
    midi_event.Data3       = value & 0x7f;
    midi_send_event_both(&midi_event, (uint8_t)(command >> 4));
}

void midi_stream_sysex_cable(const uint16_t length, uint8_t* data, const uint8_t cable)
{
    MIDI_EventPacket_t midi_event;
    uint16_t num = length;
    bool first = true;
    const uint8_t cable_bits = (uint8_t)((cable & 0x01) << 4);

    while (num > 3) {
        midi_event.Event = (uint8_t)(cable_bits | 0x04);
        midi_event.Data1 = *data++;
        midi_event.Data2 = *data++;
        midi_event.Data3 = *data++;
        MIDI_Device_SendEventPacket(&midi_event);
        first = false;
        num -= 3;
    }

    if (num) {
        midi_event.Event = (uint8_t)(cable_bits | 0x05);
        midi_event.Data1 = *data++;
        midi_event.Data2 = 0;
        midi_event.Data3 = 0;

        if (num == 2) {
            midi_event.Event = (uint8_t)(cable_bits | 0x06);
            midi_event.Data2 = *data++;
        } else if (num == 3) {
            midi_event.Event = (uint8_t)(cable_bits | (first ? 0x03 : 0x07));
            midi_event.Data2 = *data++;
            midi_event.Data3 = *data++;
        }
        MIDI_Device_SendEventPacket(&midi_event);
    }
}

void midi_stream_sysex(const uint16_t length, uint8_t* data)
{
    midi_stream_sysex_cable(length, data, g_sysex_response_cable);
}

uint8_t midi_64_key_to_note(const uint8_t keynum)
{
	uint8_t note = MIDI_BASENOTE + rotation_physical_to_logical(keynum);
	return note;
}

void midi_clock(void)
{

	if (!midi_clock_enabled) {
        display_flash_counter = 0;
        midi_clock_enabled = true;
    }
	if(ticks == 3)
	{
		ticks = 1;
		display_flash_counter += 1;
	}
	else
	{
		ticks += 1;
	}
}

