#include "types.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  for (;;) ;
}
