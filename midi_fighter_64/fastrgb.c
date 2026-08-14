#include <string.h>
#include "fastrgb.h"
#include "rotation.h"
#include "eeprom.h"
#include "palettes.h"
#include <avr/pgmspace.h>

uint8_t g_fastrgb_state[NUM_BUTTONS][3];

void fastrgb_clear(void) {
	memset(g_fastrgb_state, 0, sizeof(g_fastrgb_state));
}

inline void fastrgb_set_unsafe(uint8_t p, uint8_t r, uint8_t g, uint8_t b) {
	p = rotation_logical_to_physical(p);
	g_fastrgb_state[p][0] = r == 0? 0 : (r + 2);
	g_fastrgb_state[p][1] = g == 0? 0 : (g + 2);
	g_fastrgb_state[p][2] = b == 0? 0 : (b + 2);
}

inline void fastrgb_set(uint8_t p, uint8_t r, uint8_t g, uint8_t b) {
	fastrgb_set_unsafe(p & BUTTON_ID_FLAGS, r & 0x3F, g & 0x3F, b & 0x3F);
}

void fastrgb_decompress(uint8_t* d, uint8_t* end) {
	for (uint8_t* i = d; i < end;) {
		uint8_t r = *i++;
		uint8_t g = *i++;
		uint8_t b = *i++;

		uint8_t n = ((r & 0x40) >> 4) | ((g & 0x40) >> 5) | ((b & 0x40) >> 6);
		if (n == 0) n = *i++;

		r &= 0x3F;
		g &= 0x3F;
		b &= 0x3F;

		for (uint8_t j = 0; j < n; j++) {
			uint8_t x = *i++;

			if ((x & 0b01110000) != 0b01100000) {
				fastrgb_set_unsafe(x & BUTTON_ID_FLAGS, r, g, b);

				if (x & 0b01000000) {
					uint8_t x_ = ~x & BUTTON_ID_FLAGS;
					fastrgb_set_unsafe(x_, r, g, b);

					if (x & 0b00100000) {
						fastrgb_set_unsafe((x & 0b00011100) | (x_ & 0b00000011), r, g, b);
						fastrgb_set_unsafe((x & 0b00100011) | (x_ & 0b00011100), r, g, b);
					}
				}

			} else if (x & 0b00001000) {
				uint8_t col = x & (x & 0b00000100? 0b00100011 : 0b00000011);

				for (uint8_t k = 0; k < 8; k++) {
					fastrgb_set_unsafe(col | (k << 2), r, g, b);
				}

			} else {
				uint8_t row = ((x & 0b00000111) << 2);

				for (uint8_t k = 0; k < 4; k++) {
					fastrgb_set_unsafe(row | k, r, g, b);
					fastrgb_set_unsafe(row | 0b00100000 | k, r, g, b);
				}
			}
		}
	}
}

void fastrgb_list(uint8_t* d, uint8_t* end) {
	for (uint8_t* i = d; i + 3 < end; i += 4) {
		fastrgb_set(i[0], i[1], i[2], i[3]);
	}
}

void fastrgb_single_unsafe(uint8_t p, uint8_t r, uint8_t g, uint8_t b) {
	fastrgb_set_unsafe(p, r, g, b);
}

static inline uint8_t palette8_to_internal(uint8_t value) {
    return (uint8_t)(((uint16_t)value + 2U) >> 2);
}

static void fastrgb_set_rgb8_palette(uint8_t p, uint8_t r, uint8_t g, uint8_t b) {
    p = rotation_logical_to_physical(p);
    g_fastrgb_state[p][0] = palette8_to_internal(r);
    g_fastrgb_state[p][1] = palette8_to_internal(g);
    g_fastrgb_state[p][2] = palette8_to_internal(b);
}

void fastrgb_palette_single(uint8_t p, uint8_t channel, uint8_t v) {
    const uint8_t *pr;
    const uint8_t *pg;
    const uint8_t *pb;

    switch (channel) {
        case 0:
            pr = _r;  pg = _g;  pb = _b;
            break;
        case 1:
            pr = _r2; pg = _g2; pb = _b2;
            break;
        case 2:
            pr = _r3; pg = _g3; pb = _b3;
            break;
        default:
            return;
    }

    fastrgb_set_rgb8_palette(
        p,
        pgm_read_byte(&pr[v & 0x7F]),
        pgm_read_byte(&pg[v & 0x7F]),
        pgm_read_byte(&pb[v & 0x7F])
    );
}

static uint8_t ch4_expand_6_to_8(uint8_t value) {
    value &= 0x3F;
    return (uint8_t)((value << 2) | (value >> 4));
}

static uint8_t ch4_compress_8_to_6(uint8_t value) {
    return (uint8_t)(value >> 2);
}

static uint16_t ch4_group_address(uint8_t component, uint8_t group) {
    return (uint16_t)(EE_CH4_PALETTE + ((uint16_t)component * EE_CH4_COMPONENT_BYTES) + ((uint16_t)group * 3U));
}

static void ch4_write_group6(uint8_t component, uint8_t group, uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    uint16_t address = ch4_group_address(component, group);
    a &= 0x3F;
    b &= 0x3F;
    c &= 0x3F;
    d &= 0x3F;
    eeprom_update(address + 0, (uint8_t)(a | ((b & 0x03) << 6)));
    eeprom_update(address + 1, (uint8_t)((b >> 2) | ((c & 0x0F) << 4)));
    eeprom_update(address + 2, (uint8_t)((c >> 4) | (d << 2)));
}

uint8_t fastrgb_ch4_read_component6(uint8_t component, uint8_t index) {
    uint16_t address;
    uint8_t slot;
    uint8_t a;
    uint8_t b;

    if (component > 2) return 0;
    slot = index & 0x03;
    address = ch4_group_address(component, index >> 2);

    if (slot == 0) {
        return (uint8_t)(eeprom_read(address + 0) & 0x3F);
    }
    if (slot == 1) {
        a = eeprom_read(address + 0);
        b = eeprom_read(address + 1);
        return (uint8_t)(((a >> 6) & 0x03) | ((b & 0x0F) << 2));
    }
    if (slot == 2) {
        a = eeprom_read(address + 1);
        b = eeprom_read(address + 2);
        return (uint8_t)(((a >> 4) & 0x0F) | ((b & 0x03) << 4));
    }
    return (uint8_t)((eeprom_read(address + 2) >> 2) & 0x3F);
}

void fastrgb_ch4_write_component6(uint8_t component, const uint8_t *values) {
    if (component > 2 || values == 0) return;
    for (uint8_t group = 0; group < 32; group++) {
        uint8_t index = (uint8_t)(group << 2);
        ch4_write_group6(component, group, values[index], values[index + 1], values[index + 2], values[index + 3]);
    }
}

void fastrgb_ch4_read_color(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b) {
    index &= 0x7F;
    if (r) *r = ch4_expand_6_to_8(fastrgb_ch4_read_component6(0, index));
    if (g) *g = ch4_expand_6_to_8(fastrgb_ch4_read_component6(1, index));
    if (b) *b = ch4_expand_6_to_8(fastrgb_ch4_read_component6(2, index));
}

void fastrgb_ch4_setup(void) {
    if (eeprom_read(EE_CH4_MAGIC0) == EE_CH4_MAGIC0_VALUE &&
        eeprom_read(EE_CH4_MAGIC1) == EE_CH4_MAGIC1_VALUE) {
        return;
    }

    const uint8_t *components[3] = {_r, _g, _b};
    for (uint8_t component = 0; component < 3; component++) {
        for (uint8_t group = 0; group < 32; group++) {
            uint8_t index = (uint8_t)(group << 2);
            ch4_write_group6(
                component,
                group,
                ch4_compress_8_to_6(pgm_read_byte(&components[component][index + 0])),
                ch4_compress_8_to_6(pgm_read_byte(&components[component][index + 1])),
                ch4_compress_8_to_6(pgm_read_byte(&components[component][index + 2])),
                ch4_compress_8_to_6(pgm_read_byte(&components[component][index + 3]))
            );
        }
    }

    eeprom_update(EE_CH4_MAGIC0, EE_CH4_MAGIC0_VALUE);
    eeprom_update(EE_CH4_MAGIC1, EE_CH4_MAGIC1_VALUE);
}

void fastrgb_ch4_single(uint8_t p, uint8_t v) {
    uint8_t r, g, b;
    fastrgb_ch4_read_color(v, &r, &g, &b);
    fastrgb_set_rgb8_palette(p, r, g, b);
}
