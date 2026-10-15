// x86_64: early console output, for use before printk and the
// console driver are up (riscv: none; printk was up from the
// start). Like Linux's early_printk, it writes straight to the
// COM1 UART with no lock and no buffer, so it also works from
// trap handlers and panics. A real 16550 needs a wait for LSR.THRE
// before each byte; qemu accepts bytes at any time, so it is left
// out.

#include "types.h"
#include "x86.h"
#include "memlayout.h"

static char digits[] = "0123456789abcdef";

// print one character. Takes an int like consputc(), so that
// printk.c can use it in consputc()'s place for now.
void
earlyputc(int c)
{
  outb(COM1, c);
}

// print a NUL-terminated string.
void
earlyputs(char *s)
{
  while (*s) {
    earlyputc(*s);
    s++;
  }
}

// print x as 0x followed by 16 hex digits, most significant
// first, as xv6's printptr() does: take the top 4 bits, then
// shift the next digit up.
void
earlyputx(uint64 x)
{
  earlyputs("0x");
  for (int i = 0; i <= 15; i++) {
    outb(COM1, digits[x >> 60]);
    x = x << 4;
  }
}
