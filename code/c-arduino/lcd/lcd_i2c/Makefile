#
# Makefile for Galileo projects
# Written by JLH
# Based on the awesome work done by Amit Patwardhan 
# Aug-2014
#

BINARY = lcd_i2c
MAIN = $(BINARY).cpp
ARDUINO_VER = 153
PORT = /dev/ttyACM0

# Paths

ABS_PATH = $(HOME)/arduino-1.5.3
LIB_PATH = $(ABS_PATH)/libraries
REL_PATH=/hardware/tools/sysroots
BIN_PATH = $(ABS_PATH)$(REL_PATH)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux-uclibc
SYS_ROOT = $(ABS_PATH)$(REL_PATH)/i586-poky-linux-uclibc
ARDUINO_PATH = $(ABS_PATH)/hardware/arduino/x86/cores/arduino
VARIANT = $(ABS_PATH)/hardware/arduino/x86/variants/galileo_fab_d
ARDUINO_LIB_PATH = $(ABS_PATH)/hardware/arduino/x86/libraries

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

CXX_MODULES = $(ABS_PATH)/hardware/arduino/x86/variants/galileo_fab_d/variant.cpp
CXX_MODULES += $(ARDUINO_PATH)/RingBuffer.cpp
CXX_MODULES += $(ARDUINO_PATH)/TTYUART.cpp
CXX_MODULES += $(ARDUINO_PATH)/UtilTime.cpp
CXX_MODULES += $(ARDUINO_PATH)/Print.cpp
CXX_MODULES += $(ARDUINO_PATH)/Stream.cpp   
CXX_MODULES += $(ARDUINO_PATH)/WString.cpp
CXX_MODULES += $(ARDUINO_PATH)/WMath.cpp
CXX_MODULES += $(LIB_PATH)/LiquidCrystal/src/LiquidCrystal.cpp
CXX_MODULES += $(wildcard $(LIB_PATH)/LiquidCrystalI2C/*.cpp)
CXX_MODULES += $(wildcard $(ARDUINO_LIB_PATH)/Wire/*.cpp)


INCS  = -I$(ARDUINO_PATH)
INCS +=	-I$(ABS_PATH)/hardware/arduino/x86/variants/galileo_fab_d 
INCS +=	-I$(LIB_PATH)/LiquidCrystal/src/
INCS +=	-I$(LIB_PATH)/LiquidCrystalI2C/
INCS +=	-I$(ARDUINO_LIB_PATH)/Wire/

COBJS = $(C_MODULES:.c=.o)
CXXOBJS = $(CXX_MODULES:.cpp=.o)
OBJS = $(CXXOBJS) $(COBJS)

# rules

all: elf strip size
	@echo "Done"

elf: $(BINARY).o $(OBJS)
	$(BIN_PATH)/$(CXX) -o $(BINARY).elf $(BINARY).o $(OBJS) $(LDFLAGS) -lm -lpthread

%.o: %.c
	$(BIN_PATH)/$(CC) $< -o $@ $(INCS) $(CFLAGS)

%.o: %.cpp
	$(BIN_PATH)/$(CXX) $< -o $@ $(INCS) $(CXXFLAGS)

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
	bash $(ARDUINO_PATH)/../../tools/izmir/clupload_linux.sh $(ABS_PATH)/hardware/tools $(BINARY).elf $(PORT)
