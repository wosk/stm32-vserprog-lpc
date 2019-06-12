# Makefile for stm32-vserprog-lpc
# * Set hardware config: make BOARD=some_board_in_boards_folder
# * Override toolchain: make CROSS=/path/to/arm-none-eabi-
# * Override UART for downloading via stm32flash: make SERIAL=/dev/ttyS1 flash

###############################################################################

PROGRAM  = stm32-vserprog-lpc
CROSS   ?= arm-none-eabi-
SERIAL  ?= /dev/ttyUSB0
PSERIAL ?= /dev/ttyACM0
OBJS     = main.o \
		   vserprog.o \
           usbcdc.o \
           lpc.o

DOCS     = README.html

###############################################################################

CC       = $(CROSS)gcc
LD       = $(CROSS)gcc
OBJCOPY  = $(CROSS)objcopy
OBJDUMP  = $(CROSS)objdump
SIZE     = $(CROSS)size

ELF      = $(PROGRAM).elf
BIN      = $(PROGRAM).bin
HEX      = $(PROGRAM).hex
MAP      = $(PROGRAM).map
DMP      = $(PROGRAM).out

ifneq ($(wildcard last_board.mk),)
  include last_board.mk
endif
ifdef BOARD
  include boards/$(BOARD).mk
endif

all:
ifndef BOARD
	@echo "Please specify a board by using 'make BOARD=board'."
	@echo "Available boards:"
	@echo "===================="
	@ls boards/*.h | cut -d '/' -f 2 | cut -d '.' -f 1
else
  ifndef LAST_BOARD
	@echo "First run, cleaning..."
	@make clean
  else
    ifneq ($(LAST_BOARD), $(BOARD))
	@echo "Board changed, cleaning..."
	@make clean
    endif
  endif
	@echo "LAST_BOARD = $(BOARD)" > last_board.mk
	@ln -sfT "boards/$(BOARD).h" board.h
	@make firmware
endif

#DEBUG = DEBUG_ENABLE_SEMIHOST
#DEBUG = DEBUG_SWO_ITM

ifdef DEBUG
CFLAGS += -DDEBUG -D$(DEBUG)
OBJS += printf.o
CFLAGS  += -Wall -g

ifeq ($(DEBUG),DEBUG_SWO_ITM)
OBJS += traceswo.o
endif

ifeq ($(DEBUG),DEBUG_ENABLE_SEMIHOST)
OBJS += semihosting.o
endif
else
CFLAGS += -O3 -std=gnu99
endif

CFLAGS  += -fno-common -ffunction-sections -fdata-sections -funit-at-a-time
CFLAGS  += -fgcse-sm -fgcse-las -fgcse-after-reload -funswitch-loops
CFLAGS  += $(ARCH_FLAGS) -Ilibopencm3/include/ $(EXTRA_CFLAGS)

LIBC     = $(shell $(CC) $(CFLAGS) --print-file-name=libc.a)
LIBGCC   = $(shell $(CC) $(CFLAGS) --print-libgcc-file-name)

# LDPATH is required for libopencm3 ld scripts to work.
LDPATH   = libopencm3/lib/
LDFLAGS += -L$(LDPATH) -T$(LDSCRIPT) -Wl,-Map,$(MAP) -Wl,--gc-sections -nostartfiles --specs=nosys.specs
LDLIBS  += $(LIBOPENCM3) $(LIBC) $(LIBGCC)

firmware: $(LIBOPENCM3) $(BIN) $(HEX) $(DMP) size
docs: $(DOCS)

$(ELF): $(LDSCRIPT) $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(LDLIBS)

$(DMP): $(ELF)
	$(OBJDUMP) -d $< > $@

%.hex: %.elf
	$(OBJCOPY) -S -O ihex   $< $@

%.bin: %.elf
	$(OBJCOPY) -S -O binary $< $@

%.o: %.c board.h
	$(CC) $(CFLAGS) -c $< -o $@

%.html: %.md
	markdown $< > $@

$(LIBOPENCM3):
	git submodule init
	git submodule update --remote
	CFLAGS="$(CFLAGS)" PREFIX="$(CROSS)" make -C libopencm3 $(OPENCM3_MK) V=1

.PHONY: clean distclean flash reboot size

clean:
	rm -f *.o $(DOCS) $(ELF) $(HEX) $(BIN) $(MAP) $(DMP) board.h last_board.mk

distclean: clean
	make -C libopencm3 clean
	rm -f *~ *.hex *.bin

flash: $(HEX)
	stm32flash -w $< -v $(SERIAL)

reboot:
	stm32flash -g 0x0 $(SERIAL)

size: $(PROGRAM).elf
	@echo ""
	@$(SIZE) $(PROGRAM).elf
	@echo ""
