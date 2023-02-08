/* SPDX-License-Identifier: MIT */
ENTRY(_start)
PHDRS
{
	bootstruct	PT_LOAD	FLAGS(6);	/* RW- */
	text	PT_LOAD		FLAGS(5);	/* R-X */
	rodata	PT_LOAD		FLAGS(4);	/* R-- */
	data	PT_LOAD		FLAGS(6);	/* RW- */
}
SECTIONS
{
	. = 0xffffffff80000000;

	__bootstruct_start = .;
	.bootstruct :
	{
		KEEP(*(.bootstruct))
		. = ALIGN(4K);
	} :bootstruct
	__bootstruct_end = .;

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

		. = ALIGN(64); /* Align to a common cacheline size. */
		__cpulocal_start = .;
		*(.cpulocal)
		__cpulocal_end = .;
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

	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
		*(.note*)
	}
}
