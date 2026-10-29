# Context switch
#
#   void swtch(struct context *old, struct context *new);
#
# Save current registers in old. Load from new.
#
# x86_64: old arrives in rdi and new in rsi. Only the callee-saved
# registers (rbx, rbp, r12-r15) plus rsp and rip are saved; the
# System V ABI lets swtch clobber the rest, so its caller has
# already saved what it needs (riscv: ra, sp and s0-s11).
# There is no ra register: the return address that call pushed
# is popped into old->rip, so old->rsp is the stack as it was
# before the call, and switching back to old later looks like an
# ordinary return from swtch. new is resumed with an indirect jmp
# to new->rip rather than ret, since its return address is in the
# context and not on its stack. (xv6-public and Linux push the
# registers on the stack, save only rsp, and end in ret.)

.globl swtch
swtch:
        movq %rbx, 16(%rdi)
        movq %rbp, 24(%rdi)
        movq %r12, 32(%rdi)
        movq %r13, 40(%rdi)
        movq %r14, 48(%rdi)
        movq %r15, 56(%rdi)
        popq %rax
        movq %rax, 0(%rdi)
        movq %rsp, 8(%rdi)

        movq 16(%rsi), %rbx
        movq 24(%rsi), %rbp
        movq 32(%rsi), %r12
        movq 40(%rsi), %r13
        movq 48(%rsi), %r14
        movq 56(%rsi), %r15
        movq 8(%rsi), %rsp
        jmp *0(%rsi)
