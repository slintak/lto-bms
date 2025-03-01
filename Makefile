#------------------------------------------------------------------------------
#------------------------------------------------------------ configuration ---

#--------------------------------
# Hardware specific configuration
#--------------------------------

DEVICE = attiny824
CLOCK = 1250000

#------
# Tools
#------
PYMCUPROG_WRITE = pymcuprog write -t uart -u /dev/ttyUSB1 -d $(DEVICE) -c 125k

CC = avr-gcc
OBJCOPY = avr-objcopy
AVR-SIZE = avr-size
OBJDUMP = avr-objdump

#------------------------------------------------------------------------------
#----------------------------------------- no user serviceable parts inside ---
.PHONY: all clean flash eeprom flash_isp dirs

BUILDDIR := ./build

# Create FW version number from the latest git tag. If the current project is
# "dirty" (contains uncommited changes) or is few commits ahead of the tagged
# commit, mark version of the firmware as development "DEV".
FW_VERSION := $(shell git describe --always | sed -En -e 's/([0-9]+)\.([0-9]+)-.*/DEV/p' -e t -e 's/^([0-9]+)\.([0-9]+)$$/\1.\2/p' -e t -e 's/.*/0\.0/p')

TARGET = ./build/main-$(FW_VERSION)

SRC_DIR  := ./ ../config $(sort $(dir $(wildcard ../libs/*/))) $(sort $(dir $(wildcard ../libs/*/*/)))
SOURCES  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)*.c))
OBJECTS  := $(addprefix $(BUILDDIR)/,$(notdir $(SOURCES:.c=.o)))
INCLUDES  := -I. $(addprefix -I,$(SRC_DIR))

LDFLAGS += -Wl,--relax
LDFLAGS += -Wl,--gc-sections
CFLAGS += -std=gnu99
CFLAGS += -Os
CFLAGS += -mmcu=$(DEVICE)
CFLAGS += -DF_CPU=$(CLOCK) $(if $(DEBUG),-DDEBUG_ENABLE)
CFLAGS += -DFW_VERSION=\"$(FW_VERSION)\"
CFLAGS += -Wall
CFLAGS += -Winline
CFLAGS += -Wstrict-prototypes
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += $(INCLUDES)
CFLAGS += -funsigned-char
CFLAGS += -fdiagnostics-color=always

# default target
all: dirs hex bin

help:
	@echo
	@echo Help:
	@echo  make ....................... compile
	@echo  make flash ................. write firmware to the device
	@echo  make fuses ................. write fuses to the device
	@echo  make eeprom ................ write eeprom config to the device
	@echo  make lst ................... export assembly from elf
	@echo  make size .................. print used memory
	@echo  make clean ................. clean all

clean:
	rm -rf $(BUILDDIR)

dirs:
	mkdir -p $(BUILDDIR)

release: all
	@cp $(TARGET).bin $(CNF)-$(FW_VERSION).bin
	@cp $(TARGET).hex $(CNF)-$(FW_VERSION).hex
	@cp $(TARGET).elf $(CNF)-$(FW_VERSION).elf

flash:	all
	$(PYMCUPROG_WRITE) -f $(TARGET).hex --erase

fuses: fuses.hex
	$(PYMCUPROG_WRITE) -f $<

eeprom: $(BUILDDIR)/eeprom.hex
	$(PYMCUPROG_WRITE) -f $<

hex: $(TARGET).hex

bin: $(TARGET).bin

lst: $(TARGET).elf
	$(OBJDUMP) -d $(TARGET).elf > $(TARGET).lst

$(TARGET).elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -R eeprom -O ihex $(TARGET).elf $(TARGET).hex

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -R eeprom -O binary $(TARGET).elf $(TARGET).bin

$(BUILDDIR)/eeprom.hex: FORCE
	@if [ -z "$(SN)" ]; then echo "Missing SN variable!"; exit 1; fi
	python tools/eeprom.py --sn $(SN) $(if $(TO),--temp-offset $(TO)) --file $(BUILDDIR)/eeprom.bin
	@objcopy --input-target=binary --output-target=ihex --change-addresses=0x810000 $(BUILDDIR)/eeprom.bin $@

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/%.o: ../libs/*/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/%.o: ../libs/*/*/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

size: dirs $(TARGET).elf
	@echo "    SIZES"
	@$(OBJDUMP) -h $(TARGET).elf | perl -MPOSIX -ne '/.(text)\s+([0-9a-f]+)/ && do { $$a += eval "0x$$2" }; END { printf "    FLASH : %5d bytes\n", $$a }'
	@$(OBJDUMP) -h $(TARGET).elf | perl -MPOSIX -ne '/.(data|bss)\s+([0-9a-f]+)/ && do { $$a += eval "0x$$2" }; END { printf "    RAM   : %5d bytes\n", $$a }'
	@$(OBJDUMP) -h $(TARGET).elf | perl -MPOSIX -ne '/.(eeprom)\s+([0-9a-f]+)/ && do { $$a += eval "0x$$2" }; END { printf "    EEPROM: %5d bytes\n", $$a }'

FORCE: ;