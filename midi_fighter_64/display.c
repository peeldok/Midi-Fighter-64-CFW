
#include "key.h"
#include "fastrgb.h"
#include "display.h"
#include "idle.h"

#include "led.h"
#include "eeprom.h"

uint8_t g_display_buffer[64 * 3];

void left_fade(const uint8_t value, uint8_t *buffer);
void right_fade(const uint8_t value, uint8_t *buffer);
void front_fade(const uint8_t value, uint8_t *buffer);
void back_fade(const uint8_t value, uint8_t *buffer);

const uint8_t default_color[20][3] = {

	{0x00,0x00,0x00},
	{48,0x00,0x00},
	{24,0x00,0x00},
	{40,12,0x00},
	{20,6,0x00},
	{32,25,0x00},
	{16,12,0x00},
	{25,32,0x00},
	{12,16,0x00},
	{0x00,48,0x00},
	{0x00,24,0x00},
	{0x00,30,30},
	{0x00,15,15},
	{0x00,0x00,48},
	{0x00,0x00,24},

	{25,7,32},
	{13,3,17},

	{36,0x00,18},
	{18,0x00,9},
	{24,24,24},
};

uint8_t geometric_animation_pos = GEOMETRIC_ANIMATION_STEPS;

void start_geometric_animation(void) {

	if (g_led_counter[1] == 0) {

		g_led_counter[1] = GEOMETRIC_ANIMATION_G_LED_LIMIT;
	}

	geometric_animation_pos = 0;

}

#define GEOMETRIC_COLOR_R 0
#define GEOMETRIC_COLOR_G 0
#define GEOMETRIC_COLOR_B 48

static void draw_animation(uint8_t* buffer, uint8_t start, uint8_t pos) {
	uint8_t max = 0;
	if (pos < 3) max = pos + 1;
	else if (pos < 8) max = 4;
	else if (pos < 11) max = 11 - pos;
	else return;

	uint8_t offset = pos < 4? pos : (4 * pos - 9);

	for (uint8_t i = 0; i < max; i++) {
		buffer[(start + offset + i * 3) * 3 + 0] = GEOMETRIC_COLOR_B;
		buffer[(start + offset + i * 3) * 3 + 1] = GEOMETRIC_COLOR_R;
		buffer[(start + offset + i * 3) * 3 + 2] = GEOMETRIC_COLOR_G;
	}
}

static void geometric_animation_state(uint8_t *buffer) {
	if (geometric_animation_pos >= GEOMETRIC_ANIMATION_STEPS)
		return;

	if (g_led_counter[1] == 0) {

		g_led_counter[1] = GEOMETRIC_ANIMATION_G_LED_LIMIT;

		if (++geometric_animation_pos >= GEOMETRIC_ANIMATION_STEPS)
			return;
	}

	draw_animation(buffer, 0, geometric_animation_pos);
	draw_animation(buffer, 32, geometric_animation_pos - 4);
}

static void fastrgb_state(uint8_t* buffer) {
	for (uint8_t i=0; i<NUM_BUTTONS; i++) {
		buffer[i * 3 + 0] = g_fastrgb_state[i][2];
		buffer[i * 3 + 1] = g_fastrgb_state[i][0];
		buffer[i * 3 + 2] = g_fastrgb_state[i][1];
	}
}

void default_display_run(void)
{

	fastrgb_state(g_display_buffer);

	if (half_ms_counter >= 2000)
	{
		half_ms_counter = 0;
		one_second_counter += 1;
		if (one_second_counter >= 60)
		{
			sleep_minute_counter += 1;
			one_second_counter = 0;
		}
	}

	if (G_EE_SLEEP_TIME) {
		if (sleep_minute_counter == G_EE_SLEEP_TIME) {
			idle_init();
			sleep_minute_counter++;
		}
		if (sleep_minute_counter > G_EE_SLEEP_TIME) {
			idle_tick(g_display_buffer);
		}
		if (g_key_down) {
			one_second_counter = 0;
			sleep_minute_counter = 0;
		}
	}
	geometric_animation_state(g_display_buffer);
}

#define LAVENDER_GREEN_LIMIT 0x24
#define MF3D_UTILITY_BRIGHT_COLOR_LIMIT 0x80
#define MF3D_UTILITY_DIM_COLOR_LIMIT 0x27

void adjust_inactive_bank_leds_for_power(uint8_t* rgb)
{
	uint8_t id;
	if (!rgb[0])
	{
		if (!rgb[1])
		{
			if (!rgb[2]) {
				id = COLORID_OFF;
			}
			else {
				if (rgb[2] == default_color[COLORID_BLUE][2]) {return;}
				id = rgb[2] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_BLUE : COLORID_BLUE_DIM;
			}
		}
		else if (!rgb[2])
		{
			if (rgb[1] == default_color[COLORID_GREEN][1]) {return;}
			id = rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_GREEN : COLORID_GREEN_DIM;
		}
		else
		{
			if (rgb[1] == default_color[COLORID_CYAN][1]) {return;}
			id = rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_CYAN : COLORID_CYAN_DIM;
		}
	}
	else if (!rgb[1])
	{
		if (!rgb[2])
		{
			if (rgb[0] == default_color[COLORID_RED][0]) {return;}
			id = rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_RED : COLORID_RED_DIM;
		}
		else
		{
			if (rgb[0] == default_color[COLORID_PINK][0]) {return;}
			id = rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_PINK : COLORID_PINK_DIM;
		}
	}
	else if (!rgb[2])
	{

		if (rgb[0] < rgb[1])
		{
			if (rgb[1] == default_color[COLORID_CHARTREUSE][1]) {return;}
			id = rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_CHARTREUSE : COLORID_CHARTREUSE_DIM;
		}
		else if (rgb[0] >> 1 >= rgb[1])
		{
			if (rgb[0] == default_color[COLORID_ORANGE][0]) {return;}
			id = rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_ORANGE : COLORID_ORANGE_DIM;
		}
		else
		{
			if (rgb[0] == default_color[COLORID_YELLOW][0]) {return;}
			id = rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_YELLOW : COLORID_YELLOW_DIM;
		}
	}
	else
	{
		if (rgb[1] > LAVENDER_GREEN_LIMIT) {
			id = COLORID_WHITE;
		}
		else {

			if (rgb[2] == default_color[COLORID_LAVENDER][2]) {return;}
			id = rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_LAVENDER : COLORID_LAVENDER_DIM;
		}
	}
	rgb[0] = default_color[id][0];
	rgb[1] = default_color[id][1];
	rgb[2] = default_color[id][2];
}
void adjust_active_bank_leds_for_power(uint8_t* rgb)
{
	uint8_t id;
	if (!rgb[0])
	{
		if (!rgb[1])
		{
			if (!rgb[2]) {
				id = COLORID_OFF;
			}
			else {
				if (rgb[2] == default_color[COLORID_BLUE][2]) {return;}
				id = rgb[2] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_BLUE : COLORID_BLUE_DIM;
			}
		}
		else if (!rgb[2])
		{
			if (rgb[1] == default_color[COLORID_GREEN][1]) {return;}
			id = rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_GREEN : COLORID_GREEN_DIM;
		}
		else
		{
			if (rgb[1] == default_color[COLORID_CYAN][1]) {return;}
			id = rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_CYAN : COLORID_CYAN_DIM;
		}
	}
	else if (!rgb[1])
	{
		if (!rgb[2])
		{
			if (rgb[0] == default_color[COLORID_RED][0]) {return;}
			id = rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_RED : COLORID_RED_DIM;
		}
		else
		{
			if (rgb[0] == default_color[COLORID_PINK][0]) {return;}
			id = rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_PINK : COLORID_PINK_DIM;
		}
	}
	else if (!rgb[2])
	{

		if (rgb[0] < rgb[1])
		{
			if (rgb[1] == default_color[COLORID_CHARTREUSE][1]) {return;}
			id = rgb[1] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_CHARTREUSE : COLORID_CHARTREUSE_DIM;
		}
		else if (rgb[0] >> 1 >= rgb[1])
		{
			if (rgb[0] == default_color[COLORID_ORANGE][0]) {return;}
			id = rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_ORANGE : COLORID_ORANGE_DIM;
		}
		else
		{
			if (rgb[0] == default_color[COLORID_YELLOW][0]) {return;}
			id = rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_YELLOW : COLORID_YELLOW_DIM;
		}
	}
	else
	{
		if (rgb[1] > LAVENDER_GREEN_LIMIT) {
			id = COLORID_WHITE;
		}
		else {

			if (rgb[2] == default_color[COLORID_LAVENDER][2]) {return;}
			id = rgb[0] > MF3D_UTILITY_BRIGHT_COLOR_LIMIT ? COLORID_LAVENDER : COLORID_LAVENDER_DIM;
		}
	}
	rgb[0] = default_color[id][0];
	rgb[1] = default_color[id][1];
	rgb[2] = default_color[id][2];
}
