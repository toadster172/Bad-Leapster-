OUTPUT_FORMAT(elf32-littlearc)

MEMORY {
    ROM (rx)  : ORIGIN = 0x80000000, LENGTH = 16M /* Cart */
    /* TODO: Check actual size of static area */
    RAM (rwx) : ORIGIN = 0x3C000300, LENGTH = 3K
}

/* ENTRY(entry) */

SECTIONS {
    .text (ORIGIN(ROM) + 16K) : {
        *(.text)
        *(.text.*)

        TEXT_END = .;
    }

    .rodata (TEXT_END) : {
        *(.rodata)
        *(.rodata.*)
    }
}

SECTIONS {
    .bss : {
        *(.bss)
        *(.bss.*)
    }
}
