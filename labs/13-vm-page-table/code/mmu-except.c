#include "vm-ident.h"
#include "libc/bit-support.h"
#include "asm-helpers.h"

cp_asm(dfsr, p15, 0, c5, c0, 0)
cp_asm(far, p15, 0, c6, c0, 0)
cp_asm(ifsr, p15, 0, c5, c0, 1)
cp_asm(ifar, p15, 0, c6, c0, 2)

volatile struct proc_state proc;

// this is called by reboot: we turn off mmu so that things work.
void reboot_callout(void) {
    if(mmu_is_enabled())
        mmu_disable();
}

void prefetch_abort_vector(unsigned lr) {
    uint32_t ifsr = ifsr_get();
    uint32_t addr = ifar_get();
    uint32_t fs = bits_get(ifsr, 0, 3);
    proc.fault_count++;
    proc.fault_addr = addr;
    printk("prefetch abort %b: %x\n", fs, addr);
    if (!bit_get(ifsr, 10)) {
        if (fs == 0b1101) {
            printk("ERROR: attempting to run addr %x with no permissions (reason=%b)\n", addr, fs);
            clean_reboot();
        } else if (fs == 0b0101) {
            printk("ERROR: attempting to run unmapped addr %x (reason=%b)\n", addr, fs);
            clean_reboot();
        }
    }
}

void data_abort_vector(unsigned lr) {
    uint32_t dfsr = dfsr_get();
    uint32_t addr = far_get();
    uint32_t fs = bits_get(dfsr, 0, 3);
    uint32_t rw = bit_get(dfsr, 11);
    proc.fault_count++;
    proc.fault_addr = addr;
    if (!bit_get(dfsr, 10)) {
        if (fs == 0b0101) {
            if (addr < proc.sp_lowest_addr + OneMB && addr > proc.sp_lowest_addr - OneMB) {
                printk("section xlate fault: %x\n", addr);
                printk("fault addr=%x\n", addr);
                printk("%x is within 1mb of lowest stack pointer %x\n", addr, proc.sp_lowest_addr);
                mmu_map_section(proc.pt, bits_clr(addr, 0, 19), bits_clr(addr, 0, 19), proc.dom_id);
                staff_mmu_sync_pte_mods();
            } else {
                if (rw) {
                    printk("ERROR: attempting to store unmapped addr %x: dying (reason=%b)\n", addr, fs);
                } else {
                    printk("ERROR: attempting to load unmapped addr %x: dying (reason=%b)\n", addr, fs);
                }
                clean_reboot();
            }
            
        } else if (fs == 0b1101) {
            if (addr > STACK_ADDR) {
                printk("ERROR: attempting to store addr %x permission error: dying (reason=%b)\n", addr, fs);
                clean_reboot();
            } else {
                printk("section permission fault: %x\n", addr);
                printk("changed permissions to r/w: %x\n", addr);
                mmu_mark_sec_rw(proc.pt, addr, 1);
                staff_mmu_sync_pte_mods();
            }
            
        }
    }
    
}
