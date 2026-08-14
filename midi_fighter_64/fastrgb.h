#ifndef _fastrgb_H_INCLUDED
#define _fastrgb_H_INCLUDED

#include <stdint.h>
#include "constants.h"

extern uint8_t g_fastrgb_state[NUM_BUTTONS][3];

extern void fastrgb_clear(void);

extern void fastrgb_decompress(uint8_t* d, uint8_t* end);

extern void fastrgb_list(uint8_t* d, uint8_t* end);

extern void fastrgb_single_unsafe(uint8_t p, uint8_t r, uint8_t g, uint8_t b);

extern void fastrgb_palette_single(uint8_t p, uint8_t channel, uint8_t v);

extern void fastrgb_ch4_setup(void);
extern void fastrgb_ch4_single(uint8_t p, uint8_t v);
extern void fastrgb_ch4_read_color(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b);
extern void fastrgb_ch4_write_component6(uint8_t component, const uint8_t *values);
extern uint8_t fastrgb_ch4_read_component6(uint8_t component, uint8_t index);

#endif
