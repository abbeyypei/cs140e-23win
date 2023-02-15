#include "breakpoint.h"

#include "../../10-single-step-equiv/code/armv6-debug-impl.h"

// disable mismatch
void brkpt_mismatch_stop(void) {
    cp14_disable();
}

// enable mismatch
void brkpt_mismatch_start(void) {
    cp14_enable();
    cp14_bcr0_disable();
    cp14_wcr0_disable();
}

// set breakpoint on <addr>
void brkpt_mismatch_set(uint32_t addr) {
    uint32_t b = 0;
    set_cp14_bvr0(addr);

    b = bit_clr(b, 20);
    b = bits_set(b, 20, 21, 0b00);
    b = bits_set(b, 14, 15, 0b00);
    b = bits_set(b, 5, 8, 0b1111);
    b = bits_set(b, 1, 2, 0b11);
    b = bit_set(b, 0);

    cp14_bcr0_set(b);
}

// is it a breakpoint fault?
int brkpt_fault_p(void) {
    return was_brkpt_fault();
}