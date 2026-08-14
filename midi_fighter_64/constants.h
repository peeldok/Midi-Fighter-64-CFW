#ifndef _CONSTANTS_H_INCLUDED
#define _CONSTANTS_H_INCLUDED

#define NUM_BUTTONS 64
#define BUTTON_ID_FLAGS 0x3F

#define USB_RX_FAIL_LIMIT 105
#define USB_RX_PACKET_LIMIT 210
#define NOTE_OFF_FEEDBACK_DELAY_LIMIT 2
#define DEBOUNCE_BUFFER_SIZE 10
#define LED_ASYNC_GROUP0	_BV(PB6)
#define LED_ASYNC_GROUP1	_BV(PC6)
#define LED_ASYNC_GROUP2	_BV(PB5)
#define LED_ASYNC_GROUP3	_BV(PB4)

#define LED_LATCH  _BV(PB0)
#define LED_CLOCK  _BV(PB1)
#define LED_MOSI   _BV(PB2)
#define LED_MODE   _BV(PB7)

#define LED_BLANK  _BV(PC6)
#define LED_PWM    _BV(PC7)

#define KEY_BIT    _BV(PC7)
#define KEY_CLOCK  _BV(PD7)
#define KEY_LATCH  _BV(PD6)

#define DEVICE_FAMILY_LSB   0x06
#define DEVICE_FAMILY_MSB	0x00
#define DEVICE_FAMILY   	DEVICE_FAMILY_LSB, DEVICE_FAMILY_MSB

#define DEVICE_MODEL_LSB	0x01
#define DEVICE_MODEL_MSB    0x00
#define DEVICE_MODEL    	DEVICE_MODEL_LSB, DEVICE_MODEL_MSB

#define DEVICE_VERSION_YEAR 	0x3024
#define DEVICE_VERSION_MONTH	0x03
#define DEVICE_VERSION_DAY		0x20

#define MIDI_MFR_ID_0            0x00
#define MIDI_MFR_ID_1            0x01
#define MIDI_MFR_ID_2            0x79
#define MANUFACTURER_ID        0x0179

#define MIDI_BASENOTE              36
#define MIDI_MAX_SYSEX            270

#define EEPROM_LAYOUT                 1

#define EE_EEPROM_VERSION        0x0000
#define EE_MIDI_CHANNEL          0x0002
#define EE_MIDI_VELOCITY         0x0003
#define EE_FOUR_BANKS_MODE       0x0005
#define EE_MIDI_OUTPUT_MODE      0x0009
#define EE_COMBOS_ENABLE		 0x000A
#define EE_TILT_MODE             0x000C
#define EE_TILT_MASK			 0x000D
#define EE_ANIMATIONS            0x000E

#define EE_TILT_SENSITIVITY		 0x000F
#define EE_PITCH_SENSITIVITY     0x0010
#define EE_TILT_RANGE            0x0011
#define EE_PITCH_RANGE           0x0012
#define EE_TILT_DEADZONE         0x0013
#define EE_PITCH_DEADZONE        0x0014
#define EE_TILT_AXIS             0x0015
#define EE_PICK_SENSITIVITY      0x0016
#define EE_SLEEP_TIME            0x0017
#define EE_SIDE_BANK             0x0018
#define EE_ROTATION              0x0019
#define EE_ROTATION_MAGIC        0x001A

#define EE_COLORS_IDLE			 0x006F
#define EE_COLORS_ACTIVE		 0x01EF

#define EE_CH4_PALETTE            EE_COLORS_IDLE
#define EE_CH4_COMPONENT_BYTES    96
#define EE_CH4_MAGIC0             0x0370
#define EE_CH4_MAGIC1             0x0371
#define EE_CH4_MAGIC0_VALUE       0x36
#define EE_CH4_MAGIC1_VALUE       0x42
#define EE_LED_BRIGHTNESS         0x0372
#define EE_LED_BRIGHTNESS_MAGIC   0x0373
#define EE_LED_BRIGHTNESS_MAGIC_VALUE 0xB8
#define MIDI_CH4_CUSTOM_CHANNEL   3

#define MIDI_OUTPUT_MODE_NOTES_ONLY  0x00
#define MIDI_OUTPUT_MODE_CCS_ONLY 0x02

#endif
