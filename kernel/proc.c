#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "x86.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

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
