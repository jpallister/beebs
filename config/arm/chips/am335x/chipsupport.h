/* Chip support for TI AM335x.

   Copyright (C) 2014 Embecosm Limited and the University of Bristol

   Contributor James Pallister <james.pallister@bristol.ac.uk>

   This file is part of BEEBS

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along with
   this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef CHIPSUPPORT_H
#define CHIPSUPPORT_H

// Define the registers we need to do a pin toggle ////////////////////////////

#define ADDR(x)     (*((unsigned long*)(x)))

// These registers define clocks, RCC (Reset and clock control)
#define CM_PER_BASE     0x44E00000
#define CM_PER_L4       ADDR(CM_PER_BASE + 0x0)
#define GPIO1_CLKCTL    ADDR(CM_PER_BASE + 0xAC)

#define GPIO1_BASE      0x4804C000
#define GPIO1_OE        ADDR(GPIO1_BASE + 0x134)
#define GPIO1_DATAOUT   ADDR(GPIO1_BASE + 0x13C)

// Provide a macros to do the pin toggling ////////////////////////////////////

// Initialise the pin + clocks
// This relies on UBoot being present on a SD card, and setting up the chip.
// We then jump in on JTAG and set the configuration we want
#define PIN_INIT(number)                    \
    do {                                    \
        /* Turn on the right clock domain */\
        CM_PER_L4 = 0;                      \
        /* Turn on the GPIO1 clock */       \
        GPIO1_CLKCTL = 2;                   \
        /* Turn off the pin first */        \
        GPIO1_DATAOUT &= ~(1 << number);    \
        /* enable the output */             \
        GPIO1_OE = ~(1 << number);          \
    } while(0)

// Set the pin to high
#define PIN_SET(number)                 \
    do {                                \
        /* Pull high GPIO pin */        \
        GPIO1_DATAOUT |= 1 << number;   \
    } while(0)

// Set the pin to low
#define PIN_CLEAR(number)               \
    do {                                \
        /* Pull low GPIO pin */         \
        GPIO1_DATAOUT &= ~(1 << number);\
    } while(0)

#endif /* CHIPSUPPORT_H */
