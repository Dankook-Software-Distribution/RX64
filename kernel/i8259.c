// x86_64: the legacy 8259A interrupt controllers (riscv has
// none). A PC still has two of them, master and slave, for IRQs
// 0-15, and the BIOS hands them over set up for DOS: IRQ 0-7 go
// to vectors 8-15, on top of CPU exceptions such as #DF and #GP.
// The kernel takes interrupts through the LAPIC and IOAPIC
// instead, so the only job here is to mask every 8259 IRQ,
// as Linux's mask_8259A() does.

#include "types.h"
#include "x86.h"

// interrupt mask registers (I/O ports); a 1 bit masks that IRQ.
// Names follow Linux's asm/i8259.h.
#define PIC_MASTER_IMR 0x21
#define PIC_SLAVE_IMR  0xA1

// mask all IRQs on both 8259s. Requests still latch in the IRR,
// but none reaches the CPU. Called before interrupts are enabled.
void
picinit(void)
{
  outb(PIC_MASTER_IMR, 0xFF);
  outb(PIC_SLAVE_IMR, 0xFF);
}
