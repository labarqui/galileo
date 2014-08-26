#
# Makefile for Galileo projects
# Written by JLH
# Based on the awesome work done by Amit Patwardhan 
# Aug-2014
#

BINARY = hello_world
MAIN = $(BINARY).asm
ARDUINO_VER = 153
PORT = /dev/cu.usbmodem2411
PLATFORM = osx

# Paths

ABS_PATH = /Applications/Galileo.app/Contents/Resources/Java
REL_PATH=/hardware/tools/x86
BIN_PATH = $(ABS_PATH)$(REL_PATH)/i386-pokysdk-darwin/usr/bin/i586-poky-linux-uclibc
SYS_ROOT = $(ABS_PATH)$(REL_PATH)/i586-poky-linux-uclibc
ARDUINO_PATH = $(ABS_PATH)/hardware/arduino/x86/cores/arduino
VARIANT = $(ABS_PATH)/hardware/arduino/x86/variants/galileo_fab_d

# tools
PREFIX = i586-poky-linux-uclibc-

CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
LD = $(PREFIX)gcc
AR = $(PREFIX)ar
OBJCOPY = $(PREFIX)objcopy
SIZE = $(PREFIX)size
STRIP = $(PREFIX)strip

# flags

COM_FLAGS = -m32 -march=i586 --sysroot=$(SYS_ROOT) -c -g -Os -w -ffunction-sections -fdata-sections -MMD -DARDUINO=$(ARDUINO_VER) 
CFLAGS = $(COM_FLAGS)
CXXFLAGS = $(COM_FLAGS) -fno-exceptions

LDFLAGS = -m32 -march=i586 --sysroot=$(SYS_ROOT) -Os -Wl,--gc-sections 

# modules
# does not have all the files change as per project
C_MODULES = $(ARDUINO_PATH)/wiring_digital.c
C_MODULES += $(ARDUINO_PATH)/wiring_analog.c
C_MODULES += $(ARDUINO_PATH)/fast_gpio_sc.c  
C_MODULES += $(ARDUINO_PATH)/interrupt.c  
C_MODULES += $(ARDUINO_PATH)/sysfs.c  
C_MODULES += $(ARDUINO_PATH)/i2c.c     
C_MODULES += $(ARDUINO_PATH)/mux.c        
C_MODULES += $(ARDUINO_PATH)/trace.c  

#CXX_MODULES = $(ABS_PATH)/hardware/arduino/x86/variants/galileo_fab_d/variant.cpp
#CXX_MODULES += $(ARDUINO_PATH)/RingBuffer.cpp
#CXX_MODULES += $(ARDUINO_PATH)/TTYUART.cpp
#CXX_MODULES += $(ARDUINO_PATH)/UtilTime.cpp
#CXX_MODULES += $(ARDUINO_PATH)/Print.cpp
#CXX_MODULES += $(ARDUINO_PATH)/Stream.cpp   
#CXX_MODULES += $(ARDUINO_PATH)/WString.cpp
#CXX_MODULES += $(ARDUINO_PATH)/WMath.cpp


INCS = -I$(ARDUINO_PATH) \
	-I$(ABS_PATH)/hardware/arduino/x86/variants/galileo_fab_d

COBJS = $(C_MODULES:.c=.o)
CXXOBJS = $(CXX_MODULES:.cpp=.o)
OBJS = $(CXXOBJS) $(COBJS)

# rules

all: elf strip size
	@echo "Done"

elf: $(BINARY).o $(OBJS)
	$(BIN_PATH)/$(CC) -o $(BINARY).elf $(BINARY).o $(OBJS) $(LDFLAGS) -lm -lpthread

%.o: %.asm
	$(BIN_PATH)/$(CC) $< -o $@ $(INCS) $(CFLAGS)

.PHONY: strip size clean flash

strip: $(BINARY).elf
	$(BIN_PATH)/$(STRIP) $(BINARY).elf

size: $(BINARY).elf
	$(BIN_PATH)/$(SIZE) $(BINARY).elf
#@echo "of 262,144 bytes maximum"
clean:
	rm -f $(BINARY).o
	rm -f $(OBJS)
	rm -f *.d
	rm -f *.elf
	rm -f *.hex

# on GNU/Linux
# sudo make flash
flash: $(BINARY).elf
	@echo "flash"
	bash $(ARDUINO_PATH)/../../tools/izmir/clupload_$(PLATFORM).sh $(ABS_PATH)$(REL_PATH)/bin $(BINARY).elf $(PORT)
