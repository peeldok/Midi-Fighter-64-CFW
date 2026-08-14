#include <avr/pgmspace.h>
#include "rotation.h"
#include "constants.h"
#include "eeprom.h"
#include "midi.h"
#include "sysex.h"

static uint8_t g_rotation = ROTATION_0;

static const uint8_t rotation_map[4][64] PROGMEM = {
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63
    },
    {
        28, 24, 20, 16, 29, 25, 21, 17, 30, 26, 22, 18, 31, 27, 23, 19,
        60, 56, 52, 48, 61, 57, 53, 49, 62, 58, 54, 50, 63, 59, 55, 51,
        12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3,
        44, 40, 36, 32, 45, 41, 37, 33, 46, 42, 38, 34, 47, 43, 39, 35
    },
    {
        63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48,
        47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,
        31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
    },
    {
        35, 39, 43, 47, 34, 38, 42, 46, 33, 37, 41, 45, 32, 36, 40, 44,
        3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12,
        51, 55, 59, 63, 50, 54, 58, 62, 49, 53, 57, 61, 48, 52, 56, 60,
        19, 23, 27, 31, 18, 22, 26, 30, 17, 21, 25, 29, 16, 20, 24, 28
    }
};

static void rotation_send_state(void)
{
    uint8_t payload[] = {
        0xF0, MIDI_MFR_ID_0, MIDI_MFR_ID_1, MIDI_MFR_ID_2,
        SYSEX_COMMAND_ROTATION, g_rotation, 0xF7
    };
    midi_stream_sysex(sizeof(payload), payload);
}

static void rotation_sysex(uint16_t length, uint8_t *buffer)
{
    if (length == 0) return;

    uint8_t value = buffer[0];
    if (value != ROTATION_QUERY) {
        value &= 0x03;
        rotation_set(value);
        if (eeprom_read(EE_ROTATION) != value) {
            eeprom_write(EE_ROTATION, value);
        }
    }

    rotation_send_state();
}

void rotation_setup(void)
{
    uint8_t value;
    if (eeprom_read(EE_ROTATION_MAGIC) != 0xA5) {
        value = ROTATION_0;
        eeprom_write(EE_ROTATION, value);
        eeprom_write(EE_ROTATION_MAGIC, 0xA5);
    } else {
        value = eeprom_read(EE_ROTATION);
        if (value > ROTATION_270) {
            value = ROTATION_0;
            eeprom_write(EE_ROTATION, value);
        }
    }
    g_rotation = value;
    sysex_install(SYSEX_COMMAND_ROTATION, rotation_sysex);
}

void rotation_set(uint8_t rotation)
{
    g_rotation = rotation & 0x03;
}

uint8_t rotation_get(void)
{
    return g_rotation;
}

uint8_t rotation_logical_to_physical(uint8_t key)
{
    key &= 0x3F;
    return pgm_read_byte(&rotation_map[g_rotation][key]);
}

uint8_t rotation_physical_to_logical(uint8_t key)
{
    key &= 0x3F;
    uint8_t inverse = (uint8_t)((4 - g_rotation) & 0x03);
    return pgm_read_byte(&rotation_map[inverse][key]);
}
