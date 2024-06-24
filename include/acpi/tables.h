/**
 * ACPI table definitions.
 * Copyright (C) 2024  dbstream
 *
 * NOTE: unless otherwise stated, all values are stored as little-endian.
 */
#ifndef _ACPI_TABLES_H
#define _ACPI_TABLES_H 1

#include <davix/stdint.h>

#define _ACPI_BIT(n)			(UINT32_C(1) << n)

#define ACPI_SIG_RSDP	"RSD PTR "
#define ACPI_SIG_RSDT	"RSDT"
#define ACPI_SIG_XSDT	"XSDT"
#define ACPI_SIG_FADT	"FACP"
#define ACPI_SIG_MADT	"APIC"
#define ACPI_SIG_HPET	"HPET"

struct acpi_subtable_header;
struct acpi_table_header;

extern void
acpi_parse_table (struct acpi_table_header *header, unsigned long offset,
	int (*callback)(struct acpi_subtable_header *, void *), void *p);

extern void *
acpi_find_table (const char *sig, unsigned long idx);

static inline uint8_t
acpi_compute_checksum (void *table, unsigned long n)
{
	uint8_t x = 0;
	uint8_t *p = table;
	for (; n; p++, n--)
		x += *p;
	return x;
}

struct __attribute__((packed)) acpi_gas {
	uint8_t		address_space;
	uint8_t		bit_width;
	uint8_t		bit_offset;
	uint8_t		access_size;
	le64_t		address;
};

#define ACPI_ADDRESS_SPACE_MEMORY		0
#define ACPI_ADDRESS_SPACE_IO			1
#define ACPI_ADDRESS_SPACE_PCI_CONF		2
#define ACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER	3
#define ACPI_ADDRESS_SPACE_SMBUS		4
#define ACPI_ADDRESS_SPACE_CMOS			5
#define ACPI_ADDRESS_SPACE_PCI_BAR		6
#define ACPI_ADDRESS_SPACE_IPMI			7
#define ACPI_ADDRESS_SPACE_GPIO			8
#define ACPI_ADDRESS_SPACE_GENERIC_SERIAL_BUS	9
#define ACPI_ADDRESS_SPACE_PCC			10
#define ACPI_ADDRESS_SPACE_PRM			11
#define ACPI_ADDRESS_SPACE_FUNC_FIXED_HW	127

#define ACPI_ACCESS_SIZE_BYTE			1
#define ACPI_ACCESS_SIZE_WORD			2
#define ACPI_ACCESS_SIZE_DWORD			3
#define ACPI_ACCESS_SIZE_QWORD			4

/**
 * Some ACPI tables consist of "subtables", which have the following
 * structure:
 */
struct __attribute__((packed)) acpi_subtable_header {
	uint8_t		type;
	uint8_t		length;
};

struct __attribute__((packed)) acpi_table_rsdp {
	char		signature[8];
	uint8_t		checksum;
	char		oemid[6];
	uint8_t		revision;
	le32_t		rsdt_address;
	le32_t		length;
	le64_t		xsdt_address;
	uint8_t		ext_checksum;
	char		reserved[3];
};

struct __attribute__((packed)) acpi_table_header {
	char		signature[4];
	le32_t		length;
	uint8_t		revision;
	uint8_t		checksum;
	char		oemid[6];
	char		oem_table_id[8];
	le32_t		oem_revision;
	le32_t		creator_id;
	le32_t		creator_revision;
};

/* This is not an ACPI table. It is a "cache entry" for ACPI table headers. */
struct acpi_table_list_entry {
	struct acpi_table_header	header;
	unsigned long			address;
};

/* We store ACPI tables in an internal array for easy access. */
extern struct acpi_table_list_entry	acpi_tables[];
extern unsigned long			num_acpi_tables;

struct __attribute__((packed)) acpi_table_rsdt {
	struct acpi_table_header	header;
	le32_t				entries[];
};

struct __attribute__((packed)) acpi_table_xsdt {
	struct acpi_table_header	header;
	le64_t				entries[];
};

struct __attribute__((packed)) acpi_table_fadt {
	struct acpi_table_header	header;
	le32_t				firmware_ctrl;
	le32_t				dsdt;
	char				reserved1;
	uint8_t				preferred_pm_profile;
	le16_t				sci_int;
	le32_t				smi_cmd;
	uint8_t				acpi_enable;
	uint8_t				acpi_disable;
	uint8_t				s4bios_req;
	uint8_t				pstate_cnt;
	le32_t				pm1a_evt_blk;
	le32_t				pm1b_evt_blk;
	le32_t				pm1a_cnt_blk;
	le32_t				pm1b_cnt_blk;
	le32_t				pm2_cnt_blk;
	le32_t				pm_timer_blk;
	le32_t				gpe0_blk;
	le32_t				gpe1_blk;
	uint8_t				pm1_evt_len;
	uint8_t				pm1_cnt_len;
	uint8_t				pm2_cnt_len;
	uint8_t				pm_timer_len;
	uint8_t				gpe0_blk_len;
	uint8_t				gpe1_blk_len;
	uint8_t				gpe1_base;
	uint8_t				cst_cnt;
	le16_t				p_lvl2_lat;
	le16_t				p_lvl3_lat;
	le16_t				flush_size;
	le16_t				flush_stride;
	uint8_t				duty_offset;
	uint8_t				duty_width;
	uint8_t				day_alarm;
	uint8_t				mon_alarm;
	uint8_t				century;
	le16_t				iapc_boot_arch;
	char				reserved2;
	le32_t				flags;
	struct acpi_gas			reset_reg;
	uint8_t				reset_value;
	le16_t				arm_boot_arch;
	uint8_t				fadt_minor_version;
	le64_t				x_firmware_ctrl;
	le64_t				x_dsdt;
	struct acpi_gas			x_pm1a_evt_blk;
	struct acpi_gas			x_pm1b_evt_blk;
	struct acpi_gas			x_pm1a_cnt_blk;
	struct acpi_gas			x_pm1b_cnt_blk;
	struct acpi_gas			x_pm2_cnt_blk;
	struct acpi_gas			x_pm_timer_blk;
	struct acpi_gas			x_gpe0_blk;
	struct acpi_gas			x_gpe1_blk;
	struct acpi_gas			sleep_control_reg;
	struct acpi_gas			sleep_status_reg;
	le64_t				hypervisor_id;
};

extern struct acpi_table_fadt acpi_fadt;

#define ACPI_FADT_WBINVD		_ACPI_BIT(0)
#define ACPI_FADT_WBINVD_FLUSH		_ACPI_BIT(1)
#define ACPI_FADT_PROC_C1		_ACPI_BIT(2)
#define ACPI_FADT_P_LVL2_UP		_ACPI_BIT(3)
#define ACPI_FADT_PWR_BUTTON		_ACPI_BIT(4)
#define ACPI_FADT_SLP_BUTTON		_ACPI_BIT(5)
#define ACPI_FADT_FIX_RTC		_ACPI_BIT(6)
#define ACPI_FADT_RTC_S4		_ACPI_BIT(7)
#define ACPI_FADT_TMR_VAL_EXT		_ACPI_BIT(8)
#define ACPI_FADT_DCK_CAP		_ACPI_BIT(9)
#define ACPI_FADT_RESET_REG_SUP		_ACPI_BIT(10)
#define ACPI_FADT_SEALED_CASE		_ACPI_BIT(11)
#define ACPI_FADT_HEADLESS		_ACPI_BIT(12)
#define ACPI_FADT_CPU_SW_SLP		_ACPI_BIT(13)
#define ACPI_FADT_PCI_EXP_WAK		_ACPI_BIT(14)
#define ACPI_FADT_USE_PLATFORM_CLOCK	_ACPI_BIT(15)
#define ACPI_FADT_S4_RTC_STS_VALID	_ACPI_BIT(16)
#define ACPI_FADT_REMOTE_POWER_ON	_ACPI_BIT(17)
#define ACPI_FADT_FORCE_APIC_CLUSTER	_ACPI_BIT(18)
#define ACPI_FADT_FORCE_APIC_PHYSICAL	_ACPI_BIT(19)
#define ACPI_FADT_HW_REDUCED		_ACPI_BIT(20)
#define ACPI_FADT_LOW_POWER_S0_IDLE	_ACPI_BIT(21)
#define ACPI_FADT_PERSISTENT_CPU_CACHES(x) (((x) >> 22) & 3)

#define ACPI_IAPC_LEGACY_DEVICES	_ACPI_BIT(0)
#define ACPI_IAPC_8042			_ACPI_BIT(1)
#define ACPI_IAPC_VGA_NOT_PRESENT	_ACPI_BIT(2)
#define ACPI_IAPC_MSI_NOT_SUPPORTED	_ACPI_BIT(3)
#define ACPI_IAPC_PCIE_ASPM_CONTROLS	_ACPI_BIT(4)
#define ACPI_IAPC_CMOS_RTC_NOT_PRESENT	_ACPI_BIT(5)

#define ACPI_ARM_PSCI_COMPLIANT		_ACPI_BIT(0)
#define ACPI_ARM_PSCI_USE_HVC		_ACPI_BIT(1)

struct __attribute__((packed)) acpi_table_madt {
	struct acpi_table_header	header;
	le32_t				local_apic_addr;
	le32_t				flags;
};

#define ACPI_MADT_PCAT_COMPAT		_ACPI_BIT(0)

extern void
acpi_parse_table_madt (struct acpi_table_madt *madt,
	int (*callback)(struct acpi_subtable_header *, void *), void *p);

#define ACPI_MADT_LOCAL_APIC		0
#define ACPI_MADT_IO_APIC		1
#define ACPI_MADT_INTERRUPT_OVERRIDE	2
#define ACPI_MADT_NMI_SOURCE		3
#define ACPI_MADT_LOCAL_APIC_NMI	4
#define ACPI_MADT_LAPIC_ADDR_OVERRIDE	5
#define ACPI_MADT_LOCAL_X2APIC		9
#define ACPI_MADT_LOCAL_X2APIC_NMI	10
#define ACPI_MADT_SMP_WAKEUP		16

#define ACPI_MADT_LAPIC_ENABLED		_ACPI_BIT(0)
#define ACPI_MADT_LAPIC_ONLINE_CAPABLE	_ACPI_BIT(1)

struct __attribute__((packed)) acpi_madt_local_apic {
	struct acpi_subtable_header	header;
	uint8_t				acpi_processor_uid;
	uint8_t				apic_id;
	le32_t				flags;
};

struct __attribute__((packed)) acpi_madt_io_apic {
	struct acpi_subtable_header	header;
	uint8_t				ioapic_id;
	uint8_t				reserved;
	le32_t				ioapic_addr;
	le32_t				gsi_base;
};

struct __attribute__((packed)) acpi_madt_interrupt_override {
	struct acpi_subtable_header	header;
	uint8_t				bus;
	uint8_t				source_irq;
	le32_t				gsi_irq;
	le16_t				flags;
};

struct __attribute__((packed)) acpi_madt_nmi_source {
	struct acpi_subtable_header	header;
	le16_t				flags;
	le32_t				gsi_irq;
};

struct __attribute__((packed)) acpi_madt_local_apic_nmi {
	struct acpi_subtable_header	header;
	uint8_t				acpi_processor_uid;
	le16_t				flags;
	uint8_t				lint_num;
};

struct __attribute__((packed)) acpi_madt_lapic_addr_override {
	struct acpi_subtable_header	header;
	le16_t				reserved;
	le64_t				address;
};

struct __attribute__((packed)) acpi_madt_local_x2apic {
	struct acpi_subtable_header	header;
	char				reserved[2];
	le32_t				apic_id;
	le32_t				flags;
	le32_t				acpi_processor_uid;
};

struct __attribute__((packed)) acpi_madt_local_x2apic_nmi {
	struct acpi_subtable_header	header;
	le16_t				flags;
	le32_t				acpi_processor_uid;
	uint8_t				lint_num;
	char				reserved[3];
};

struct __attribute__((packed)) acpi_table_hpet {
	struct acpi_table_header	header;
	uint32_t			evtt_blk_id;
	struct acpi_gas			address;
	uint8_t				hpet_number;
	le16_t				min_tick;
	uint8_t				pg_prot;
};

#define ACPI_HPET_COUNT_SIZE_CAP		_ACPI_BIT(13)
#define ACPI_HPET_LEGACY_REPLACEMENT_CAP	_ACPI_BIT(15)
#define ACPI_HPET_COMPARATOR_COUNT(evtt)	(((evtt) >> 8) & 31)

#endif /* _ACPI_TABLES_H */
