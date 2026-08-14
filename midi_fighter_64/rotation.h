#ifndef _ROTATION_H_INCLUDED
#define _ROTATION_H_INCLUDED

#include <stdint.h>

#define ROTATION_0   0
#define ROTATION_90  1
#define ROTATION_180 2
#define ROTATION_270 3

#define SYSEX_COMMAND_ROTATION 0x08
#define ROTATION_QUERY 0x7F

void rotation_setup(void);
void rotation_set(uint8_t rotation);
uint8_t rotation_get(void);
uint8_t rotation_logical_to_physical(uint8_t key);
uint8_t rotation_physical_to_logical(uint8_t key);

#endif
