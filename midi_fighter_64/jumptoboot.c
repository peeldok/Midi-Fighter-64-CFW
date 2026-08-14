#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#include "native_usb.h"

uint32_t Boot_Key __attribute__((section(".noinit")));

#define MAGIC_BOOT_KEY 0xDC42ACCAUL

void Bootloader_Jump_Check(void) __attribute__((section(".init3"), naked, used));
void Bootloader_Jump_Check(void)
{
    if ((MCUSR & (1U << WDRF)) && (Boot_Key == MAGIC_BOOT_KEY)) {
        Boot_Key = 0;
        __asm__ __volatile__("jmp 0x7000");
    }
}

void Jump_To_Bootloader(void)
{
    USB_Disable();
    cli();

    for (uint8_t i = 0; i < 128; i++) {
        _delay_ms(16);
    }

    Boot_Key = MAGIC_BOOT_KEY;
    wdt_enable(WDTO_250MS);
    for (;;) {
    }
}
