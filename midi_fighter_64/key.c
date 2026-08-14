#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "key.h"
#include "constants.h"


static uint64_t g_key_debounce_buffer[DEBOUNCE_BUFFER_SIZE];
uint64_t g_key_state = 0;
uint64_t g_key_prev_state = 0;
uint64_t g_key_up = 0;
uint64_t g_key_down= 0;

volatile uint32_t system_time_ms = 0;
#define TIMER_TIMEOUT_1MS	0xD0

void key_setup(void)
{

    DDRD |= KEY_CLOCK;
    DDRD |= KEY_LATCH;

    DDRC &= ~KEY_BIT;
    PORTC |= KEY_BIT;

    PORTD |= KEY_CLOCK;
    PORTD |= KEY_LATCH;

    memset(g_key_debounce_buffer, 0, sizeof(g_key_debounce_buffer));

    TCCR0B |= _BV(CS02);
    TCCR0B &= ~_BV(CS01);
    TCCR0B &= ~_BV(CS00);

    TCNT0 = TIMER_TIMEOUT_1MS;

    TIMSK0 |= _BV(TOIE0);

    sei();

    g_key_state = 0;
    g_key_prev_state = 0;
    g_key_up = 0;
    g_key_down = 0;
}

ISR(TIMER0_OVF_vect)
{

    static uint8_t buffer_pos = 0;

    TCNT0 = TIMER_TIMEOUT_1MS;

	PORTD |= KEY_LATCH;
	PORTD &= ~KEY_LATCH;

	uint64_t value = 0;
	uint64_t bit = 0x1;
	for (uint8_t i=0; i<64; i++) {
        PORTD &= ~KEY_CLOCK;
        value |= (PINC & KEY_BIT) ? 0 : bit;
        bit <<= 1;
        PORTD |= KEY_CLOCK;
	}
    g_key_debounce_buffer[buffer_pos] = ~value;
    buffer_pos = (buffer_pos + 1) % DEBOUNCE_BUFFER_SIZE;

	system_time_ms += 1;
  	return;
}

void key_read(void)
{

    g_key_state = 0xffffffffffffffff;
    for(uint8_t i=0; i<DEBOUNCE_BUFFER_SIZE; ++i) {
        g_key_state &= g_key_debounce_buffer[i];
    }
}

void key_calc(void)
{

	g_key_down = (g_key_prev_state ^ g_key_state) & g_key_state;

    g_key_up = (g_key_prev_state ^ g_key_state) & g_key_prev_state;

    g_key_prev_state = g_key_state;
}
