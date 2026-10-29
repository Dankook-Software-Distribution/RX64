// x86_64: the kernel GDT (riscv has no segmentation).
// In 64-bit mode the CPU ignores base and limit of code and data
// segments, so a descriptor only says code or data, the privilege
// level (DPL) and, for code, L = 64-bit. The slot order is fixed
// by syscall/sysret; see the selectors in x86.h.

#include "types.h"
#include "param.h"
#include "x86.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// the initial GDT contents. Each CPU has its own GDT in struct
// cpu, because each needs its own TSS descriptor, and seginit()
// copies this template into it (as Linux's per-CPU gdt_page
// starts from the same initializer on every CPU).
static const uint64 gdt_template[NSEGS] = {
  // [0] is the null descriptor, required by the CPU.
  [KCSEG >> 3] = 0x00209A0000000000, // P, DPL 0, code, read, L
  [KDSEG >> 3] = 0x0000920000000000, // P, DPL 0, data, write
  [UDSEG >> 3] = 0x0000F20000000000, // P, DPL 3, data, write
  [UCSEG >> 3] = 0x0020FA0000000000, // P, DPL 3, code, read, L
  // the last two slots are the 16-byte TSS descriptor, which
  // seginit() fills in with this CPU's TSS address.
};

// fill in c's TSS and the GDT slots that describe it.
// x86_64: the TSS descriptor (SDM Vol. 3A Figure 10-4). A
// system descriptor is 16 bytes in 64-bit mode, so it takes two
// GDT slots: the low one is laid out like a code or data
// descriptor with base[31:0] and the limit, the high one holds
// base[63:32]. Type 9 is an available 64-bit TSS; ltr changes
// it to 0xB (busy). An io_bitmap_base beyond the limit means
// there is no I/O bitmap, so user in/out raise #GP. Named after
// Linux's set_tss_desc().
static void
set_tss_desc(struct cpu *c)
{
  uint64 base = (uint64)&c->tss;
  uint64 limit = sizeof(c->tss) - 1;

  c->tss.io_bitmap_base = sizeof(c->tss);

  c->gdt[TSSSEG >> 3] =
    (limit & 0xFFFF) |                 // limit[15:0]
    ((base & 0xFFFFFF) << 16) |        // base[23:0]
    ((uint64)0x9 << 40) |              // type 9: available 64-bit TSS
    ((uint64)1 << 47) |                // P; S = 0 and DPL = 0
    (((limit >> 16) & 0xF) << 48) |    // limit[19:16]; G = 0
    (((base >> 24) & 0xFF) << 56);     // base[31:24]
  c->gdt[(TSSSEG >> 3) + 1] = base >> 32; // base[63:32]
}

// set up and load this CPU's GDT: copy the template into
// mycpu()->gdt, add the TSS descriptor, lgdt it, then ltr. The
// boot GDT in entry.S sits at a physical address that only the
// identity map covers, so switch to gdt's high address before
// that map goes away. CS is not reloaded: KCSEG and its
// descriptor are the same in both GDTs, and ds, es and ss stay
// null (Linux's switch_to_new_gdt() also only does lgdt).
// ltr must follow lgdt, since it reads the TSS descriptor from
// the GDT just loaded; the boot GDT has no slot 5. It runs once
// per CPU, as Linux's cpu_init() does through load_TR_desc().
void
seginit(void)
{
  struct cpu *c;
  c = mycpu();
  memmove(c->gdt, gdt_template, sizeof(c->gdt));
  set_tss_desc(c);
  lgdt(c->gdt, sizeof(c->gdt));
  ltr(TSSSEG);
}
