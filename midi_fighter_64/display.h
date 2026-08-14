#ifndef _DISPLAY_H_INCLUDED
#define _DISPLAY_H_INCLUDED

#include <stdint.h>

#define DISPLAY_SCALING_COLOR_IN_MAX_VALUE 48
#define DISPLAY_SCALING_COLOR_OUT_MAX_VALUE 127

#define GEOMETRIC_ANIMATION_G_LED_LIMIT	0x1C

#define GEOMETRIC_ANIMATION_STEPS 15

enum DefaultColorIds {
	COLORID_OFF = 0,
	COLORID_RED = 1,
	COLORID_RED_DIM = 2,
	COLORID_ORANGE = 3,
	COLORID_ORANGE_DIM = 4,
	COLORID_YELLOW = 5,
	COLORID_YELLOW_DIM = 6,
	COLORID_CHARTREUSE = 7,
	COLORID_CHARTREUSE_DIM = 8,
	COLORID_GREEN = 9,
	COLORID_GREEN_DIM = 10,
	COLORID_CYAN = 11,
	COLORID_CYAN_DIM = 12,
	COLORID_BLUE = 13,
	COLORID_BLUE_DIM = 14,
	COLORID_LAVENDER = 15,
	COLORID_LAVENDER_DIM = 16,
	COLORID_PINK = 17,
	COLORID_PINK_DIM = 18,
	COLORID_WHITE = 19
};

extern uint8_t g_display_buffer[64 * 3];
extern const uint8_t default_color[20][3];

void default_display_run(void);

void start_geometric_animation(void);

void adjust_inactive_bank_leds_for_power(uint8_t* rgb);
void adjust_active_bank_leds_for_power(uint8_t* rgb);

#endif
