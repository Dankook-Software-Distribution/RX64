#include "types.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "defs.h"

volatile static int started = 0;

// x86_64: entry.S's high64 calls this on the boot CPU only,
// at the high address with a high-address stack (riscv: start()
// jumped here in supervisor mode on all CPUs).
void
main()
{
  seginit();       // kernel GDT at its high address

  char *test = "\nHello, world!\n";

  while (*test != '\0') {
    outb(COM1, *test);
    test++;
  }

  for (;;) ;
}
