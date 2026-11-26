/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Partition table compiled into the stock 825X_Pro kernel. Autochips generates this
 * header during the vendor Android build; the GPL kernel package does not ship it.
 * sd.c only uses it for CONFIG_MMC_ATC_SW_WP (off at boot).
 */
#ifndef __ATC_STORAGE_PARTITION_H__
#define __ATC_STORAGE_PARTITION_H__

#include <linux/types.h>

struct atc_part_info {
	char name[32];
	u64 part_offset;
	u64 part_size;
	char rw_type[8];
};

#define PART_NUM 42

static const struct atc_part_info PartInfo[PART_NUM] = {
	{ "preloader", 0x0000000000ULL, 0x0000040000ULL, "RO" },
	{ "pgpt", 0x0000000000ULL, 0x0000008000ULL, "RO" },
	{ "boot_para", 0x0000008000ULL, 0x0000100000ULL, "RW" },
	{ "recovery", 0x0000108000ULL, 0x0002000000ULL, "RO" },
	{ "para", 0x0002108000ULL, 0x0000080000ULL, "RW" },
	{ "expdb", 0x0002188000ULL, 0x0001400000ULL, "RW" },
	{ "nvcfg", 0x0003588000ULL, 0x0002000000ULL, "RW" },
	{ "nvdata", 0x0005588000ULL, 0x0004000000ULL, "RW" },
	{ "metadata", 0x0009588000ULL, 0x0002000000ULL, "RW" },
	{ "protect1", 0x000b588000ULL, 0x0000800000ULL, "RW" },
	{ "protect2", 0x000bd88000ULL, 0x0000a78000ULL, "RW" },
	{ "seccfg", 0x000c800000ULL, 0x0000800000ULL, "RW" },
	{ "persist", 0x000d000000ULL, 0x0003000000ULL, "RW" },
	{ "sec1", 0x0010000000ULL, 0x0000200000ULL, "RW" },
	{ "proinfo", 0x0010200000ULL, 0x0000300000ULL, "RW" },
	{ "md1img", 0x0010500000ULL, 0x0006400000ULL, "RW" },
	{ "spmfw", 0x0016900000ULL, 0x0000100000ULL, "RW" },
	{ "scp1", 0x0016a00000ULL, 0x0000100000ULL, "RW" },
	{ "scp2", 0x0016b00000ULL, 0x0000100000ULL, "RW" },
	{ "sspm_1", 0x0016c00000ULL, 0x0000100000ULL, "RW" },
	{ "sspm_2", 0x0016d00000ULL, 0x0000100000ULL, "RW" },
	{ "gz1", 0x0016e00000ULL, 0x0001000000ULL, "RW" },
	{ "gz2", 0x0017e00000ULL, 0x0001000000ULL, "RW" },
	{ "nvram", 0x0018e00000ULL, 0x0004000000ULL, "RW" },
	{ "arm2", 0x001ce00000ULL, 0x0002000000ULL, "RO" },
	{ "lk", 0x001ee00000ULL, 0x0000200000ULL, "RO" },
	{ "lk2", 0x001f000000ULL, 0x0000200000ULL, "RO" },
	{ "boot", 0x001f200000ULL, 0x0000a00000ULL, "RO" },
	{ "dtbo", 0x001fc00000ULL, 0x0000800000ULL, "RO" },
	{ "tee1", 0x0020400000ULL, 0x0000500000ULL, "RO" },
	{ "tee2", 0x0020900000ULL, 0x0000700000ULL, "RO" },
	{ "vendor", 0x0021000000ULL, 0x0040000000ULL, "RO" },
	{ "system", 0x0061000000ULL, 0x0080000000ULL, "RO" },
	{ "vbmeta", 0x00e1000000ULL, 0x0000800000ULL, "RW" },
	{ "logo", 0x00e1800000ULL, 0x0000800000ULL, "RW" },
	{ "metazone", 0x00e2000000ULL, 0x0000100000ULL, "RW" },
	{ "misc", 0x00e2100000ULL, 0x0000008000ULL, "RW" },
	{ "forfanzone", 0x00e2108000ULL, 0x0000100000ULL, "RW" },
	{ "tszone", 0x00e2208000ULL, 0x0000100000ULL, "RW" },
	{ "avm", 0x00e2308000ULL, 0x00014f8000ULL, "RW" },
	{ "cache", 0x00e3800000ULL, 0x001b000000ULL, "RW" },
	{ "userdata", 0x00fe800000ULL, 0x00c0000000ULL, "RW" },
};

#endif /* __ATC_STORAGE_PARTITION_H__ */
