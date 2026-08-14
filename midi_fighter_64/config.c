#include <util/delay.h>
#include <avr/wdt.h>

#include "config.h"
#include "sysex.h"
#include "eeprom.h"
#include "jumptoboot.h"
#include "led.h"
#include "display.h"
#include "midi.h"

#define SYSEX_COMMAND_PUSH_CONF    0x1
#define SYSEX_COMMAND_PULL_CONF    0x2
#define SYSEX_COMMAND_SYSTEM       0x3
#define SYSEX_COMMAND_BULK_XFER    0x4

#define TV_TABLE_SIZE 24
typedef union {
    struct {

        uint8_t midiChannel;
        uint8_t midiVelocity;
        uint8_t keypressLeds;
        uint8_t fourBanksMode;
        uint8_t expansionDigital;
        uint8_t expansionAnalog;
        uint8_t autoUpdate;
        uint8_t softwareMode;
        uint8_t combos;
        uint8_t multiplexer;

        uint8_t animation;

		uint8_t rotation;
        uint8_t tilt;
        uint8_t tiltMode;
		uint8_t tiltSens;
		uint8_t pitchSens;
		uint8_t tiltRange;
		uint8_t pitchRange;
		uint8_t tiltDead;
		uint8_t pitchDead;
		uint8_t tiltAxis;
		uint8_t pickSens;
		uint8_t sleepTime;
		uint8_t sideBank;

    };
    uint8_t bytes[TV_TABLE_SIZE];
} tvtable_t;

static void send_config_data(void);

static void tv_table_decode(tvtable_t* table, uint8_t* buffer, uint16_t size)
{
    uint16_t idx = 0;
    uint8_t tag;
    while (idx < size - 1) {
        tag = buffer[idx++];
        if (tag < TV_TABLE_SIZE) {
            table->bytes[tag] = buffer[idx];
        }
        ++idx;
    }
}

static void sysExCmdPushConfig (uint16_t length, uint8_t* buffer)
{

	wdt_disable();

    tvtable_t config = {{0}};
    tv_table_decode(&config, buffer, length);

    eeprom_write(EE_MIDI_CHANNEL, config.midiChannel - 1);
    eeprom_write(EE_MIDI_VELOCITY, config.midiVelocity);
	eeprom_write(EE_FOUR_BANKS_MODE, config.fourBanksMode);
	eeprom_write(EE_MIDI_OUTPUT_MODE, config.softwareMode);
	eeprom_write(EE_COMBOS_ENABLE, config.combos);
	eeprom_write(EE_ANIMATIONS, config.animation);
	eeprom_write(EE_TILT_MASK, (config.tilt << 4) | (config.rotation));
	eeprom_write(EE_TILT_MODE, config.tiltMode + 1);
 	eeprom_write(EE_TILT_SENSITIVITY, config.tiltSens);
 	eeprom_write(EE_TILT_RANGE, config.tiltRange);
 	eeprom_write(EE_PITCH_RANGE, config.pitchRange);
 	eeprom_write(EE_TILT_DEADZONE, config.tiltDead);
 	eeprom_write(EE_TILT_AXIS, config.tiltAxis);
	eeprom_write(EE_PICK_SENSITIVITY, config.pickSens);
	eeprom_write(EE_SLEEP_TIME, config.sleepTime);
	eeprom_write(EE_SIDE_BANK, config.sideBank);

	start_geometric_animation();

    send_config_data();

	eeprom_setup();

	wdt_enable(WDTO_2S);
}

static void send_config_data(void)
{
    uint8_t payload[] = {0xf0, 0x00, MANUFACTURER_ID >> 8, MANUFACTURER_ID & 0x7f,
                                SYSEX_COMMAND_PULL_CONF,
                                0x1,
                                0 , eeprom_read(EE_MIDI_CHANNEL) + 1,
                                1 , eeprom_read(EE_MIDI_VELOCITY),
                                3 , eeprom_read(EE_FOUR_BANKS_MODE),
                                7 , eeprom_read(EE_MIDI_OUTPUT_MODE),
                                8 , eeprom_read(EE_COMBOS_ENABLE),
                                10, eeprom_read(EE_ANIMATIONS),
                                11, (eeprom_read(EE_TILT_MASK)) & 0x3,
                                12, (eeprom_read(EE_TILT_MASK) >> 4) & 0xf,
                                13, eeprom_read(EE_TILT_MODE) - 1,
                                14, eeprom_read(EE_TILT_SENSITIVITY),
                                15, eeprom_read(EE_PITCH_SENSITIVITY),
                                16, eeprom_read(EE_TILT_RANGE),
                                17, eeprom_read(EE_PITCH_RANGE),
                                18, eeprom_read(EE_TILT_DEADZONE),
                                19, eeprom_read(EE_PITCH_DEADZONE),
                                20, eeprom_read(EE_TILT_AXIS),
                                21, eeprom_read(EE_PICK_SENSITIVITY),
                                22, eeprom_read(EE_SLEEP_TIME),
                                23, eeprom_read(EE_SIDE_BANK),
                                0xf7};
    midi_stream_sysex( sizeof(payload), payload);
}

static void sysExCmdPullConfig (uint16_t length, uint8_t* buffer)
{

    if (length > 0 && *buffer == 0x0) {

        wdt_disable();

        send_config_data();

        wdt_enable(WDTO_2S);
    }
}

static void sysExCmdSystem (uint16_t length, uint8_t* buffer)
{
    if (length == 0) return;

    switch (*buffer) {
    case 0:
        {

        }
        break;
    case 1:
        {
			wdt_disable();

			led_set_state_dfu();
            Jump_To_Bootloader();
        }
        break;
    case 2:
        {
			wdt_disable();

            eeprom_factory_reset();

            _delay_ms(300);

			send_config_data();
			wdt_enable(WDTO_2S);

        }
        break;
    default:
        break;
    }
}

static void sysExCmdBulkXfer(uint16_t length, uint8_t* buffer)
{
    if (length > 2) {
        uint8_t command = *buffer++;
        uint8_t tag = *buffer++;

        if (command == 0) {
            if (length > 5) {
                if (tag == 0x0) return;
                uint8_t part = *buffer++;
                if (part == 0) return;
                buffer++;
                uint8_t size = *buffer++;
                if (size > length - 5) return;

				if (part > 16) {
					return;
				}

				uint8_t bank = (part-1) >> 3;
				uint8_t offset = ((part-1) & 0x07) * 24;

			    wdt_disable();
                for (uint8_t i = 0; i+2 < size; i+=3) {
                    for (uint8_t j = 0; j < 3; ++j) {
                        buffer[i+j] *= 2;
                    }

                    if (tag == 2) adjust_active_bank_leds_for_power(buffer + i);
                    else adjust_inactive_bank_leds_for_power(buffer + i);

                    for (uint8_t j = 0; j < 3; j++) {
                        eeprom_write(
                            (tag == 2? EE_COLORS_ACTIVE : EE_COLORS_IDLE)+(bank*NUM_BUTTONS*3)+offset+i+j,
                            buffer[i+j]
                        );
                    }
                }
			    wdt_enable(WDTO_2S);
            }
        } else if (command == 1) {
            uint16_t source;
            if (tag == 1) {
                source = EE_COLORS_IDLE;
            } else if (tag == 2) {
                source = EE_COLORS_ACTIVE;
            } else {
                return;
            }

            uint16_t bytes_remaining = 2 * NUM_BUTTONS * 3;

            uint16_t total = bytes_remaining / 24;
            uint16_t index=0;
            for (uint16_t part=1; part<=total; ++part) {

                uint16_t size = bytes_remaining > 24 ? 24 : bytes_remaining;
                bytes_remaining -= 24;

                uint8_t payload[] = {0xf0, 0x00, MANUFACTURER_ID >> 8, MANUFACTURER_ID & 0x7f,
                                SYSEX_COMMAND_BULK_XFER,
                                0x0,
                                tag,
                                part,
                                total,
                                size,

                                0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,
                                0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,0xf7,
                                0xf7};

			    wdt_disable();
                for (uint8_t idx=10; idx < size+10; ++idx) {

                    uint16_t this_color = eeprom_read(source + index++);
                    this_color = this_color * DISPLAY_SCALING_COLOR_OUT_MAX_VALUE / DISPLAY_SCALING_COLOR_IN_MAX_VALUE;

                    payload[idx] = this_color;

                }
			    wdt_enable(WDTO_2S);

                midi_stream_sysex(11 + size, payload);
				MIDI_Device_Flush();
            }
        }
    }
}

void config_setup (void)
{

    sysex_install(SYSEX_COMMAND_PUSH_CONF, sysExCmdPushConfig);
    sysex_install(SYSEX_COMMAND_PULL_CONF, sysExCmdPullConfig);
    sysex_install(SYSEX_COMMAND_SYSTEM,    sysExCmdSystem);
    sysex_install(SYSEX_COMMAND_BULK_XFER, sysExCmdBulkXfer);
}
