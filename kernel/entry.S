#include "kernel/x86.h"
#include "kernel/memlayout.h"
#include "kernel/param.h"

        # x86_64: qemu -kernel loads us as a Multiboot kernel
        # (riscv: qemu jumped to 0x80000000 in M-mode, 64-bit).
        # Flag bit 16 makes qemu use the addresses below instead
        # of the ELF headers, so the 64-bit ELF loads. All
        # addresses are physical, hence V2P_WO.
        # The header must be 4-byte aligned in the first 8KB of
        # the file; kernel.ld puts .multiboot first.
        .section .multiboot, "a"
        .align 4
mbhdr:
        .long 0x1BADB002                # magic
        .long 1<<16                     # flags: address fields valid
        .long -(0x1BADB002 + (1<<16))   # checksum: magic + flags + checksum == 0
        .long V2P_WO(mbhdr)             # header_addr: where this header lands
        .long V2P_WO(mbhdr)             # load_addr: start loading here
        .long V2P_WO(edata)             # load_end_addr: end of .data
        .long V2P_WO(end)               # bss_end_addr: end of .bss
        .long V2P_WO(_entry)            # entry_addr: jump here after loading

        # x86_64: Multiboot enters in 32-bit protected mode with
        # paging off, EAX = 0x2BADB002, EBX = Multiboot info,
        # interrupts off. ESP and GDTR are undefined, so set up a
        # stack before any call/push and a GDT before reloading
        # segment registers.
        .text
        .code32
.globl _entry
_entry:
        # print 'A' on COM1 to show we got here. qemu's UART
        # needs no initialization to transmit.
        movw $COM1, %dx
        movb $'A', %al
        outb %al, %dx

        # x86_64: zero [edata, end). qemu's Multiboot loader copies
        # the file only up to edata and leaves .bss as is (riscv:
        # qemu loaded the ELF, which covered .bss).
        cld
        movl $V2P_WO(edata), %edi
        xorl %eax, %eax
        movl $V2P_WO(end), %ecx
        subl $V2P_WO(edata), %ecx
        rep stosb

        # set up a stack for C. after the bss loop, because the
        # stack lives in .bss. ebp = 0 ends the frame-pointer chain
        # for backtraces.
        movl $V2P_WO(bootstacktop), %esp
        xorl %ebp, %ebp

spin:
        hlt
        jmp spin

        # x86_64: stack for the boot CPU only (riscv: stack0, one
        # per CPU). it stays CPU 0's scheduler stack after boot.
        # other CPUs get kalloc'd stacks when they are started.
        # aligned to its size so a guard page can go below it.
        .bss
        .balign KSTACKSIZE
bootstack:
        .space KSTACKSIZE
bootstacktop:

        # x86_64: boot page table, used only until kvminit() builds
        # the real one (riscv: paging stayed off until kvminithart).
        # Maps physical 0..1GB twice with 2MB pages:
        #   VA 0         -> PA 0   identity, so the code keeps running
        #                          at its low address when paging turns on
        #   VA KERNBASE  -> PA 0   where the kernel is linked
        # Entries hold physical addresses, hence V2P_WO. Filled in at
        # link time and placed in .data so Multiboot loads it.
        .data
        .balign 4096
pml4:
        .quad V2P_WO(pdpt) + PTE_P + PTE_RW     # [0]   VA 0 .. 512GB
        .space (511 - 1) * 8                    # [1..510] not present
        .quad V2P_WO(pdpt) + PTE_P + PTE_RW     # [511] top 512GB, holds KERNBASE

        .balign 4096
pdpt:
        .quad V2P_WO(pd) + PTE_P + PTE_RW       # [0]   VA 0 .. 1GB
        .space (511 - 2) * 8                    # [1..509] not present
        .quad V2P_WO(pd) + PTE_P + PTE_RW       # [510] KERNBASE .. +1GB
        .space 8                                # [511] not present

        .balign 4096
pd:
        # 512 2MB pages: entry i maps PA i * 2MB.
        .set pd_i, 0
        .rept 512
        .quad (pd_i << 21) + PTE_PS + PTE_P + PTE_RW
        .set pd_i, pd_i + 1
        .endr
