#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "x86.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "traps.h"

// x86_64: every interrupt, exception and system call comes here,
// from the kernel or from user space alike (riscv: usertrap() via
// trampoline.S and kerneltrap() via kernelvec.S). alltraps in
// trapasm.S builds the trapframe on the kernel stack and passes
// it in tf; returning goes back through trapret and iretq.
// For now only kernel exceptions can happen, and each one is a
// bug, so print the trap and stop, as kerneltrap() panics on an
// unknown trap. Returning would rerun a faulting instruction
// (#PF, #GP) forever. Interrupts, system calls and user faults
// come in later steps.
void
trap(struct trapframe *tf)
{
  earlyputs("trap ");
  earlyputx(tf->trapno);
  earlyputs(" err ");
  earlyputx(tf->err);
  earlyputs(" rip ");
  earlyputx(tf->rip);
  if (tf->trapno == T_PF) {
    earlyputs(" cr2 ");
    earlyputx(read_cr2());
  }
  earlyputs("\n");

  for (;;)
    ;
}
