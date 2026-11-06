
EXEPREFIX = 
NAMEPREFIX = arm-none-eabi-

CC      = $(EXEPREFIX)$(NAMEPREFIX)gcc
LD      = $(EXEPREFIX)$(NAMEPREFIX)gcc
OBJCOPY = $(EXEPREFIX)$(NAMEPREFIX)objcopy
SIZE    = $(EXEPREFIX)$(NAMEPREFIX)size

DEFINES = -DSTM32F030x6

NOPIE = -fno-pie -no-pie

CPU = -mcpu=cortex-m0 -mtune=cortex-m0 -mthumb

OPT = -Og
#OPT = -Os -fno-builtin

FLTO = #-flto

LINKRAM = -Tram.ld -Wl,-Map=ram.map
LINKROM = -Trom.ld -Wl,-Map=rom.map


CFLAGS =  $(FLTO) $(NOPIE) $(CPU) $(OPT) -std=gnu99 -g3 -ggdb $(DEFINES)
LDFLAGS = $(FLTO) $(NOPIE) $(CPU) $(OPT) -nostartfiles -nodefaultlibs -lgcc

GCC ?= $(CC)

.PHONY: all
all: rom.bin
#all: rom.bin ram.bin

### Source files
targets = startup main

# include dependencies for each target
include $(targets:%=%.d)


### Specific rules

ram.elf: $(targets:%=%.o)
	$(LD) -o$@ $^ $(LDFLAGS) $(LINKRAM)
	$(SIZE) $@


rom.elf: $(targets:%=%.o)
	$(LD) -o$@ $^ $(LDFLAGS) $(LINKROM)
	$(SIZE) $@

%.d: %.c Makefile
	$(GCC) -MM $(CFLAGS) $< | \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

#%.d: %.s Makefile
#	$(GCC) -MM $(CPPFLAGS) $< | \
#		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@


%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

#%.o: %.s Makefile
#	$(CC) $(CFLAGS) -c -o $@ $<


%.bin: %.elf Makefile
	$(OBJCOPY) -O binary $< $@

### PHONY target only below


clean:
	rm -f *.elf *.o *.bin *.d *.map


#ram: all
#	printf "reset halt\nload_image ram.elf\nreset\nexit\n" | nc localhost 4444

flash: all
	printf "reset halt\nflash write_image erase rom.elf\nreset\n" | nc -N localhost 4444

