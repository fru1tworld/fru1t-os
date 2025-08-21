ENTRY(boot)

SECTIONS {
    . = 0x80200000;
    
    .text : {
        *(.text.boot)
        *(.text)
    }
    
    .rodata : {
        *(.rodata*)
    }
    
    .data : {
        *(.data)
    }
    
    .bss : {
        bss = .;
        *(.bss)
        bss_end = .;
    }
    
    . = ALIGN(16);
    __stack_top = . + 0x10000;  /* 64KB 스택 */

    . = ALIGN(4096);
    __free_ram = .;
    . += 64 * 1024 * 1024; /* 64MB */
    __free_ram_end = .;

}
