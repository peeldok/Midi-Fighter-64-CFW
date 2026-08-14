#ifndef _LED_H_INCLUDED
#define _LED_H_INCLUDED

#include <stdint.h>

#include "constants.h"

extern uint16_t g_led_counter[4];
extern uint16_t display_flash_counter;
extern uint16_t half_ms_counter;
extern uint8_t one_second_counter;
extern uint8_t sleep_minute_counter;

void led_setup(void);
void led_disable(void);
void led_enable(void);
void led_update_pixels(uint8_t *buffer);
void led_brightness_setup(void);
void led_brightness_set(uint8_t brightness);
uint8_t led_brightness_get(void);

void led_set_state_dfu(void);

#endif
