#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "led.h"

#include "eeprom.h"
#include "midi.h"

uint16_t g_led_counter[4];
uint16_t display_flash_counter;
uint16_t half_ms_counter;
uint8_t one_second_counter;
uint8_t sleep_minute_counter;

static uint16_t tick = 0;

ISR(TIMER1_OVF_vect)
{

    TCNT1 = 0;

    TCNT1 = 0xFFE0;

    for (uint8_t i=0; i<4; ++i) {
        if (g_led_counter[i] > 0) { --g_led_counter[i]; }
    }

	if(!midi_clock_enabled)
	{
		if(tick == 75)
		{
			tick = 1;
			display_flash_counter += 1;
		}
		else
		{
			tick += 1;
		}
	}

	half_ms_counter +=1;

    if (g_led_counter[3] == 0) {
        g_led_counter[3] = 16;
    }
}

void led_setup(void)
{
    DDRB = LED_CLOCK + LED_MOSI + LED_LATCH + LED_MODE;
    DDRC = LED_BLANK + LED_PWM;

    PRR0 &= ~_BV(PRTIM1);

    TCCR1A = 0;

    TCCR1B |= _BV(CS12);
    TCCR1B &= ~_BV(CS11);
    TCCR1B &= ~_BV(CS10);

    cli();

    TCNT1 = 0xFE40;

    TIMSK1 |= _BV(TOIE1);

    for (uint8_t i=0; i<4; ++i) {
        g_led_counter[i] = 0;
    }
}

void led_disable(void)
{

    TIMSK1 &= ~(_BV(TOIE1));
}

void led_enable(void)
{

	TIMSK1 |= _BV(TOIE1);
}

static void led_update_pixel_group0(const uint8_t *buffer);
static void led_update_pixel_group1(const uint8_t *buffer);
static void led_update_pixel_group2(const uint8_t *buffer);
static void led_update_pixel_group3(const uint8_t *buffer);

void led_set_state_dfu(void)
{
	const uint8_t indicator_states[48] = {
	48,0,0,    0,0,0,   48,0,0,   0,0,0,  0,0,0,  48,0,0, 0,0,0, 48,0,0,
	48,0,0,    0,0,0,   48,0,0,   0,0,0,  0,0,0,  48,0,0, 0,0,0, 48,0,0
	};
	DDRC |= LED_ASYNC_GROUP1;
	DDRB |= LED_ASYNC_GROUP0 | LED_ASYNC_GROUP2 | LED_ASYNC_GROUP3;
	cli();
	led_update_pixel_group0(indicator_states);
	led_update_pixel_group1(indicator_states);
	led_update_pixel_group2(indicator_states);
	led_update_pixel_group3(indicator_states);
	sei();

}

static void led_update_pixel_group0(const uint8_t *buffer)
{

	static uint32_t test_buffer = 0x00000000;
	uint32_t *single_led_buffer;

	for (uint8_t this_led=0; this_led <= 31; this_led++) {

		single_led_buffer = (uint32_t *)(buffer);
		test_buffer = *single_led_buffer;
		if (this_led & 0x01) {
			buffer = buffer + 3;
		}

		uint32_t this_mask = 0x800000;
		for (uint8_t this_bit=0; this_bit < 24; this_bit++) {

			if (test_buffer & this_mask) {
				PORTB |= LED_ASYNC_GROUP0;

				asm("nop");
				asm("nop");
				asm("nop");
				asm("nop");
				PORTB &= ~LED_ASYNC_GROUP0;

			}
			else {

				PORTB |= LED_ASYNC_GROUP0;

				PORTB &= ~LED_ASYNC_GROUP0;

			}
			this_mask >>= 1;
		}
	}

	return;
}

static void led_update_pixel_group1(const uint8_t *buffer)
{

	static uint32_t test_buffer = 0x00000000;
	uint32_t *single_led_buffer;

	for (uint8_t this_led=0; this_led <= 31; this_led++) {

		single_led_buffer = (uint32_t *)(buffer);
		test_buffer = *single_led_buffer;
		if (this_led & 0x01) {
			buffer = buffer + 3;
		}

		uint32_t this_mask = 0x800000;
		for (uint8_t this_bit=0; this_bit < 24; this_bit++) {

			if (test_buffer & this_mask) {
				PORTC |= LED_ASYNC_GROUP1;

				asm("nop");
				asm("nop");
				asm("nop");
				asm("nop");
				PORTC &= ~LED_ASYNC_GROUP1;

			}
			else {

				PORTC |= LED_ASYNC_GROUP1;

				PORTC &= ~LED_ASYNC_GROUP1;

			}
			this_mask >>= 1;
		}
	}

	return;
}

static void led_update_pixel_group2(const uint8_t *buffer)
{
	static uint32_t test_buffer = 0x00000000;
	uint32_t *single_led_buffer;

	for (uint8_t this_led=0; this_led <= 31; this_led++) {

		single_led_buffer = (uint32_t *)(buffer);
		test_buffer = *single_led_buffer;
		if (this_led & 0x01) {
			buffer = buffer + 3;
		}

		uint32_t this_mask = 0x800000;
		for (uint8_t this_bit=0; this_bit < 24; this_bit++) {

			if (test_buffer & this_mask) {
				PORTB |= LED_ASYNC_GROUP2;

				asm("nop");
				asm("nop");
				asm("nop");
				asm("nop");
				PORTB &= ~LED_ASYNC_GROUP2;

			}
			else {

				PORTB |= LED_ASYNC_GROUP2;

				PORTB &= ~LED_ASYNC_GROUP2;

			}
			this_mask >>= 1;
		}
	}

	return;
}

static void led_update_pixel_group3(const uint8_t *buffer)
{
	static uint32_t test_buffer = 0x00000000;
	uint32_t *single_led_buffer;

	for (uint8_t this_led=0; this_led <= 31; this_led++) {

		single_led_buffer = (uint32_t *)(buffer);
		test_buffer = *single_led_buffer;
		if (this_led & 0x01) {
			buffer = buffer + 3;
		}

		uint32_t this_mask = 0x800000;
		for (uint8_t this_bit=0; this_bit < 24; this_bit++) {

			if (test_buffer & this_mask) {
				PORTB |= LED_ASYNC_GROUP3;

				asm("nop");
				asm("nop");
				asm("nop");
				asm("nop");
				PORTB &= ~LED_ASYNC_GROUP3;

			}
			else {

				PORTB |= LED_ASYNC_GROUP3;

				PORTB &= ~LED_ASYNC_GROUP3;

			}
			this_mask >>= 1;
		}
	}

	return;
}

#define LED_OUTPUT_BRIGHTNESS_DEFAULT 180U
#define LED_OUTPUT_BRIGHTNESS_MAX 180U
#define LED_POWER_LIMIT_COLOR_SUM 6200UL
#define LED_POWER_LIMIT_NUMERATOR16 ((uint16_t)(LED_POWER_LIMIT_COLOR_SUM * 8UL))

static uint8_t g_led_output_brightness = LED_OUTPUT_BRIGHTNESS_DEFAULT;
static uint16_t g_led_power_limit_threshold = (uint16_t)((LED_POWER_LIMIT_COLOR_SUM * 32UL) / LED_OUTPUT_BRIGHTNESS_DEFAULT);
static uint8_t parallel_masks[768];

void led_brightness_set(uint8_t brightness)
{
	if (brightness == 0U) brightness = 1U;
	if (brightness > LED_OUTPUT_BRIGHTNESS_MAX) brightness = LED_OUTPUT_BRIGHTNESS_MAX;

	g_led_output_brightness = brightness;

	uint32_t threshold = (LED_POWER_LIMIT_COLOR_SUM * 32UL) / brightness;
	if (threshold > 65535UL) threshold = 65535UL;
	g_led_power_limit_threshold = (uint16_t)threshold;
}

uint8_t led_brightness_get(void)
{
	return g_led_output_brightness;
}

void led_brightness_setup(void)
{
	uint8_t brightness = LED_OUTPUT_BRIGHTNESS_DEFAULT;

	if (eeprom_read(EE_LED_BRIGHTNESS_MAGIC) == EE_LED_BRIGHTNESS_MAGIC_VALUE) {
		uint8_t stored = eeprom_read(EE_LED_BRIGHTNESS);
		if (stored >= 1U && stored <= LED_OUTPUT_BRIGHTNESS_MAX) {
			brightness = stored;
		}
	} else {
		eeprom_update(EE_LED_BRIGHTNESS, LED_OUTPUT_BRIGHTNESS_DEFAULT);
		eeprom_update(EE_LED_BRIGHTNESS_MAGIC, EE_LED_BRIGHTNESS_MAGIC_VALUE);
	}

	led_brightness_set(brightness);
}

static uint8_t led_compute_effective_brightness(const uint8_t *buffer)
{
	uint16_t total_internal = 0;
	uint8_t brightness = g_led_output_brightness;

	for (uint8_t i = 0; i < 192; i++) {
		uint8_t value = buffer[i];
		if (value > 64U) value = 64U;
		total_internal += value;
	}

	if (total_internal <= g_led_power_limit_threshold) return brightness;

	uint16_t divisor = (uint16_t)((total_internal + 2U) >> 2);
	if (divisor == 0U) return brightness;

	uint16_t limited = (uint16_t)(LED_POWER_LIMIT_NUMERATOR16 / divisor);
	if (limited >= brightness) return brightness;
	if (limited == 0U) return 1U;
	return (uint8_t)limited;
}

static inline uint8_t led_scale_internal(uint8_t value, uint8_t brightness)
{
	if (value > 64U) value = 64U;
	return (uint8_t)((((uint16_t)value * brightness) + 32U) >> 6);
}

static inline uint8_t led_get_grb_component(const uint8_t *buffer, uint8_t key, uint8_t component, uint8_t brightness)
{
	uint8_t source_component;
	if (component == 0) source_component = 2;
	else if (component == 1) source_component = 1;
	else source_component = 0;

	return led_scale_internal(buffer[(uint16_t)key * 3U + source_component], brightness);
}

static void led_fill_parallel_masks(const uint8_t *buffer, uint8_t portb_base, uint8_t brightness)
{
	uint16_t out = 0;

	for (uint8_t physical = 0; physical < 32; physical++) {
		uint8_t key0 = (uint8_t)(physical >> 1);
		uint8_t key2 = (uint8_t)(32 + (physical >> 1));
		uint8_t key3 = (uint8_t)(48 + (physical >> 1));

		for (uint8_t component = 0; component < 3; component++) {
			uint8_t v0 = led_get_grb_component(buffer, key0, component, brightness);
			uint8_t v2 = led_get_grb_component(buffer, key2, component, brightness);
			uint8_t v3 = led_get_grb_component(buffer, key3, component, brightness);

			for (uint8_t bit = 0x80; bit != 0; bit >>= 1) {
				uint8_t mask = portb_base;
				if (v0 & bit) mask |= LED_ASYNC_GROUP0;
				if (v2 & bit) mask |= LED_ASYNC_GROUP2;
				if (v3 & bit) mask |= LED_ASYNC_GROUP3;
				parallel_masks[out++] = mask;
			}
		}
	}
}

static inline __attribute__((always_inline)) void led_send_fourway_byte(
	const uint8_t *data,
	uint8_t pc_value,
	uint8_t portb_base,
	uint8_t portc_base)
{
	uint8_t count = 8;
	uint8_t mid;
	uint8_t high_b = (uint8_t)(portb_base | LED_ASYNC_GROUP0 | LED_ASYNC_GROUP2 | LED_ASYNC_GROUP3);
	uint8_t high_c = (uint8_t)(portc_base | LED_ASYNC_GROUP1);
	const uint8_t *ptr = data;

	asm volatile(
		"1:\n\t"
		"ld %[mid], Z+\n\t"
		"out 0x05, %[high_b]\n\t"
		"out 0x08, %[high_c]\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"out 0x05, %[mid]\n\t"
		"sbrs %[pc], 7\n\t"
		"out 0x08, %[low_c]\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"out 0x05, %[low_b]\n\t"
		"out 0x08, %[low_c]\n\t"
		"lsl %[pc]\n\t"
		"dec %[count]\n\t"
		"brne 1b\n\t"
		: [ptr] "+z" (ptr),
		  [pc] "+r" (pc_value),
		  [count] "+r" (count),
		  [mid] "=&r" (mid)
		: [high_b] "r" (high_b),
		  [high_c] "r" (high_c),
		  [low_b] "r" (portb_base),
		  [low_c] "r" (portc_base)
		: "memory"
	);
}

void led_update_pixels(uint8_t *buffer)
{
	DDRC |= LED_ASYNC_GROUP1;
	DDRB |= LED_ASYNC_GROUP0 | LED_ASYNC_GROUP2 | LED_ASYNC_GROUP3;

	uint8_t sreg = SREG;
	uint8_t portb_base = (uint8_t)(PORTB & (uint8_t)~(LED_ASYNC_GROUP0 | LED_ASYNC_GROUP2 | LED_ASYNC_GROUP3));
	uint8_t portc_base = (uint8_t)(PORTC & (uint8_t)~LED_ASYNC_GROUP1);

	uint8_t effective_brightness = led_compute_effective_brightness(buffer);
	led_fill_parallel_masks(buffer, portb_base, effective_brightness);

	cli();

	uint16_t offset = 0;
	for (uint8_t physical = 0; physical < 32; physical++) {
		uint8_t key1 = (uint8_t)(16 + (physical >> 1));

		for (uint8_t component = 0; component < 3; component++) {
			uint8_t v1 = led_get_grb_component(buffer, key1, component, effective_brightness);
			led_send_fourway_byte(
				&parallel_masks[offset],
				v1,
				portb_base,
				portc_base
			);
			offset += 8;
		}
	}

	PORTB = portb_base;
	PORTC = portc_base;
	SREG = sreg;
	_delay_us(60);
}
