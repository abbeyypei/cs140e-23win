// see that you can grow the stack.
#include "vm-ident.h"
#include "libc/bit-support.h"
#include "vector-base.h"
#include "armv6-cp15.h"


fld_t * create_table() {
    mmu_init();
    assert(!mmu_is_enabled());

    // ugly hack: we start by allocating a single page table at a known address.
    fld_t *pt = mmu_pt_alloc(4096);
    assert(pt == (void*)0x100000);

    // 2. setup mappings

    // map the first MB: shouldn't need more memory than this.
    mmu_map_section(pt, 0x0, 0x0, dom_id);
    // map the heap/page table: for lab cksums must be at 0x100000.
    mmu_map_section(pt, 0x100000,  0x100000, dom_id);
    fld_t *pte = mmu_map_section(pt, 0x200000, 0x200000, dom_id);
    pte->nG = 1;

    // map the GPIO: make sure these are not cached and not writeback.
    // [how to check this in general?]
    mmu_map_section(pt, 0x20000000, 0x20000000, dom_id);
    
    mmu_map_section(pt, 0x20100000, 0x20100000, dom_id);
    mmu_map_section(pt, 0x20200000, 0x20200000, dom_id);


    // setup user stack (note: grows down!)
    uint32_t stack_begin = STACK_ADDR - OneMB;
    mmu_map_section(pt, stack_begin, stack_begin, dom_id);

    // setup interrupt stack (note: grows down!)
    // if we don't do this, then the first exception = bad infinite loop
    uint32_t int_stack_begin = INT_STACK_ADDR - OneMB;
    mmu_map_section(pt, int_stack_begin, int_stack_begin, dom_id);

    // 3. install fault handler to catch if we make mistake.
    extern uint32_t default_vec_ints[];
    vector_base_set(default_vec_ints);

    // 4. start the context switch:

    // set up permissions for the one domain we use rn.
    domain_access_ctrl_set(0b01 << dom_id*2);
    vm_ident_mmu_on(pt);

    return pt;
}

fld_t *copy_table(fld_t *ori_pt) {

    // ugly hack: we start by allocating a single page table at a known address.
    fld_t *pt = mmu_pt_alloc(4096);

    memcpy(pt, ori_pt, 4096 * (sizeof(fld_t)));
    fld_t *pte = mmu_map_section(pt, 0x200000, 0x400000, dom_id);
    pte->nG = 1;

    return pt;
}


void vm_test(void) {
    // setup a simple process structure that tracks where the stack is.
    mmu_be_quiet();
    kmalloc_init_set_start((void*)OneMB, OneMB);
    fld_t *pt1 = create_table();
    fld_t *pt2 = copy_table(pt1);

    volatile uint32_t *p = (void*)(0x200000);
    volatile uint32_t *pa1 = (void*)(0x200000);
    volatile uint32_t *pa2 = (void*)(0x400000);
    

    set_procid_ttbr0(0x140e, 1, pt1);
    put32(p, 11);
    mmu_disable();
    printk("setting first PT 0x200000 to 11\n");
    printk("PA1 is %d\n", get32(pa1));
    printk("PA2 is %d\n", get32(pa2));
    mmu_enable();

    set_procid_ttbr0(0x140e, 2, pt2);
    put32(p, 22);
    // turn off 
    assert(get32(p) == 22);
    mmu_disable();
    printk("setting second PT 0x200000 to 22\n");
    printk("PA1 is %d\n", get32(pa1));
    printk("PA2 is %d\n", get32(pa2));
    mmu_enable();

    // set_procid_ttbr0(0x140e, 1, pt1);
    // printk("p is %d\n", get32(p));
}

void notmain() {
    vm_test();
}