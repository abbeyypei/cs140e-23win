// put your code here.
//
#include "rpi.h"
#include "libc/bit-support.h"

// has useful enums and helpers.
#include "vector-base.h"
#include "pinned-vm.h"
#include "pinned-vm-asm.h"
#include "mmu.h"
#include "procmap.h"

// generate the _get and _set methods.
// (see asm-helpers.h for the cp_asm macro 
// definition)
// arm1176.pdf: 3-149

// do a manual translation in tlb:
//   1. store result in <result>
//   2. return 1 if entry exists, 0 otherwise.
int tlb_contains_va(uint32_t *result, uint32_t va) {
    // 3-79
    assert(bits_get(va, 0, 2) == 0);
    uint32_t offset = bits_get(va, 0, 19);
    xlate_kern_rd_set(bits_clr(va, 0, 19));
    uint32_t pa = xlate_pa_get();
    *result = bits_set(pa, 0, 19, offset);
    return bit_is_off(pa, 0);
}

// map <va>-><pa> at TLB index <idx> with attributes <e>
void pin_mmu_sec(unsigned idx,  
                uint32_t va, 
                uint32_t pa,
                pin_t e) {

    demand(idx < 8, lockdown index too large);
    // lower 20 bits should be 0.
    demand(bits_get(va, 0, 19) == 0, only handling 1MB sections);
    demand(bits_get(pa, 0, 19) == 0, only handling 1MB sections);

    // if(va != pa)
    //     panic("for today's lab, va (%x) should equal pa (%x)\n",
    //             va,pa);

    debug("about to map %x->%x\n", va,pa);


    // these will hold the values you assign for the tlb entries.
    uint32_t x, va_ent, pa_ent, attr;
    
    va_ent = e.asid;
    va_ent = bits_set(va_ent, 12, 31, va >> 12);
    va_ent = bits_set(va_ent, 9, 9, e.G);

    pa_ent = 1;
    pa_ent = bits_set(pa_ent, 1, 3, e.AP_perm);
    pa_ent = bits_set(pa_ent, 12, 31, pa >> 12);
    pa_ent = bits_set(pa_ent, 6, 7, e.pagesize);

    attr = e.mem_attr << 1;
    attr = bits_set(attr, 7, 10, e.dom);

    lockdown_index_set(idx);
    lockdown_va_set(va_ent);
    lockdown_attr_set(attr);
    lockdown_pa_set(pa_ent);


    // put your code here.

    if((x = lockdown_va_get()) != va_ent)
        panic("lockdown va: expected %x, have %x\n", va_ent,x);
    if((x = lockdown_pa_get()) != pa_ent)
        panic("lockdown pa: expected %x, have %x\n", pa_ent,x);
    if((x = lockdown_attr_get()) != attr)
        panic("lockdown attr: expected %x, have %x\n", attr,x);

}

void domain_fault(unsigned pc) {
    uint32_t dfsr = dfsr_get();
    debug("Data Abort Fault -- pc: %x reason: %b\n", pc, bits_get(dfsr, 0, 3));
    
    domain_access_ctrl_set(~0); 
}

void prefetch_fault(unsigned pc) {
    uint32_t dfsr = dfsr_get();
    debug("Prefetch Abort Fault -- pc: %x reason: %b\n", pc, bits_get(dfsr, 0, 3));
    domain_access_ctrl_set(~0); 
    return;
}


// check that <va> is pinned.  
void pin_check_exists(uint32_t va) {
    if(!mmu_is_enabled())
        panic("XXX: i think we can only check existence w/ mmu enabled\n");

    uint32_t r;
    if(tlb_contains_va(&r, va)) {
        pin_debug("success: TLB contains %x, returned %x\n", va, r);
        assert(va == r);
    } else
        panic("TLB should have %x: returned %x [reason=%b]\n", 
            va, r, bits_get(r,1,6));
}

// TLB pin all entries in procmap <p>
// very simpleminded atm.
void pin_procmap(procmap_t *p) {
    for(unsigned i = 0; i < p->n; i++) {
        pr_ent_t *e = &p->map[i];
        assert(e->nbytes == MB);

        switch(e->type) {
        case MEM_DEVICE:
                pin_mmu_sec(i, e->addr, e->addr, pin_mk_device(e->dom));
                break;
        case MEM_RW:
        {
                // currently everything is uncached.
                pin_t g = pin_mk_global(e->dom, perm_rw_priv, MEM_uncached);
                pin_mmu_sec(i, e->addr, e->addr, g);
                break;
        }
        case MEM_RO: panic("not handling\n");
        default: panic("unknown type: %d\n", e->type);
        }
    }
}


// turn the pinned MMU system on.
//    1. initialize the MMU (maybe not actually needed): clear TLB, caches
//       etc.  if you're obsessed with low line count this might not actually
//       be needed, but we don't risk it.
//    2. allocate a 2^14 aligned, 0-filled 4k page table so that any nonTLB
//       access gets a fault.
//    3. set the domain privileges (to DOM_client)
//    4. set the exception handler up using <vector_base_set>
//    5. turn the MMU on --- this can be much simpler than the normal
//       mmu procedure since it's never been on yet and we do not turn 
//       it off.
//    6. profit!
void pin_mmu_on(procmap_t *p) {

    assert(!mmu_is_enabled());

    // we have to clear the MMU before setting any entries.
    mmu_init();
    pin_procmap(p);

    void *null_pt = kmalloc_aligned(4096*4, 1<<14);

    enum { dom_kern = 1,            // domain for kernel=1
           dom_user = 2 };          // domain for user = 2

    uint32_t d = (DOM_client << dom_kern*2)
                |(DOM_client << dom_user*2);
    domain_access_ctrl_set(d);

    extern uint32_t interrupt_table[];
    vector_base_set(interrupt_table);

    mmu_on_first_time(1, null_pt);
    assert(mmu_is_enabled());
    pin_debug("enabled!\n");

    // can only check this after MMU is on.
    pin_debug("going to check entries are pinned\n");
    for(unsigned i = 0; i < p->n; i++)
        pin_check_exists(p->map[i].addr);
}

void lockdown_print_entry(unsigned idx) {
    trace("   idx=%d\n", idx);
    lockdown_index_set(idx);
    uint32_t va_ent = lockdown_va_get();
    uint32_t pa_ent = lockdown_pa_get();
    unsigned v = bit_get(pa_ent, 0);

    if(!v) {
        trace("     [invalid entry %d]\n", idx);
        return;
    }

    // 3-149
    uint32_t va = va_ent >> 12;
    uint32_t G = bit_get(va_ent, 9);
    uint32_t asid = bits_get(va_ent, 0, 7);
    trace("     va_ent=%x: va=%x|G=%d|ASID=%d\n",
        va_ent, va, G, asid);

    // 3-150
    uint32_t nsa = bit_get(pa_ent, 9);
    uint32_t nstid = bit_get(pa_ent, 8);
    uint32_t size = bits_get(pa_ent, 6, 7);
    uint32_t pa = pa_ent >> 12;
    uint32_t apx = bits_get(pa_ent, 1, 3);
    uint32_t ap = bits_get(pa_ent, 1, 2);
    trace("     pa_ent=%x: pa=%x|nsa=%d|nstid=%d|size=%b|apx=%b|v=%d\n",
                pa_ent, pa, nsa,nstid,size, apx,v);

    // 3-151
    uint32_t attr = lockdown_attr_get();
    uint32_t dom = bits_get(attr, 7, 10);
    uint32_t xn = bit_get(attr, 6);
    uint32_t tex = bits_get(attr, 3, 5);
    uint32_t C = bit_get(attr, 2);
    uint32_t B = bit_get(attr, 1);
    trace("     attr=%x: dom=%d|xn=%d|tex=%b|C=%d|B=%d\n",
            attr, dom,xn,tex,C,B);
}

void lockdown_print_entries(const char *msg) {
    trace("-----  <%s> ----- \n", msg);
    trace("  pinned TLB lockdown entries:\n");
    for(int i = 0; i < 8; i++)
        lockdown_print_entry(i);
    trace("----- ---------------------------------- \n");
}