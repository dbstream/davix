/* SPDX-License-Identifier: MIT */
ENTRY(_start)
PHDRS
{
	text	PT_LOAD		FLAGS(5);	/* R-X */
	rodata	PT_LOAD		FLAGS(4);	/* R-- */
	data	PT_LOAD		FLAGS(6);	/* RW- */
}
SECTIONS
{
	. = 0x0000000001000000;	/* 16 MiB */

	__loader_start = .;
	__boot_start = .;
	.boot :
	{
		*(.boot)
		. = ALIGN(4K);
		*(.boot_data)
		. = ALIGN(4K);
	} :text
	__boot_end = .;

	__text_start = .;
	.text :
	{
		*(.text)
		. = ALIGN(4K);
	} :text
	__text_end = .;

	__rodata_start = .;
	.rodata :
	{
		*(.rodata*)
		. = ALIGN(4K);
	} :rodata
	__rodata_end = .;

	__data_start = .;
	.data :
	{
		*(.data)
	} :data
	__data_end = .;

	__bss_start = .;
	.bss :
	{
		*(COMMON)
		*(.bss*)
		. = ALIGN(4K);
	} :data
	__bss_end = .;
	__loader_end = .;

	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
		*(.note*)
	}
}
