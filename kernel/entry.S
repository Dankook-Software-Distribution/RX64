#include "kernel/x86.h"
#include "kernel/memlayout.h"

        .section .multiboot, "a"
        .align 4
mbhdr:
        .long 0x1BADB002                # magic number
        .long 1<<16                     # flags
        .long -(0x1BADB002 + (1<<16))   # checksum: magic number + flags + checksum = 0
        .long V2P_WO(mbhdr)             # header_addr: where this header lands
        .long V2P_WO(mbhdr)             # load_addr: start loading here
        .long V2P_WO(edata)             # load_end_addr: end of .data
        .long V2P_WO(end)               # bss_end_addr: end of .bss
        .long V2P_WO(_entry)            # entry_addr: jump here after loading
        
        .text
        .code32
        .global _entry
_entry:
        # nothing to do yet
spin:
        hlt
        jmp spin
