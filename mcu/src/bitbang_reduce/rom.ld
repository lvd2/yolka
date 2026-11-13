MEMORY
{
/* 4 kB RAM, split into general-purpose RAM and stack */
	RAM    (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00F00
  	STK    (rw)  : ORIGIN = 0x20000F00, LENGTH = 0x00100
/* 16 kB flash */
	ROM (rx)     : ORIGIN = 0x08000000, LENGTH = 0x04000
}

SECTIONS
{

	.vectors :
	{
		__vectors__ = .;
		KEEP(*(.vectors))
	} >ROM

	.text ALIGN(4) :
	{
		*(.text)
		*(.text*)
		. = ALIGN(4);
		*(.glue_7)
		*(.glue_7t)
		*(.eh_frame)
		*(.rodata)
		*(.rodata*)
		. = ALIGN(4);
	} >ROM
	__etext__ = .;


	.data  : AT (__etext__)
	{
		__data_start__ = .;
		*(.data)
		*(.data*)
		. = ALIGN(4);
		__data_end__ = .;
	} >RAM
	
	.bss ALIGN(4) (NOLOAD) :
	{
		__bss_start__ = . ;
		*(.bss)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = . ;
	} >RAM

	/DISCARD/ : { *(__io_scripts__*) }

	/* stack */
	__stk_canary__ = ORIGIN(STK); /* stack canary */
	__stk_top__    = ORIGIN(STK)+LENGTH(STK);
}

