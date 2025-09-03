ENTRY(boot)

SECTIONS {
  . = 0x80200000;

  .text.boot : {
    *(.text.boot)
  }

  .text : {
    *(.text)
    *(.text.*)
  }

  .rodata : {
    *(.rodata)
    *(.rodata.*)
  }

  .data : {
    *(.data)
    *(.data.*)
  }

  bss = .;
  .bss : {
    *(.bss)
    *(.bss.*)
    *(COMMON)
  }
  bss_end = .;

  . = ALIGN(4096);
  __free_ram = .;
  __free_ram_end = . + 128 * 1024 * 1024; /* 128MB */

  __stack_top = 0x80400000;
}