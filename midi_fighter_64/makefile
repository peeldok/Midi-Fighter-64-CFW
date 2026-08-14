MCU = atmega32u4
F_CPU = 16000000UL
TARGET = midifighter64

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
NM = avr-nm

SRC = $(TARGET).c \
      eeprom.c \
      led.c \
      key.c \
      midi.c \
      random.c \
      display.c \
      usb_descriptors.c \
      native_usb.c \
      jumptoboot.c \
      sysex.c \
      config.c \
      fastrgb.c \
      palettes.c \
      rotation.c \
      settings_ui.c \
      boot_animation.c \
      idle.c

OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d)

CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=gnu99 -O2 -MMD -MP
CFLAGS += -funsigned-char -funsigned-bitfields -ffunction-sections
CFLAGS += -fno-inline-small-functions -fpack-struct -fshort-enums
CFLAGS += -fno-strict-aliasing -Wall -Wstrict-prototypes -I.

LDFLAGS = -mmcu=$(MCU) -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -Wl,--relax -Wl,--gc-sections -lm

.PHONY: all build elf hex size clean dfu lss sym

-include $(DEP)

all: elf hex size
build: elf hex
elf: $(TARGET).elf
hex: $(TARGET).hex

$(TARGET).elf: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

lss: $(TARGET).elf
	$(OBJDUMP) -h -S -z $< > $(TARGET).lss

sym: $(TARGET).elf
	$(NM) -n $< > $(TARGET).sym

dfu: $(TARGET).hex
	dfu-programmer $(MCU) erase
	dfu-programmer $(MCU) flash $(TARGET).hex
	dfu-programmer $(MCU) reset

ifeq ($(OS),Windows_NT)
clean:
	cmd /C "del /Q $(OBJ) $(DEP) $(TARGET).elf $(TARGET).hex $(TARGET).map $(TARGET).lss $(TARGET).sym 2>NUL || exit /B 0"
else
clean:
	rm -f $(OBJ) $(DEP) $(TARGET).elf $(TARGET).hex $(TARGET).map $(TARGET).lss $(TARGET).sym
endif
