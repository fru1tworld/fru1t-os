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
}
