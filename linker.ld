ENTRY(_start)
SECTIONS
{
    . = 0x10000;
    .text : {
        *(.entry)
        *(.text)
        *(.text.*)
    }
    .rodata : { *(.rodata) *(.rodata.*) }
    .data   : { *(.data)   *(.data.*)   }
    .commands : ALIGN(4) {
        __start_commands = .;
        KEEP(*(.commands))
        __stop_commands = .;
    }
    .bss : {
        *(.bss) *(.bss.*) *(COMMON)
    }
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame) }
}