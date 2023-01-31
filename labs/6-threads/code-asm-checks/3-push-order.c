// Q: does push change sp before writing to it or after?
#include "rpi.h"

// should take a few lines: 
//   - push argument (in r0) onto the stack.
//   - call <after_push> with:
//      - the first argument: sp after you do the push.
//      - second argument: sp before you do the push.

// implement this assembly routine in <asm-checks.S>
uint32_t * push_r4_r12_asm(uint32_t *scratch);


// called with:
//   - <sp_after_push>: value of the <sp> after the push 
//   - <sp_before_push>: value of the <sp> before the push 
void check_push_order(void) {
    uint32_t scratch_mem[64], 
            // pointer to the middle so doesn't matter if up
            // or down
            *p = &scratch_mem[32];

    uint32_t *ret = push_r4_r12_asm(p);
    assert(ret < p);

    // check that regs holds the right values.
    assert(*ret == 4);
    assert(*(ret+1) == 5);
    assert(*(ret+2) == 6);
    assert(*(ret+3) == 7);
    assert(*(ret+4) == 8);
    assert(*(ret+5) == 9);
    assert(*(ret+6) == 10);
    assert(*(ret+7) == 11);
    assert(*(ret+8) == 12);

    return;

}

void notmain() {
    trace("about to check push order\n");
    check_push_order();
    trace("SUCCESS\n");
}
