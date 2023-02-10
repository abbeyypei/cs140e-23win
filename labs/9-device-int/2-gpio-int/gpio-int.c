// engler, cs140 put your gpio-int implementations in here.
#include "rpi.h"

enum {
    GPIO_BASE = 0x20200000,
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34),
    gpio_eds0  = (GPIO_BASE + 0x40),
    gpio_ren0  = (GPIO_BASE + 0x4C),
    gpio_fen0  = (GPIO_BASE + 0x58),
    gpio_hen0  = (GPIO_BASE + 0x64),
    gpio_len0  = (GPIO_BASE + 0x70),
};

enum {
    INT_BASE = 0x2000B000,
    int_irq_pending2 = INT_BASE + 0x208,
    int_irq_enable2 = INT_BASE + 0x214,
};

void or32(volatile void *addr, uint32_t val) {
    dev_barrier();
    put32(addr, get32(addr) | val);
    dev_barrier();
}

void OR32(uint32_t addr, uint32_t val) {
    or32((volatile void*) (long) addr, val);
}

// returns 1 if there is currently a GPIO_INT0 interrupt, 
// 0 otherwise.
//
// note: we can only get interrupts for <GPIO_INT0> since the
// (the other pins are inaccessible for external devices).
int gpio_has_interrupt(void) {
    int pin = GPIO_INT0;
    unsigned mask = 1 << (pin-32);
    unsigned v = (GET32(int_irq_pending2) & mask) >> (pin % 32);
    return DEV_VAL32(v);
}

void gpio_enable_interrupt(void) {
    PUT32(int_irq_enable2, 1 << (GPIO_INT0-32));
}

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin) {
    if(pin >= 32 && pin != 47)
        return;
    OR32(gpio_ren0 + (pin < 32 ? 0 : 4), 1 << pin % 32);
    gpio_enable_interrupt();
    dev_barrier();
}

// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
    if(pin >= 32)
        return;
    OR32(gpio_fen0 + (pin < 32 ? 0 : 4), 1 << pin % 32);
    gpio_enable_interrupt();
    dev_barrier();
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
    if(pin >= 32)
        return -1;

    unsigned mask = 1 << (pin % 32);

    dev_barrier();
    unsigned v = (GET32(gpio_eds0 + (pin < 32 ? 0 : 4)) & mask) >> (pin % 32);
    dev_barrier();
    return DEV_VAL32(v);
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
    if(pin >= 32)
        return;
    dev_barrier();
    PUT32(gpio_eds0 + (pin < 32 ? 0 : 4), 1 << pin % 32);
    dev_barrier();
}