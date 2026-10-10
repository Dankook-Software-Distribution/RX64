# x86_64: one entry stub per interrupt vector (riscv: a single
# kernelvec, since scause tells the kernel what happened). The
# CPU jumps to the stub IDT[n] names but does not say which n it
# was, so each stub pushes its own number. Some exceptions also
# make the CPU push an error code; for every other vector the
# stub pushes a fake 0 first, so alltraps always finds trapno and
# err just below rip, as in struct trapframe.
# The stubs are made by the assembler with .macro and .rept, as
# Linux does, instead of xv6's vectors.pl.

#include "kernel/traps.h"

# stub for vector n. The CPU pushes an error code only for these
# exceptions (SDM Vol. 3A Table 6-1); see the "err" marks in
# traps.h.
.macro vector n
vector\n:
        .if !(\n == T_DF || (\n >= T_TS && \n <= T_PF) || \
              \n == T_AC || \n == T_CP)
                pushq $0                # fake error code
        .endif
        pushq $\n                       # trapno
        jmp alltraps
.endm

# one table entry: vector n's address, filled in by the linker.
.macro vecptr n
        .quad vector\n
.endm

        # vector0 .. vector255. "vector i" would pass the letter i,
        # making every label "vectori"; in .altmacro mode, %i
        # passes i's current value instead. i exists only while
        # assembling.
        .text
        .altmacro
        .set i, 0
        .rept 256
                vector %i
                .set i, i + 1
        .endr

        # vectors[n] is the address of vectorn, for the IDT setup in
        # C (extern uint64 vectors[]). The stubs differ in size, so
        # their addresses cannot be computed. i is still 256 from
        # the loop above, so start over at 0.
        .section .rodata
        .balign 8
.globl vectors
vectors:
        .set i, 0
        .rept 256
                vecptr %i
                .set i, i + 1
        .endr
        .noaltmacro
