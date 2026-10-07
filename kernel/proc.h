// Saved registers for kernel context switches.
// x86_64: riscv saved ra and sp; here rip is the return address
// that call pushed on the stack, so swtch pops it into rip and
// saves rsp as it is after the pop. swtch.S uses these offsets.
struct context {
  /*  0 */ uint64 rip;
  /*  8 */ uint64 rsp;

  // callee-saved
  /* 16 */ uint64 rbx;
  /* 24 */ uint64 rbp;
  /* 32 */ uint64 r12;
  /* 40 */ uint64 r13;
  /* 48 */ uint64 r14;
  /* 56 */ uint64 r15;
};
_Static_assert(sizeof(struct context) == 64,
               "context layout must match swtch.S");

// Per-CPU state.
struct cpu {
  struct proc *proc;      // The process running on this cpu, or null.
  struct context context; // swtch() here to enter scheduler().
  int noff;               // Depth of push_off() nesting.
  int intena;             // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

// registers saved on a trap, at the top of the process's kernel
// stack. built from the top down: the CPU pushes ss..rip (and err
// for some vectors), the stub in vectors.S pushes trapno (and a
// zero err otherwise), and alltraps in trapasm.S pushes r15..rax.
// trapret pops it in reverse and finishes with iretq.
//
// x86_64: riscv kept the trapframe in its own page mapped just
// under the trampoline in the user page table, with kernel_satp,
// kernel_sp, kernel_trap and kernel_hartid for uservec to load.
// none of that is needed here: the CPU takes the kernel stack
// from TSS.rsp0 and the handler from the IDT, and cr3 stays the
// same because every user page table also maps the kernel.
// layout follows SDM Vol. 3A Figure 7-9 for rip..ss; trapasm.S
// uses these offsets as numbers, so keep the two in sync.
struct trapframe {
  // pushed by alltraps
  /*   0 */ uint64 rax;
  /*   8 */ uint64 rbx;
  /*  16 */ uint64 rcx;
  /*  24 */ uint64 rdx;
  /*  32 */ uint64 rbp;
  /*  40 */ uint64 rsi;
  /*  48 */ uint64 rdi;
  /*  56 */ uint64 r8;
  /*  64 */ uint64 r9;
  /*  72 */ uint64 r10;
  /*  80 */ uint64 r11;
  /*  88 */ uint64 r12;
  /*  96 */ uint64 r13;
  /* 104 */ uint64 r14;
  /* 112 */ uint64 r15;

  // pushed by the vector stub in vectors.S
  /* 120 */ uint64 trapno;
  /* 128 */ uint64 err;         // pushed by the CPU for some vectors

  // pushed by the CPU
  /* 136 */ uint64 rip;
  /* 144 */ uint16 cs;          // only low 16 bits are meaningful
            uint16 padding[3];
  /* 152 */ uint64 rflags;
  /* 160 */ uint64 rsp;
  /* 168 */ uint64 ss;
};
_Static_assert(sizeof(struct trapframe) == 176,
               "trapframe layout must match trapasm.S");

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state; // Process state
  void *chan;           // If non-zero, sleeping on chan
  int killed;           // If non-zero, have been killed
  int xstate;           // Exit status to be returned to parent's wait
  int pid;              // Process ID

  // wait_lock must be held when using this:
  struct proc *parent; // Parent process

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // top of kstack; x86_64: was a page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};
