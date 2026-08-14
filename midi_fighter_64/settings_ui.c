#include <stdint.h>
#include <stdbool.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "native_usb.h"

#include "settings_ui.h"
#include "constants.h"
#include "key.h"
#include "led.h"
#include "midi.h"
#include "display.h"
#include "fastrgb.h"
#include "rotation.h"
#include "eeprom.h"

#define SETTINGS_CONFIRM_MS 120U

#define SR_ROT_0_A       23U
#define SR_ROT_0_B       52U
#define SR_ROT_90_A      45U
#define SR_ROT_90_B      49U
#define SR_ROT_180_A     11U
#define SR_ROT_180_B     40U
#define SR_ROT_270_A     14U
#define SR_ROT_270_B     18U

#define SR_EXIT_ROT_0    35U
#define SR_EXIT_ROT_90    0U
#define SR_EXIT_ROT_180  28U
#define SR_EXIT_ROT_270  63U

#define SETTINGS_GREEN_DIM     18U
#define SETTINGS_GREEN_BRIGHT  64U
#define SETTINGS_RED_BRIGHT    64U
#define SETTINGS_WHITE_DIM     18U
#define SETTINGS_WHITE_BRIGHT  64U

#define SETTINGS_BRIGHTNESS_STEPS 8U

static const uint8_t settings_brightness_values[SETTINGS_BRIGHTNESS_STEPS] PROGMEM = {
    40U, 60U, 80U, 100U, 120U, 140U, 160U, 180U
};

static const uint8_t settings_brightness_sr[4][SETTINGS_BRIGHTNESS_STEPS] PROGMEM = {
    { 4U, 5U, 6U, 7U, 36U, 37U, 38U, 39U },
    { 29U, 25U, 21U, 17U, 13U, 9U, 5U, 1U },
    { 59U, 58U, 57U, 56U, 27U, 26U, 25U, 24U },
    { 34U, 38U, 42U, 46U, 50U, 54U, 58U, 62U }
};

static uint64_t settings_sr_mask(uint8_t address)
{
    return ((uint64_t)1U << address);
}

static uint8_t settings_exit_address(uint8_t rotation)
{
    switch (rotation & 0x03U) {
        case ROTATION_0:   return SR_EXIT_ROT_0;
        case ROTATION_90:  return SR_EXIT_ROT_90;
        case ROTATION_180: return SR_EXIT_ROT_180;
        default:           return SR_EXIT_ROT_270;
    }
}

static void settings_set_sr_led(uint8_t address, uint8_t r, uint8_t g, uint8_t b)
{
    g_fastrgb_state[address][0] = r;
    g_fastrgb_state[address][1] = g;
    g_fastrgb_state[address][2] = b;
}

static void settings_copy_to_display(void)
{
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        g_display_buffer[(uint16_t)i * 3U + 0U] = g_fastrgb_state[i][2];
        g_display_buffer[(uint16_t)i * 3U + 1U] = g_fastrgb_state[i][0];
        g_display_buffer[(uint16_t)i * 3U + 2U] = g_fastrgb_state[i][1];
    }
}

static void settings_show_pair(uint8_t a, uint8_t b, bool selected)
{
    uint8_t green = selected ? SETTINGS_GREEN_BRIGHT : SETTINGS_GREEN_DIM;
    settings_set_sr_led(a, 0U, green, 0U);
    settings_set_sr_led(b, 0U, green, 0U);
}

static uint8_t settings_brightness_address(uint8_t rotation, uint8_t index)
{
    return pgm_read_byte(&settings_brightness_sr[rotation & 0x03U][index]);
}

static uint8_t settings_brightness_value(uint8_t index)
{
    return pgm_read_byte(&settings_brightness_values[index]);
}

static uint8_t settings_brightness_index(uint8_t brightness)
{
    uint8_t best = 0U;
    uint8_t best_diff = 255U;

    for (uint8_t i = 0; i < SETTINGS_BRIGHTNESS_STEPS; i++) {
        uint8_t value = settings_brightness_value(i);
        uint8_t diff = (brightness > value) ? (uint8_t)(brightness - value) : (uint8_t)(value - brightness);
        if (diff < best_diff) {
            best_diff = diff;
            best = i;
        }
    }

    return best;
}

static void settings_show_brightness(uint8_t rotation)
{
    uint8_t selected = settings_brightness_index(led_brightness_get());

    for (uint8_t i = 0; i < SETTINGS_BRIGHTNESS_STEPS; i++) {
        uint8_t white = (i == selected) ? SETTINGS_WHITE_BRIGHT : SETTINGS_WHITE_DIM;
        settings_set_sr_led(settings_brightness_address(rotation, i), white, white, white);
    }
}

static void settings_render(uint8_t selected_rotation)
{
    fastrgb_clear();

    settings_show_pair(SR_ROT_0_A, SR_ROT_0_B, selected_rotation == ROTATION_0);
    settings_show_pair(SR_ROT_90_A, SR_ROT_90_B, selected_rotation == ROTATION_90);
    settings_show_pair(SR_ROT_180_A, SR_ROT_180_B, selected_rotation == ROTATION_180);
    settings_show_pair(SR_ROT_270_A, SR_ROT_270_B, selected_rotation == ROTATION_270);

    settings_show_brightness(selected_rotation);
    settings_set_sr_led(settings_exit_address(selected_rotation), SETTINGS_RED_BRIGHT, 0U, 0U);

    settings_copy_to_display();
    led_update_pixels(g_display_buffer);
}

static void settings_discard_midi(void)
{
    if (USB_DeviceState != DEVICE_STATE_Configured) return;

    MIDI_EventPacket_t input_event;
    uint8_t count = 0;
    while (count < 64U && MIDI_Device_ReceiveEventPacket(&input_event)) {
        count++;
    }
}

static void settings_tick(void)
{
    settings_discard_midi();
    wdt_reset();
}

static void settings_wait_ms(uint16_t ms)
{
    while (ms--) {
        settings_tick();
        _delay_ms(1);
    }
}

static int8_t settings_rotation_from_down(uint64_t down)
{
    if (down & (settings_sr_mask(SR_ROT_0_A) | settings_sr_mask(SR_ROT_0_B))) return ROTATION_0;
    if (down & (settings_sr_mask(SR_ROT_90_A) | settings_sr_mask(SR_ROT_90_B))) return ROTATION_90;
    if (down & (settings_sr_mask(SR_ROT_180_A) | settings_sr_mask(SR_ROT_180_B))) return ROTATION_180;
    if (down & (settings_sr_mask(SR_ROT_270_A) | settings_sr_mask(SR_ROT_270_B))) return ROTATION_270;
    return -1;
}

static int8_t settings_brightness_from_down(uint8_t rotation, uint64_t down)
{
    for (uint8_t i = 0; i < SETTINGS_BRIGHTNESS_STEPS; i++) {
        uint8_t address = settings_brightness_address(rotation, i);
        if (down & settings_sr_mask(address)) return (int8_t)i;
    }
    return -1;
}

static void settings_wait_exit_release(uint8_t exit_address)
{
    for (;;) {
        key_read();
        if ((g_key_state & settings_sr_mask(exit_address)) == 0) return;
        settings_tick();
        _delay_ms(1);
    }
}

static void settings_reboot_to_normal(uint8_t current_rotation)
{
    uint8_t exit_address = settings_exit_address(current_rotation);
    settings_wait_exit_release(exit_address);

    fastrgb_clear();
    settings_copy_to_display();
    led_update_pixels(g_display_buffer);
    settings_wait_ms(30U);

    wdt_enable(WDTO_15MS);
    for (;;) {
    }
}

bool settings_ui_boot_requested(void)
{
    uint8_t trigger_address = rotation_logical_to_physical(SR_EXIT_ROT_0);
    return (g_key_state & settings_sr_mask(trigger_address)) != 0;
}

void settings_ui_run(void)
{
    uint8_t current = rotation_get();
    led_enable();
    settings_render(current);

    g_key_down = 0;
    g_key_up = 0;
    g_key_prev_state = g_key_state;

    for (;;) {
        key_read();
        key_calc();

        uint8_t exit_address = settings_exit_address(current);
        if (g_key_down & settings_sr_mask(exit_address)) {
            settings_reboot_to_normal(current);
        }

        int8_t brightness_step = settings_brightness_from_down(current, g_key_down);
        if (brightness_step >= 0) {
            uint8_t brightness = settings_brightness_value((uint8_t)brightness_step);
            led_brightness_set(brightness);
            eeprom_update(EE_LED_BRIGHTNESS, brightness);
            eeprom_update(EE_LED_BRIGHTNESS_MAGIC, EE_LED_BRIGHTNESS_MAGIC_VALUE);
            settings_render(current);
            g_key_down = 0;
            g_key_up = 0;
            g_key_prev_state = g_key_state;
        }

        int8_t selected = settings_rotation_from_down(g_key_down);
        if (selected >= 0) {
            uint8_t value = (uint8_t)selected;
            if (value != current) {
                rotation_set(value);
                eeprom_update(EE_ROTATION, value);
                eeprom_update(EE_ROTATION_MAGIC, 0xA5);
                current = value;
            }

            settings_render(current);
            settings_wait_ms(SETTINGS_CONFIRM_MS);
            g_key_down = 0;
            g_key_up = 0;
            g_key_prev_state = g_key_state;
        }

        settings_tick();
        _delay_ms(1);
    }
}
