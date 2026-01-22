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
PYMCUPROG_WRITE = pymcuprog write -t uart -u /dev/ttyUSB0 -d $(DEVICE) -c 125k

CC = avr-gcc
OBJCOPY = avr-objcopy
AVR-SIZE = avr-size
OBJDUMP = avr-objdump

#------------------------------------------------------------------------------
#----------------------------------------- no user serviceable parts inside ---
.PHONY: all all-chem all-one all-LTO all-NAION clean flash eeprom flash_isp dirs

OUTDIR := ./build

# Create FW version number from the latest git tag.
FW_VERSION ?= dev
CHEMISTRY ?= LTO

ifeq ($(filter $(CHEMISTRY),LTO NAION),)
$(error CHEMISTRY must be LTO or NAION)
endif

OBJDIR := $(OUTDIR)/obj-$(CHEMISTRY)-$(FW_VERSION)
TARGET = $(OUTDIR)/main-$(CHEMISTRY)-$(FW_VERSION)
EEPROM_BIN = $(OUTDIR)/eeprom-$(CHEMISTRY)-$(FW_VERSION).bin
EEPROM_HEX = $(OUTDIR)/eeprom-$(CHEMISTRY)-$(FW_VERSION).hex

SRC_DIR  := ./ ../config $(sort $(dir $(wildcard ../libs/*/))) $(sort $(dir $(wildcard ../libs/*/*/)))
SOURCES  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)*.c))
OBJECTS  := $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.c=.o)))
INCLUDES  := -I. $(addprefix -I,$(SRC_DIR))

LDFLAGS += -Wl,--relax
LDFLAGS += -Wl,--gc-sections
CFLAGS += -std=gnu99
CFLAGS += -Os
CFLAGS += -mmcu=$(DEVICE)
CFLAGS += -DF_CPU=$(CLOCK) $(if $(DEBUG),-DDEBUG_ENABLE)
CFLAGS += -DFW_VERSION=\"$(FW_VERSION)\"
CFLAGS += -DCELL_$(CHEMISTRY)
CFLAGS += -Wall
CFLAGS += -Winline
CFLAGS += -Wstrict-prototypes
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += $(INCLUDES)
CFLAGS += -funsigned-char
CFLAGS += -fdiagnostics-color=always

# default target
all: all-chem

all-chem: all-LTO all-NAION

all-LTO:
	$(MAKE) all-one CHEMISTRY=LTO FW_VERSION=$(FW_VERSION)

all-NAION:
	$(MAKE) all-one CHEMISTRY=NAION FW_VERSION=$(FW_VERSION)

all-one: dirs hex bin

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
	rm -rf $(OUTDIR)

dirs:
	mkdir -p $(OUTDIR) $(OBJDIR)

release: all
	@cp $(TARGET).bin $(CNF)-$(FW_VERSION).bin
	@cp $(TARGET).hex $(CNF)-$(FW_VERSION).hex
	@cp $(TARGET).elf $(CNF)-$(FW_VERSION).elf

flash:
ifeq ($(FW_FILE),)
	$(MAKE) all-one
	$(PYMCUPROG_WRITE) -f $(TARGET).hex --erase
else
	@echo "Flashing provided firmware: $(FW_FILE)"
	$(PYMCUPROG_WRITE) -f $(FW_FILE) --erase
endif


fuses: fuses.hex
	$(PYMCUPROG_WRITE) -f $<

eeprom: $(EEPROM_HEX)
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

$(EEPROM_HEX): FORCE
	@if [ -z "$(SN)" ]; then echo "Missing SN variable!"; exit 1; fi
	python tools/eeprom.py --chemistry $(CHEMISTRY) --sn $(SN) $(if $(TO),--temp-offset $(TO)) --file $(EEPROM_BIN)
	@objcopy --input-target=binary --output-target=ihex --change-addresses=0x810000 $(EEPROM_BIN) $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: ../libs/*/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: ../libs/*/*/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

size: dirs $(TARGET).elf
	@echo "    SIZES"
	@$(OBJDUMP) -h $(TARGET).elf | perl -MPOSIX -ne '/.(text)\s+([0-9a-f]+)/ && do { $$a += eval "0x$$2" }; END { printf "    FLASH : %5d bytes\n", $$a }'
	@$(OBJDUMP) -h $(TARGET).elf | perl -MPOSIX -ne '/.(data|bss)\s+([0-9a-f]+)/ && do { $$a += eval "0x$$2" }; END { printf "    RAM   : %5d bytes\n", $$a }'
	@$(OBJDUMP) -h $(TARGET).elf | perl -MPOSIX -ne '/.(eeprom)\s+([0-9a-f]+)/ && do { $$a += eval "0x$$2" }; END { printf "    EEPROM: %5d bytes\n", $$a }'

FORCE: ;
