#!/usr/bin/env python
#
# Blink an LED on the Intel Galileo board
# JLH, Aug 2014

import io
import os
import time

def open_gpio(gpio, gpio_dir = "out"):
    base = '/sys/class/gpio'

    # Export symbol
    f = os.fdopen(os.open(base + '/export', os.O_WRONLY, 0600), 'w')
    f.write("%d" % gpio)
    f.close()

    # Set direction
    f = os.fdopen(os.open(base + ("/gpio%d" % gpio) + "/direction", os.O_WRONLY, 0600), 'w')
    f.write(gpio_dir)
    f.close()
    
    # Return the file for further operations
    f = os.fdopen(os.open(base + ("/gpio%d" % gpio) + "/value", os.O_WRONLY), 'w')
    return f

def write_gpio(f_gpio, val = 1):
    print val
    f_gpio.write("%d" % val)
    f_gpio.flush()

def close_gpio(gpio, f_gpio):
    f_gpio.close()
    base = '/sys/class/gpio'
    f = os.fdopen(os.open(base + "/unexport", os.O_WRONLY, 0600), 'w')
    f.write("%d" % gpio)
    f.close()

# 3 is the pin in the Quark GPIO handler
gpio = 3

# Open the file handle
f = open_gpio(gpio)

# Iterate
for i in range(1):
    write_gpio(f, 1)
    time.sleep(1)
    write_gpio(f, 0)
    time.sleep(1)

# Clean
close_gpio(gpio, f)
