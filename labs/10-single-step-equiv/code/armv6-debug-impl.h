#ifndef __ARMV6_DEBUG_IMPL_H__
#define __ARMV6_DEBUG_IMPL_H__
#include "armv6-debug.h"

// all your code should go here.  implementation of the debug interface.

// example of how to define get and set for status registers
coproc_mk(status, p14, 0, c0, c1, 0)
coproc_mk(dfsr, p15, 0, c5, c0, 0)
coproc_mk(ifar, p15, 0, c6, c0, 0)
coproc_mk(ifsr, p15, 0, c5, c0, 1)
coproc_mk(dscr, p14, 0, c0, c1, 0)
coproc_mk(wcr0, p14, 0, c0, c0, 7)
coproc_mk(wvr0, p14, 0, c0, c0, 6)
coproc_mk(bcr0, p14, 0, c0, c0, 5)
coproc_mk(bvr0, p14, 0, c0, c0, 4)
coproc_mk(wfar, p14, 0, c0, c6, 0)
coproc_mk(far, p15, 0, c6, c0, 0)
// you'll need to define these and a bunch of other routines.
static inline uint32_t cp15_dfsr_get(void);
static inline uint32_t cp15_ifar_get(void);
static inline uint32_t cp15_ifsr_get(void);
static inline uint32_t cp14_dscr_get(void);

static inline void cp14_wcr0_set(uint32_t r);

// return 1 if enabled, 0 otherwise.  
//    - we wind up reading the status register a bunch:
//      could return its value instead of 1 (since is 
//      non-zero).
static inline int cp14_is_enabled(void) {
    uint32_t dscr = cp14_dscr_get();
    return bit_get(dscr, 15);
}

// enable debug coprocessor 
static inline void cp14_enable(void) {
    // if it's already enabled, just return?
    if(cp14_is_enabled())
        return;

    // for the core to take a debug exception, monitor debug mode has to be both 
    // selected and enabled --- bit 14 clear and bit 15 set.
    uint32_t dscr = cp14_dscr_get();
    dscr = bit_clr(dscr, 14);
    dscr = bit_set(dscr, 15);
    cp14_dscr_set(dscr);

    assert(cp14_is_enabled());
}

// disable debug coprocessor
static inline void cp14_disable(void) {
    if(!cp14_is_enabled())
        return;

    uint32_t dscr = cp14_dscr_get();
    dscr = bit_clr(dscr, 15);
    // dscr = bit_set(dscr, 14);
    cp14_dscr_set(dscr);

    assert(!cp14_is_enabled());
}


static inline int cp14_bcr0_is_enabled(void) {
    uint32_t bcr = cp14_bcr0_get();
    return bits_get(bcr, 0, 0);
}
static inline void cp14_bcr0_enable(void) {
    if(cp14_bcr0_is_enabled())
        return;

    uint32_t bcr = cp14_bcr0_get();
    bcr = bits_set(bcr, 0, 0, 1);
    cp14_bcr0_set(bcr);

    assert(cp14_bcr0_is_enabled());
}
static inline void cp14_bcr0_disable(void) {
    if(!cp14_bcr0_is_enabled())
        return;

    uint32_t bcr = cp14_bcr0_get();
    bcr = bits_clr(bcr, 0, 0);
    cp14_bcr0_set(bcr);

    assert(!cp14_bcr0_is_enabled());
}

// was this a brkpt fault?
static inline int was_brkpt_fault(void) {
    // use IFSR and then DSCR
    // CHECK IF BIT 10 IS SET
    uint32_t ifsr = cp15_ifsr_get();
    if (!bit_get(ifsr, 10) == 0) return 0;
    uint32_t dscr = cp14_dscr_get();

    return (bits_eq(ifsr, 0, 3, 0b0010) && bits_eq(dscr, 2, 5, 0b0001));
}

// was watchpoint debug fault caused by a load?
static inline int datafault_from_ld(void) {
    return bit_isset(cp15_dfsr_get(), 11) == 0;
}
// ...  by a store?
static inline int datafault_from_st(void) {
    return !datafault_from_ld();
}


// 13-33: tabl 13-23
static inline int was_watchpt_fault(void) {
    // use DFSR then DSCR
    uint32_t dfsr = cp15_dfsr_get();
    uint32_t dscr = cp14_dscr_get();
    return (bits_eq(dfsr, 0, 3, 0b0010) && bits_eq(dscr, 2, 5, 0b0010));
}

static inline int cp14_wcr0_is_enabled(void) {
    uint32_t wcr = cp14_wcr0_get();
    return bits_get(wcr, 0, 0);
}
static inline void cp14_wcr0_enable(void) {
    if(cp14_wcr0_is_enabled())
        return;

    uint32_t wcr = cp14_wcr0_get();
    wcr = bits_set(wcr, 0, 0, 1);
    cp14_wcr0_set(wcr);

    assert(cp14_wcr0_is_enabled());
}
static inline void cp14_wcr0_disable(void) {
    if(!cp14_wcr0_is_enabled())
        return;

    uint32_t wcr = cp14_wcr0_get();
    wcr = bits_clr(wcr, 0, 0);
    cp14_wcr0_set(wcr);

    assert(!cp14_wcr0_is_enabled());
}

static inline void set_cp14_wvr0(uint32_t v) {
    cp14_wvr0_set(v);
}
static inline void set_cp14_bvr0(uint32_t v) {
    cp14_bvr0_set(v);
}

// Get watchpoint fault using WFAR
static inline uint32_t watchpt_fault_pc(void) {
    uint32_t wfar = cp14_wfar_get();
    return wfar - 0x8;
}

static inline uint32_t watchpt_fault_addr() {
    uint32_t far = cp15_far_get();
    return far;
}
    
#endif
