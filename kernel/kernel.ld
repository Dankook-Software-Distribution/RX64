/*
 * x86_64: the kernel runs at a higher-half virtual address but is
 * loaded at a low physical address, so every section has two
 * addresses (riscv used 0x80000000 for both):
 *
 *   VMA (virtual)  KERNLINK = 0xFFFFFFFF80100000, where code runs
 *                  and what symbols resolve to.
 *   LMA (physical) EXTMEM   = 0x100000, where the loader copies it.
 *
 * VMA - LMA == KERNBASE for every section, so V2P/P2V in
 * memlayout.h also work on kernel symbols. Keep the numbers here
 * in sync with memlayout.h; a linker script cannot include it.
 *
 * qemu -kernel loads us as Multiboot with the a.out kludge (flag
 * bit 16): it ignores the ELF headers and copies the file, from
 * the Multiboot header up to edata, to the physical addresses
 * named in that header. So the file must be laid out exactly like
 * memory: check that VirtAddr - Offset is the same for every LOAD
 * segment in `readelf -l kernel/kernel`.
 */
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

/*
 * Only for gdb and readelf. The loader jumps to the entry address
 * in the Multiboot header instead, which is 32-bit code at a
 * physical address.
 */
ENTRY(_entry)

SECTIONS
{
  . = 0xFFFFFFFF80100000; /* KERNLINK */

  /*
   * AT() sets the LMA of .text; the sections after it keep the
   * same VMA - LMA offset automatically.
   */
  .text : AT(0x100000) { /* EXTMEM */
    /*
     * The Multiboot header must be 4-byte aligned and within the
     * first 8KB of the file, so entry.S puts it in its own section
     * and it goes first. KEEP so the linker never drops it.
     */
    KEEP(*(.multiboot))
    *(.text .text.*)
    . = ALIGN(0x1000);
    PROVIDE(etext = .); /* kvmmake maps [etext, end) with PTE_XD */
  }

  .rodata : {
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }

  .data : {
    . = ALIGN(16);
    *(.data .data.*)
  }

  /*
   * End of what is in the file. Multiboot's load_end_addr; the
   * loader does not clear what follows, so entry.S zeroes
   * [edata, end) itself.
   */
  PROVIDE(edata = .);

  .bss : {
    . = ALIGN(16);
    *(.bss .bss.*)
    *(COMMON)
  }

  PROVIDE(end = .); /* kalloc starts here */

  /*
   * Ubuntu gcc emits these. Left unplaced, the linker may put
   * them in a LOAD segment of their own and break the file layout
   * described above.
   */
  /DISCARD/ : {
    *(.note.gnu.property)
    *(.comment)
  }
}
