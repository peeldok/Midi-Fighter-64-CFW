#ifndef _KEY_H_INCLUDED
#define _KEY_H_INCLUDED

#include <stdint.h>
#include "constants.h"

extern uint64_t g_key_state;
extern uint64_t g_key_prev_state;
extern uint64_t g_key_up;
extern uint64_t g_key_down;

extern volatile uint32_t system_time_ms;

void key_setup(void);
void key_read(void);
void key_calc(void);

#endif
