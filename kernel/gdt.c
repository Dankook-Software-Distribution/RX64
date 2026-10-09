// x86_64: the kernel GDT (riscv has no segmentation).
// In 64-bit mode the CPU ignores base and limit of code and data
// segments, so a descriptor only says code or data, the privilege
// level (DPL) and, for code, L = 64-bit. The slot order is fixed
// by syscall/sysret; see the selectors in x86.h.

#include "types.h"
#include "x86.h"

// one GDT for now, since only the boot CPU runs. It moves into
// struct cpu when each CPU gets its own TSS.
static uint64 gdt[NSEGS] = {
  // [0] is the null descriptor, required by the CPU.
  [KCSEG >> 3] = 0x00209A0000000000, // P, DPL 0, code, read, L
  [KDSEG >> 3] = 0x0000920000000000, // P, DPL 0, data, write
  [UDSEG >> 3] = 0x0000F20000000000, // P, DPL 3, data, write
  [UCSEG >> 3] = 0x0020FA0000000000, // P, DPL 3, code, read, L
  // the last two slots are the 16-byte TSS descriptor, filled in
  // once there is a TSS.
};

// load this CPU's GDT. The boot GDT in entry.S sits at a physical
// address that only the identity map covers, so switch to gdt's
// high address before that map goes away. CS is not reloaded:
// KCSEG and its descriptor are the same in both GDTs, and ds, es
// and ss stay null (Linux's switch_to_new_gdt() also only does
// lgdt).
void
seginit(void)
{
  lgdt(gdt, sizeof(gdt));
}
