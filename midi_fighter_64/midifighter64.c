#include <stdbool.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "native_usb.h"
#include "key.h"
#include "led.h"
#include "midi.h"
#include "eeprom.h"
#include "display.h"
#include "constants.h"
#include "fastrgb.h"
#include "jumptoboot.h"
#include "sysex.h"
#include "config.h"
#include "rotation.h"
#include "boot_animation.h"
#include "settings_ui.h"

static bool watchdog_flag = false;

void EVENT_USB_Device_Connect(void)
{
    led_enable();
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
    led_enable();
    wdt_enable(WDTO_2S);
}

static void update_note_off_feedback_delay(void)
{
    uint8_t now = (system_time_ms & 0x7F) * 2;
    for (uint8_t key = 0; key < NUM_BUTTONS; key++) {
        if (!(g_midi_note_off_counter[key] & 0x80)) continue;
        uint8_t note_off_time = (g_midi_note_off_counter[key] & 0x7F) * 2;
        if ((uint8_t)(now - note_off_time) >= ((uint8_t)NOTE_OFF_FEEDBACK_DELAY_LIMIT * 2)) {
            fastrgb_single_unsafe(key, 0, 0, 0);
            g_midi_note_off_counter[key] = 0;
        }
    }
}

static void Midifighter_GetIncomingUsbMidiMessages(void)
{
    MIDI_EventPacket_t input_event;
    uint16_t fail_count = 0;
    uint16_t packet_count = 0;

    while (packet_count < USB_RX_PACKET_LIMIT) {
        if (!MIDI_Device_ReceiveEventPacket(&input_event)) {
            if (++fail_count >= USB_RX_FAIL_LIMIT) break;
            wdt_reset();
            continue;
        }

        packet_count++;
        fail_count = 0;

        uint8_t cable = (uint8_t)((input_event.Event >> 4) & 0x0F);
        if (cable > 1U) continue;

        switch (input_event.Event & 0x0F) {
            case 0xF:
                switch (input_event.Data1) {
                    case 0xF8:
                        midi_clock_enabled = true;
                        midi_clock();
                        break;
                    case 0xFA:
                        midi_clock_enabled = true;
                        break;
                    case 0xFC:
                        midi_clock_enabled = false;
                        break;
                    default:
                        break;
                }
                break;

            case 0x9: {
                uint8_t channel = input_event.Data1 & 0x0F;
                if (channel > MIDI_CH4_CUSTOM_CHANNEL) break;
                uint8_t key = input_event.Data2 - MIDI_BASENOTE;
                if (key >= NUM_BUTTONS) break;
                if (channel == MIDI_CH4_CUSTOM_CHANNEL) {
                    fastrgb_ch4_single(key, input_event.Data3);
                } else {
                    fastrgb_palette_single(key, channel, input_event.Data3);
                }
                g_midi_note_off_counter[key] = 0;
                break;
            }

            case 0x8: {
                uint8_t channel = input_event.Data1 & 0x0F;
                if (channel > MIDI_CH4_CUSTOM_CHANNEL) break;
                uint8_t key = input_event.Data2 - MIDI_BASENOTE;
                if (key >= NUM_BUTTONS) break;
                g_midi_note_off_counter[key] = (system_time_ms & 0x7F) | 0x80;
                break;
            }

            case 0x4:
                midi_set_sysex_response_cable(cable);
                sysex_handle_3sc(&input_event);
                break;
            case 0x5:
                midi_set_sysex_response_cable(cable);
                sysex_handle_1e(&input_event);
                break;
            case 0x6:
                midi_set_sysex_response_cable(cable);
                sysex_handle_2e(&input_event);
                break;
            case 0x7:
                midi_set_sysex_response_cable(cable);
                sysex_handle_3e(&input_event);
                break;
            default:
                break;
        }
    }
}

static void Midifighter_Task(void)
{
    if (USB_DeviceState != DEVICE_STATE_Configured) return;

    Midifighter_GetIncomingUsbMidiMessages();
    key_read();
    key_calc();

    if (g_key_down) sleep_minute_counter = 0;

    uint64_t key_bit = 1;
    for (uint8_t key = 0; key < NUM_BUTTONS; key++, key_bit <<= 1) {
        if (g_key_down & key_bit) {
            uint8_t note = midi_64_key_to_note(key);
            if (G_EE_MIDI_OUTPUT_MODE < MIDI_OUTPUT_MODE_CCS_ONLY) {
                midi_stream_note_ch(G_EE_MIDI_CHANNEL, note, true);
            }
            if (G_EE_MIDI_OUTPUT_MODE > MIDI_OUTPUT_MODE_NOTES_ONLY) {
                midi_stream_raw_cc(G_EE_MIDI_CHANNEL, note, 127);
            }
        }

        if (g_key_up & key_bit) {
            uint8_t note = midi_64_key_to_note(key);
            if (G_EE_MIDI_OUTPUT_MODE < MIDI_OUTPUT_MODE_CCS_ONLY) {
                midi_stream_note_ch(G_EE_MIDI_CHANNEL, note, false);
            }
            if (G_EE_MIDI_OUTPUT_MODE > MIDI_OUTPUT_MODE_NOTES_ONLY) {
                midi_stream_raw_cc(G_EE_MIDI_CHANNEL, note, 0);
            }
        }
    }

    MIDI_Device_Flush();
    update_note_off_feedback_delay();
    default_display_run();
    led_update_pixels(g_display_buffer);
    watchdog_flag = true;
}

int main(void)
{
    MCUSR &= ~(1 << WDRF);
    wdt_disable();
    clock_prescale_set(clock_div_1);

    MCUCR = (1 << JTD);
    MCUCR = (1 << JTD);

    eeprom_setup();
    fastrgb_ch4_setup();
    rotation_setup();
    led_brightness_setup();
    led_setup();
    led_disable();
    key_setup();
    fastrgb_clear();
    config_setup();

    _delay_ms(20);
    key_read();
    key_calc();

    uint8_t bootloader_trigger_sr = rotation_logical_to_physical(0U);
    if (g_key_state & ((uint64_t)1U << bootloader_trigger_sr)) {
        led_set_state_dfu();
        Jump_To_Bootloader();
        for (;;) {
        }
    }

    bool settings_ui_requested = settings_ui_boot_requested();

    USB_Init();
    sei();

    if (settings_ui_requested) settings_ui_run();
    boot_animation_play();

    half_ms_counter = 0;
    one_second_counter = 0;
    sleep_minute_counter = 0;

    for (;;) {
        Midifighter_Task();
        if (watchdog_flag) wdt_reset();
    }
}
