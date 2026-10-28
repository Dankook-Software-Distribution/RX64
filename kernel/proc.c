#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "x86.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
// x86_64: the stacks sit just above the RAM mapping at KSTACKBASE
// (riscv: beneath TRAMPOLINE), so an overflow faults on the guard
// page instead of silently overwriting the direct map, like
// Linux's VMAP_STACK. kalloc() returns a kernel virtual address
// (riscv: the physical address, hence its name pa), so the
// variable is named stack and the PTE gets V2P(stack). Stacks
// are writable and never executed: PTE_RW | PTE_XD (riscv:
// PTE_R | PTE_W; x86 has no R bit, and XD forbids execution
// where riscv's X would allow it).
void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    char *stack = kalloc();
    if (stack == 0)
      panic("kalloc");
    uint64 va = KSTACK((int)(p - proc));
    kvmmap(kpgtbl, va, V2P(stack), PGSIZE, PTE_RW | PTE_XD);
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
// x86_64: riscv read the hart id from tp. For now only the boot
// CPU runs (-smp 1) and it is CPU 0, as Linux treats the boot CPU
// before per-CPU data is set up. Replace this when other CPUs
// start: x86 can find the current CPU by its LAPIC ID (through a
// table from APIC ID to cpus[] index) or by gs, and the choice
// waits until then.
int
cpuid()
{
  return 0;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}
