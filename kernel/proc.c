#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "x86.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
// x86_64: only a stub for now, so that allocproc() can point
// new contexts at it. 5-c-3 fills it in: release the p->lock
// that scheduler() took, then start running the process.
void
forkret(void)
{
  panic("forkret");
}

struct cpu cpus[NCPU];

struct proc proc[NPROC];

int nextpid = 1;
struct spinlock pid_lock;

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

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

void
procinit(void)
{
  struct proc *p;

  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");

  for (p = proc; p < &proc[NPROC]; p++) {
    initlock(&p->lock, "proc");
    p->state = UNUSED;
    p->kstack = KSTACK((int)(p - proc));
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

int
allocpid()
{
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid += 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, return it with p->lock held; otherwise return 0.
// x86_64: split out of allocproc() (riscv: a loop with goto found).
static struct proc *
find_unused(void)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->state == UNUSED)
      return p;
    release(&p->lock);
  }

  return 0;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, return 0.
// x86_64: no user page table yet; proc_pagetable() comes back
// in step 6.
static struct proc *
allocproc(void)
{
  struct proc *p = find_unused();
  if (p == 0)
    return 0;

  p->pid = allocpid();
  p->state = USED;

  // x86_64: the trapframe is the top of the kernel stack
  // (riscv: a page of its own from kalloc(), so that
  // trampoline.S could reach it). On a trap from user mode the
  // CPU switches to the TSS rsp0, which the scheduler sets to
  // p->kstack + PGSIZE, and pushes the frame from there down.
  // It belongs to the stack page, so freeproc() must not
  // kfree() it.
  p->trapframe = (struct trapframe *)(p->kstack + PGSIZE) - 1;

  // Set up new context to start executing at forkret.
  // x86_64: swtch() enters forkret with jmp, which pushes no
  // return address, so leave an 8-byte slot below the trapframe
  // in its place. That gives rsp % 16 == 8 at function entry, as
  // the ABI expects (the trapframe is 16-byte aligned since
  // sizeof(struct trapframe) is 176). Step 7 puts trapret's
  // address in the slot, so forkret returns into trapret.
  memset(&p->context, 0, sizeof(p->context));
  p->context.rsp = (uint64)(p->trapframe) - 8;
  p->context.rip = (uint64)forkret;
  return p;
}

// TEST(5-c-1): allocate two procs and print what allocproc()
// set up. Expect pid 1 and 2, kstack KSTACK(0) and KSTACK(1),
// the trapframe 176 bytes below each stack top, rip at forkret,
// and rsp 8 bytes below the trapframe. Remove with 5-c-3.
void
allocproc_test(void)
{
  struct proc *p;

  for (int i = 0; i < 2; i++) {
    if ((p = allocproc()) == 0)
      panic("allocproc_test");
    printk("pid %d kstack %p trapframe %p rip %p rsp %p\n",
           p->pid, (void *)p->kstack, p->trapframe,
           (void *)p->context.rip, (void *)p->context.rsp);
    release(&p->lock);
  }
}
