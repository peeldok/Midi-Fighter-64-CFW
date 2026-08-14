#ifndef _idle_H_INCLUDED
#define _idle_H_INCLUDED

#include <stdint.h>

void idle_init(void);
void idle_tick(uint8_t* buffer);

#endif
