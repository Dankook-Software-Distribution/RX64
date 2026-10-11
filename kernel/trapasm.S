        # x86_64: common trap entry and exit (riscv: kernelvec for
        # traps from the kernel, uservec/userret in trampoline.S for
        # traps from user space). Every vector stub in vectors.S
        # jumps to alltraps, from the kernel or from user space
        # alike, with the stack already holding the top of a
        # struct trapframe:
        #
        #   ss, rsp, rflags, cs, rip    pushed by the CPU
        #   err                         by the CPU, or a fake 0 by the stub
        #   trapno                      by the stub   <- rsp
        #
        # alltraps pushes the general registers below that to
        # complete the trapframe and calls trap(tf); trapret undoes
        # it and returns with iretq.

        .text
.globl alltraps
alltraps:
        # struct trapframe starts with rax at offset 0, and the
        # stack grows down, so push in the reverse of field order:
        # r15 first, rax last.
        pushq %r15
        pushq %r14
        pushq %r13
        pushq %r12
        pushq %r11
        pushq %r10
        pushq %r9
        pushq %r8
        pushq %rdi
        pushq %rsi
        pushq %rbp
        pushq %rdx
        pushq %rcx
        pushq %rbx
        pushq %rax

        # rsp now points at the complete trapframe; pass it as the
        # first argument. In 64-bit mode the CPU aligns rsp to 16
        # bytes before pushing its frame, and the 176-byte trapframe
        # is a multiple of 16, so rsp is aligned for the call as the
        # SysV ABI requires.
        movq %rsp, %rdi
        call trap

        # trap() returns here. A new process will also start here
        # on its first return to user space.
.globl trapret
trapret:
        popq %rax
        popq %rbx
        popq %rcx
        popq %rdx
        popq %rbp
        popq %rsi
        popq %rdi
        popq %r8
        popq %r9
        popq %r10
        popq %r11
        popq %r12
        popq %r13
        popq %r14
        popq %r15

        # drop trapno and err, leaving rsp at the CPU's frame.
        addq $16, %rsp

        # pop rip, cs, rflags, rsp and ss. iretq, not iret: iret
        # would pop 4-byte values.
        iretq
