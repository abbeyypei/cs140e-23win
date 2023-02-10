@ hacked code for testing vector base code and testing the
@ difference between the position-independent "generic" method
@ of last lab (that used the ldr instruction) with the faster
@ method of a relative branch (so: no memory load, no cache miss,
@ also, the ldr will commonly = a misprediction).
#include "rpi-asm.h"

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ support to do a single system call, <sys_plus_1>

@ our trivial, kernel-level single system call implementation
@ that adds 1 to <r0> and returns.
@
@ this is about the minimum overhead for a call we can
@ have since there's no (1) saving/restoring of registers,
@ (2) additional jumps or (3) sanity checking.
@
@ NOTE: we will trash SUPER mode's <lr> so the caller 
@ has to push{lr}/pop{lr}
@
MK_FN(sys_plus1_handler)
    add r0, r0, #1
    movs pc, lr

@ trivial "user-level" client callsite that invokes 
@ <sys_plus1_handler> system call using a SWI 
@ instruction. 
@
@ NOTE: we are running at SUPER level today, as is the SWI handler:
@ thus we have to push{lr}/pop{lr} since otherwise the swi
@ exception would trash these.
@
@ for speed: you can also inline this, just be sure to specify that
@ the code clobbers all the caller (including lr)
MK_FN(sys_plus1)
    push {lr}
    swi 1
    pop {lr}
    bx lr

@ the <unhandled_*> trampolines are in 
@       <staff-src/unhandled-exception.S>
_reset_asm:                   .word unhandled_reset
_undefined_instruction_asm:   .word unhandled_undefined_instruction
_software_interrupt_asm:      .word sys_plus1_handler
_prefetch_abort_asm:          .word unhandled_prefetch_abort
_data_abort_asm:              .word unhandled_data_abort
_interrupt_asm:               .word unhandled_interrupt

data_abort_asm:
  mov sp, #INT_STACK_ADDR   @ i believe we have 512mb - 16mb, so this should be safe
  sub   lr, lr, #8

  push  {r0-r12,lr}         @ XXX: pushing too many registers: only need caller
  @ vpush {s0-s15}	        @ uncomment if want to save caller-saved fp regs

  mov   r0, lr              @ Pass old pc
  bl    data_abort_vector    @ C function

  @ vpop {s0-s15}           @ pop caller saved fp regs
  pop   {r0-r12,lr} 	    @ pop integer registers

  @ return from interrupt handler: will re-enable general ints.
  movs    pc, lr        @ moves the link register into the pc and implicitly
                        @ loads the PC with the result, then copies the 
                        @ SPSR to the CPSR.

prefetch_abort_asm:
  mov sp, #INT_STACK_ADDR   @ i believe we have 512mb - 16mb, so this should be safe
  sub   lr, lr, #4

  push  {r0-r12,lr}         @ XXX: pushing too many registers: only need caller
  @ vpush {s0-s15}	        @ uncomment if want to save caller-saved fp regs

  mov   r0, lr              @ Pass old pc
  bl    prefetch_abort_vector    @ C function

  @ vpop {s0-s15}           @ pop caller saved fp regs
  pop   {r0-r12,lr} 	    @ pop integer registers

  @ return from interrupt handler: will re-enable general ints.
  movs    pc, lr        @ moves the link register into the pc and implicitly
                        @ loads the PC with the result, then copies the 
                        @ SPSR to the CPSR.

.globl _interrupt_vector
.align 5
_interrupt_vector:
  ldr pc, =_reset_asm
  ldr pc, =_undefined_instruction_asm
  ldr pc, =_software_interrupt_asm
  ldr pc, = prefetch_abort_asm
  ldr pc, = data_abort_asm
  ldr pc, =_reset_asm
  ldr pc, =_interrupt_asm
