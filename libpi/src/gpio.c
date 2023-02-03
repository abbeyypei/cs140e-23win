/*
 * Implement the following routines to set GPIO pins to input or output,
 * and to read (input) and write (output) them.
 *
 * DO NOT USE loads and stores directly: only use GET32 and PUT32
 * to read and write memory.  Use the minimal number of such calls.
 *
 * See rpi.h in this directory for the definitions.
 */
#include "rpi.h"

// see broadcomm documents for magic addresses.
enum {
    GPIO_BASE = 0x20200000,
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34)
};

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//

// set <pin> to be an output pin.
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use array calculations!
void gpio_set_output(unsigned pin) {
  gpio_set_function(pin, 1);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
  if(pin >= 32 && pin != 47)
        return;
  PUT32(gpio_set0 + (pin < 32 ? 0 : 4), 1 << pin % 32);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
  if(pin >= 32 && pin != 47)
        return;
  PUT32(gpio_clr0 + (pin < 32 ? 0 : 4), 1 << pin % 32);
  // implement this
  // use <gpio_clr0>
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
    if(v)
        gpio_set_on(pin);
    else
        gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> to input.
void gpio_set_input(unsigned pin) {
  gpio_set_function(pin, 0);
}

// return the value of <pin>
int gpio_read(unsigned pin) {
  if(pin >= 32 && pin != 47)
        return -1;

  unsigned mask = 1 << (pin % 32);

  unsigned v = (GET32(gpio_lev0 + (pin < 32 ? 0 : 4)) & mask) >> (pin % 32);
  // implement.
  return v;
}

void gpio_set_function(unsigned pin, gpio_func_t function) {
  if((pin >= 32 && pin != 47) || function > 7)
        return;

  unsigned idx = pin / 10;
  uint32_t gpio_fsel_pin = GPIO_BASE + idx * 4;
  unsigned mask = GET32(gpio_fsel_pin);
  mask &= ~(7 << (pin % 10) * 3);
  mask |= function << (pin % 10) * 3;
  PUT32(gpio_fsel_pin, mask);
}