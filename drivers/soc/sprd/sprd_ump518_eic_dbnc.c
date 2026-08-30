// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Spreadtrum Communications Inc.
 */



#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

/* PMIC EIC DBNC0~15 */
#define SPRD_EIC_DBNC_BANK0_START	208
#define SPRD_EIC_DBNC_BANK0_END		223
/* EIC_DBNC BANK0 */
#define EIC_DBNC0_OTP_OUT_WARN		208
#define EIC_DBNC1_VDDCAMA0_OCP		209
#define EIC_DBNC2_VDDCAMA1_OCP		210
#define EIC_DBNC3_VDDCAMAMOT_OCP	211
#define EIC_DBNC4_VDDSIM0_OCP		212
#define EIC_DBNC5_VDDSIM1_OCP		213
#define EIC_DBNC6_VDDSIM2_OCP		214
#define EIC_DBNC7_VDDEMMCCORE_OCP	215
#define EIC_DBNC8_VDDSDCORE_OCP		216
#define EIC_DBNC9_VDDSDIO_OCP		217
#define EIC_DBNC10_VDD28_OCP		218
#define EIC_DBNC11_VDDWIFIPA_OCP	219
#define EIC_DBNC12_VDDUSB33_OCP		220
#define EIC_DBNC13_VDDLDO0_OCP		221
#define EIC_DBNC14_VDDLDO1_OCP		222
#define EIC_DBNC15_VDDLDO2_OCP		223

/* PMIC EIC DBNC16~31 */
#define SPRD_EIC_DBNC_BANK1_START	224
#define SPRD_EIC_DBNC_BANK1_END		239
/* EIC_DBNC BANK1 */
#define EIC_DBNC16_VDDVIB_OCP		224
#define EIC_DBNC17_VDDKPLED_OCP		225
#define EIC_DBNC18_LDO_AVDD18_OCP	226
#define EIC_DBNC19_LDO_RF18_OCP		227
#define EIC_DBNC20_VDDLDO3_OCP		228
#define EIC_DBNC21_LDO_VDDWCN_OCP	229
#define EIC_DBNC22_LDO_VDDCAMD1_OCP	230
#define EIC_DBNC23_LDO_VDDCAMD0_OCP	231
#define EIC_DBNC24_LDO_VDDRF1V25_OCP	232
#define EIC_DBNC25_LDO_AVDD12_OCP	233
#define EIC_DBNC26_DCDC_GEN0_OCP	234
#define EIC_DBNC27_DCDC_GEN1_OCP	235
#define EIC_DBNC28_DCDC_MEMQ_OCP	236
#define EIC_DBNC29_DCDC_MEM_OCP		237
#define EIC_DBNC30_DCDC_CPU_OCP		238
#define EIC_DBNC31_DCDC_CORE_OCP	239

/* PMIC EIC DBNC32~45 */
#define SPRD_EIC_DBNC_BANK2_START	240
#define SPRD_EIC_DBNC_BANK2_END		255
/* EIC_DBNC BANK2 */
#define EIC_DBNC32_DCDC_GPU_OCP		240
#define EIC_DBNC33_DCDC_MODEM_OCP	241
#define EIC_DBNC34_DCDC_WPA_OCP		242
#define EIC_DBNC35_DCDC_SRAM_OCP	243
#define EIC_DBNC36_DCDC_GEN0_SCP	244
#define EIC_DBNC37_DCDC_GEN1_SCP	245
#define EIC_DBNC38_DCDC_MEMQ_SCP	246
#define EIC_DBNC39_DCDC_MEM_SCP		247
#define EIC_DBNC40_DCDC_CPU_SCP		248
#define EIC_DBNC41_DCDC_CORE_SCP	249
#define EIC_DBNC42_DCDC_GPU_SCP		250
#define EIC_DBNC43_DCDC_MODEM_SCP	251
#define EIC_DBNC44_DCDC_WPA_SCP		252
#define EIC_DBNC45_DCDC_SRAM_SCP	253

/* control reg */
#define EIC_DBNC_IE0	0x18
#define EIC_DBNC_IE1	0x1C
#define EIC_DBNC_IE2	0x20
#define EIC_DBNC_MIS0	0x48
#define EIC_DBNC_MIS1	0x4c
#define EIC_DBNC_MIS2	0x50
#define EIC_DBNC_IC0	0x60
#define EIC_DBNC_IC1	0x64
#define EIC_DBNC_IC2	0x68

/* ANA_REG_GLB_LDO_OCP_CTRL0 */
#define BIT_LDO_VDD28_OCP_FLAG_CLR		BIT(15)
#define BIT_LDO_VDDSDIO_OCP_FLAG_CLR		BIT(14)
#define BIT_LDO_VDDSDCORE_OCP_PD_CLR		BIT(13)
#define BIT_LDO_VDDSDCORE_OCP_MODE		BIT(12)
#define BIT_LDO_VDDSDCORE_OCP_FLAG_CLR		BIT(11)
#define BIT_LDO_VDDEMMCCORE_OCP_PD_CLR		BIT(10)
#define BIT_LDO_VDDEMMCCORE_OCP_MODE		BIT(9)
#define BIT_LDO_VDDEMMCCORE_OCP_FLAG_CLR	BIT(8)
#define BIT_LDO_VDDSIM2_OCP_FLAG_CLR		BIT(7)
#define BIT_LDO_VDDSIM1_OCP_FLAG_CLR		BIT(6)
#define BIT_LDO_VDDSIM0_OCP_FLAG_CLR		BIT(5)
#define BIT_LDO_VDDCAMMOT_OCP_PD_CLR		BIT(4)
#define BIT_LDO_VDDCAMMOT_OCP_MODE		BIT(3)
#define BIT_LDO_VDDCAMMOT_OCP_FLAG_CLR		BIT(2)
#define BIT_LDO_VDDCAMA1_OCP_FLAG_CLR		BIT(1)
#define BIT_LDO_VDDCAMA0_OCP_FLAG_CLR		BIT(0)

/* ANA_REG_GLB_LDO_OCP_CTRL1 */
#define BIT_LDO_VDDWCN_OCP_FLAG_CLR		BIT(14)
#define BIT_LDO_VDDLDO3_OCP_FLAG_CLR		BIT(13)
#define BIT_LDO_RF18_OCP_PD_CLR			BIT(12)
#define BIT_LDO_RF18_OCP_MODE			BIT(11)
#define BIT_LDO_RF18_OCP_FLAG_CLR		BIT(10)
#define BIT_LDO_AVDD18_OCP_FLAG_CLR		BIT(9)
#define BIT_LDO_VDDKPLED_OCP_FLAG_CLR		BIT(8)
#define BIT_LDO_VDDVIB_OCP_FLAG_CLR		BIT(7)
#define BIT_LDO_VDDLDO2_OCP_FLAG_CLR		BIT(6)
#define BIT_LDO_VDDLDO1_OCP_FLAG_CLR		BIT(5)
#define BIT_LDO_VDDLDO0_OCP_FLAG_CLR		BIT(4)
#define BIT_LDO_VDDUSB33_OCP_FLAG_CLR		BIT(3)
#define BIT_LDO_VDDWIFIPA_OCP_PD_CLR		BIT(2)
#define BIT_LDO_VDDWIFIPA_OCP_MODE		BIT(1)
#define BIT_LDO_VDDWIFIPA_OCP_FLAG_CLR		BIT(0)

/* ANA_REG_GLB_LDO_OCP_CTRL2 */
#define BIT_LDO_AVDD12_OCP_PD_CLR		BIT(11)
#define BIT_LDO_AVDD12_OCP_MODE			BIT(10)
#define BIT_LDO_AVDD12_OCP_FLAG_CLR		BIT(9)
#define BIT_LDO_VDDRF1V25_OCP_PD_CLR		BIT(8)
#define BIT_LDO_VDDRF1V25_OCP_MODE		BIT(7)
#define BIT_LDO_VDDRF1V25_OCP_FLAG_CLR		BIT(6)
#define BIT_LDO_VDDCAMD1_OCP_PD_CLR		BIT(5)
#define BIT_LDO_VDDCAMD1_OCP_MODE		BIT(4)
#define BIT_LDO_VDDCAMD1_OCP_FLAG_CLR		BIT(3)
#define BIT_LDO_VDDCAMD0_OCP_PD_CLR		BIT(2)
#define BIT_LDO_VDDCAMD0_OCP_MODE		BIT(1)
#define BIT_LDO_VDDCAMD0_OCP_FLAG_CLR		BIT(0)

/* ANA_REG_GLB_LDO_OCP_FLAG0 */
#define BIT_LDO_VDDVIB_OCP_FLAG			BIT(15)
#define BIT_LDO_VDDLDO2_OCP_FLAG		BIT(14)
#define BIT_LDO_VDDLDO1_OCP_FLAG		BIT(13)
#define BIT_LDO_VDDLDO0_OCP_FLAG		BIT(12)
#define BIT_LDO_VDDUSB33_OCP_FLAG		BIT(11)
#define BIT_LDO_VDDWIFIPA_OCP_FLAG		BIT(10)
#define BIT_LDO_VDD28_OCP_FLAG			BIT(9)
#define BIT_LDO_VDDSDIO_OCP_FLAG		BIT(8)
#define BIT_LDO_VDDSDCORE_OCP_FLAG		BIT(7)
#define BIT_LDO_VDDEMMCCORE_OCP_FLAG		BIT(6)
#define BIT_LDO_VDDSIM2_OCP_FLAG		BIT(5)
#define BIT_LDO_VDDSIM1_OCP_FLAG		BIT(4)
#define BIT_LDO_VDDSIM0_OCP_FLAG		BIT(3)
#define BIT_LDO_VDDCAMMOT_OCP_FLAG		BIT(2)
#define BIT_LDO_VDDCAMA1_OCP_FLAG		BIT(1)
#define BIT_LDO_VDDCAMA0_OCP_FLAG		BIT(0)

/* ANA_REG_GLB_LDO_OCP_FLAG1 */
#define BIT_LDO_AVDD12_OCP_FLAG			BIT(8)
#define BIT_LDO_VDDRF1V25_OCP_FLAG		BIT(7)
#define BIT_LDO_VDDCAMD1_OCP_FLAG		BIT(6)
#define BIT_LDO_VDDCAMD0_OCP_FLAG		BIT(5)
#define BIT_LDO_VDDWCN_OCP_FLAG			BIT(4)
#define BIT_LDO_VDDLDO3_OCP_FLAG		BIT(3)
#define BIT_LDO_RF18_OCP_FLAG			BIT(2)
#define BIT_LDO_AVDD18_OCP_FLAG			BIT(1)
#define BIT_LDO_VDDKPLED_OCP_FLAG		BIT(0)

/* ANA_REG_GLB_DCDC_OCP_CTRL0 */
#define BIT_DCDC_CORE_OCP_PD_CLR		BIT(15)
#define BIT_DCDC_CORE_OCP_MODE			BIT(14)
#define BIT_DCDC_CORE_OCP_FLAG_CLR		BIT(13)
#define BIT_DCDC_CPU_OCP_PD_CLR			BIT(12)
#define BIT_DCDC_CPU_OCP_MODE			BIT(11)
#define BIT_DCDC_CPU_OCP_FLAG_CLR		BIT(10)
#define BIT_DCDC_MEM_OCP_PD_CLR			BIT(9)
#define BIT_DCDC_MEM_OCP_MODE			BIT(8)
#define BIT_DCDC_MEM_OCP_FLAG_CLR		BIT(7)
#define BIT_DCDC_MEMQ_OCP_PD_CLR		BIT(6)
#define BIT_DCDC_MEMQ_OCP_MODE			BIT(5)
#define BIT_DCDC_MEMQ_OCP_FLAG_CLR		BIT(4)
#define BIT_DCDC_GEN1_OCP_MODE			BIT(3)
#define BIT_DCDC_GEN1_OCP_CHIP_PD_FLAG_CLR	BIT(2)
#define BIT_DCDC_GEN0_OCP_MODE			BIT(1)
#define BIT_DCDC_GEN0_OCP_CHIP_PD_FLAG_CLR	BIT(0)

/* ANA_REG_GLB_DCDC_OCP_CTRL1 */
#define BIT_DCDC_SRAM_OCP_PD_CLR		BIT(11)
#define BIT_DCDC_SRAM_OCP_MODE			BIT(10)
#define BIT_DCDC_SRAM_OCP_FLAG_CLR		BIT(9)
#define BIT_DCDC_WPA_OCP_PD_CLR			BIT(8)
#define BIT_DCDC_WPA_OCP_MODE			BIT(7)
#define BIT_DCDC_WPA_OCP_FLAG_CLR		BIT(6)
#define BIT_DCDC_MODEM_OCP_PD_CLR		BIT(5)
#define BIT_DCDC_MODEM_OCP_MODE			BIT(4)
#define BIT_DCDC_MODEM_OCP_FLAG_CLR		BIT(3)
#define BIT_DCDC_GPU_OCP_PD_CLR			BIT(2)
#define BIT_DCDC_GPU_OCP_MODE			BIT(1)
#define BIT_DCDC_GPU_OCP_FLAG_CLR		BIT(0)

/* ANA_REG_GLB_DCDC_SCP_CTRL0 */
#define BIT_DCDC_CORE_SCP_PD_CLR		BIT(15)
#define BIT_DCDC_CORE_SCP_MODE			BIT(14)
#define BIT_DCDC_CORE_SCP_FLAG_CLR		BIT(13)
#define BIT_DCDC_CPU_SCP_PD_CLR			BIT(12)
#define BIT_DCDC_CPU_SCP_MODE			BIT(11)
#define BIT_DCDC_CPU_SCP_FLAG_CLR		BIT(10)
#define BIT_DCDC_MEM_SCP_PD_CLR			BIT(9)
#define BIT_DCDC_MEM_SCP_MODE			BIT(8)
#define BIT_DCDC_MEM_SCP_FLAG_CLR		BIT(7)
#define BIT_DCDC_MEMQ_SCP_PD_CLR		BIT(6)
#define BIT_DCDC_MEMQ_SCP_MODE			BIT(5)
#define BIT_DCDC_MEMQ_SCP_FLAG_CLR		BIT(4)
#define BIT_DCDC_GEN1_SCP_MODE			BIT(3)
#define BIT_DCDC_GEN1_SCP_CHIP_PD_FLAG_CLR	BIT(2)
#define BIT_DCDC_GEN0_SCP_MODE			BIT(1)
#define BIT_DCDC_GEN0_SCP_CHIP_PD_FLAG_CLR	BIT(0)

/* ANA_REG_GLB_DCDC_SCP_CTRL1 */
#define BIT_DCDC_SRAM_SCP_PD_CLR		BIT(11)
#define BIT_DCDC_SRAM_SCP_MODE			BIT(10)
#define BIT_DCDC_SRAM_SCP_FLAG_CLR		BIT(9)
#define BIT_DCDC_WPA_SCP_PD_CLR			BIT(8)
#define BIT_DCDC_WPA_SCP_MODE			BIT(7)
#define BIT_DCDC_WPA_SCP_FLAG_CLR		BIT(6)
#define BIT_DCDC_MODEM_SCP_PD_CLR		BIT(5)
#define BIT_DCDC_MODEM_SCP_MODE			BIT(4)
#define BIT_DCDC_MODEM_SCP_FLAG_CLR		BIT(3)
#define BIT_DCDC_GPU_SCP_PD_CLR			BIT(2)
#define BIT_DCDC_GPU_SCP_MODE			BIT(1)
#define BIT_DCDC_GPU_SCP_FLAG_CLR		BIT(0)

/* ANA_REG_GLB_DCDC_OCP_FLAG0 */
#define BIT_DCDC_SRAM_OCP_FLAG			BIT(9)
#define BIT_DCDC_WPA_OCP_FLAG			BIT(8)
#define BIT_DCDC_MODEM_OCP_FLAG			BIT(7)
#define BIT_DCDC_GPU_OCP_FLAG			BIT(6)
#define BIT_DCDC_CORE_OCP_FLAG			BIT(5)
#define BIT_DCDC_CPU_OCP_FLAG			BIT(4)
#define BIT_DCDC_MEM_OCP_FLAG			BIT(3)
#define BIT_DCDC_MEMQ_OCP_FLAG			BIT(2)
#define BIT_DCDC_GEN1_OCP_CHIP_PD_FLAG		BIT(1)
#define BIT_DCDC_GEN0_OCP_CHIP_PD_FLAG		BIT(0)

/* ANA_REG_GLB_DCDC_SCP_FLAG0 */

#define BIT_DCDC_SRAM_SCP_FLAG			BIT(9)
#define BIT_DCDC_WPA_SCP_FLAG			BIT(8)
#define BIT_DCDC_MODEM_SCP_FLAG			BIT(7)
#define BIT_DCDC_GPU_SCP_FLAG			BIT(6)
#define BIT_DCDC_CORE_SCP_FLAG			BIT(5)
#define BIT_DCDC_CPU_SCP_FLAG			BIT(4)
#define BIT_DCDC_MEM_SCP_FLAG			BIT(3)
#define BIT_DCDC_MEMQ_SCP_FLAG			BIT(2)
#define BIT_DCDC_GEN1_SCP_CHIP_PD_FLAG		BIT(1)
#define BIT_DCDC_GEN0_SCP_CHIP_PD_FLAG		BIT(0)

#define BIT_EIC_DBNC_EN				BIT(10)
#define BIT_RTC_EIC_DBNC_EN			BIT(5)

#define ANA_REG_GLB_MODULE_EN0			0x0008
#define ANA_REG_GLB_RTC_CLK_EN0			0x0010
#define ANA_REG_GLB_LDO_OCP_CTRL0		0x044C
#define ANA_REG_GLB_LDO_OCP_CTRL1		0x0450
#define ANA_REG_GLB_LDO_OCP_CTRL2		0x0454
#define ANA_REG_GLB_LDO_OCP_FLAG0		0x0458
#define ANA_REG_GLB_LDO_OCP_FLAG1		0x045C
#define ANA_REG_GLB_DCDC_OCP_CTRL0		0x0460
#define ANA_REG_GLB_DCDC_OCP_CTRL1		0x0464
#define ANA_REG_GLB_DCDC_SCP_CTRL0		0x0468
#define ANA_REG_GLB_DCDC_SCP_CTRL0		0x0468
#define ANA_REG_GLB_DCDC_SCP_CTRL1		0x046C
#define ANA_REG_GLB_DCDC_OCP_FLAG0		0x0470
#define ANA_REG_GLB_DCDC_SCP_FLAG0		0x0474

#define EIC_DBNC_INVALID_REG			(~(u64)0)
#define BIT(nr)					(UL(1) << (nr))
#define DBNC_SIZE(a)				(sizeof(a)/sizeof(a[0]))

struct eic_dbnc_desc_priv {
	const char *power;
	/* 00 none / 01 ocp / 10 scp / 11 both */
	int ie;
	/* BIT4: (0: off, 1: alive); BIT0~BIT3: (0: ldo ; 2: dcdc) */
	int type_power;
	/* eic_dbnc ocp id */
	int eic_dbnc_ocp;
	/* eic_dbnc scp id */
	int eic_dbnc_scp;
	unsigned long ocp_ctrl;
	u32 ocp_pd_clr_bit;
	u32 ocp_mode_bit;
	u32 ocp_flag_clr_bit;
	unsigned long scp_ctrl;
	u32 scp_pd_clr_bit;
	u32 scp_mode_bit;
	u32 scp_flag_clr_bit;
	unsigned long ocp_flag;
	u32 ocp_occur;
	unsigned long scp_flag;
	u32 scp_occur;
};

struct sprd_eic_dbnc {
	struct device *dev;
	struct regmap *reg_map;
	int irq;
	u32 eic_dbnc_base;
	u32 glb_base;
	const struct eic_dbnc_desc_priv *priv;
};

struct eic_dbnc_info{
	u8 bit_bank;
	u8 bit_num;
};

struct eic_dbnc_desc_priv ump518_eic_dbnc_desc[] = {
	{"vddcama0_eic", 0, 0x10, EIC_DBNC1_VDDCAMA0_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL0, 0, 0, BIT_LDO_VDDCAMA0_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDCAMA0_OCP_FLAG, 0, 0},
	{"vddcama1_eic", 0, 0x10, EIC_DBNC2_VDDCAMA1_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL0, 0, 0, BIT_LDO_VDDCAMA1_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDCAMA1_OCP_FLAG, 0, 0},
	{"vddcammot_eic", 0, 0x10, EIC_DBNC3_VDDCAMAMOT_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL0, BIT_LDO_VDDCAMMOT_OCP_PD_CLR,
		BIT_LDO_VDDCAMMOT_OCP_MODE, BIT_LDO_VDDCAMMOT_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDCAMMOT_OCP_FLAG, 0, 0},
	{"vddsim0_eic", 0, 0x10, EIC_DBNC4_VDDSIM0_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL0, 0, 0, BIT_LDO_VDDSIM0_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDSIM0_OCP_FLAG, 0, 0},
	{"vddsim1_eic", 0, 0x10, EIC_DBNC5_VDDSIM1_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL0, 0, 0, BIT_LDO_VDDSIM1_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDSIM1_OCP_FLAG, 0, 0},
	{"vddsim2_eic", 0, 0x10, EIC_DBNC6_VDDSIM2_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL0, 0, 0, BIT_LDO_VDDSIM2_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDSIM2_OCP_FLAG, 0, 0},
	{"vddemmccore_eic", 0, 0x10, EIC_DBNC7_VDDEMMCCORE_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL0, BIT_LDO_VDDEMMCCORE_OCP_PD_CLR,
		BIT_LDO_VDDEMMCCORE_OCP_MODE, BIT_LDO_VDDEMMCCORE_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDEMMCCORE_OCP_FLAG, 0, 0},
	{"vddsdcore_eic", 0, 0x10, EIC_DBNC8_VDDSDCORE_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL0, BIT_LDO_VDDSDCORE_OCP_PD_CLR,
		BIT_LDO_VDDSDCORE_OCP_MODE, BIT_LDO_VDDSDCORE_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDSDCORE_OCP_FLAG, 0, 0},
	{"vddsdio_eic", 0, 0x10, EIC_DBNC9_VDDSDIO_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL0, 0, 0, BIT_LDO_VDDSDIO_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDSDIO_OCP_FLAG, 0, 0},
	{"vdd28_eic", 0, 0x10, EIC_DBNC10_VDD28_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL0, 0, 0, BIT_LDO_VDD28_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDD28_OCP_FLAG, 0, 0},
	{"vddwifipa_eic", 0, 0x10, EIC_DBNC11_VDDWIFIPA_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, BIT_LDO_VDDWIFIPA_OCP_PD_CLR,
		BIT_LDO_VDDWIFIPA_OCP_MODE, BIT_LDO_VDDWIFIPA_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDWIFIPA_OCP_FLAG, 0, 0},
	{"vddusb33_eic", 0, 0x10, EIC_DBNC12_VDDUSB33_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, 0, 0, BIT_LDO_VDDUSB33_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDUSB33_OCP_FLAG, 0, 0},
	{"vddldo0_eic", 0, 0x10, EIC_DBNC13_VDDLDO0_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, 0, 0, BIT_LDO_VDDLDO0_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDLDO0_OCP_FLAG, 0, 0},
	{"vddldo1_eic", 0, 0x10, EIC_DBNC14_VDDLDO1_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, 0, 0, BIT_LDO_VDDLDO1_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDLDO1_OCP_FLAG, 0, 0},
	{"vddldo2_eic", 0, 0x10, EIC_DBNC15_VDDLDO2_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, 0, 0, BIT_LDO_VDDLDO2_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDLDO2_OCP_FLAG, 0, 0},
	{"vddvib_eic", 0, 0x10, EIC_DBNC16_VDDVIB_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, 0, 0, BIT_LDO_VDDVIB_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG0, BIT_LDO_VDDVIB_OCP_FLAG, 0, 0},
	{"vddkpled_eic", 0, 0x10, EIC_DBNC17_VDDKPLED_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, 0, 0, BIT_LDO_VDDKPLED_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG1, BIT_LDO_VDDKPLED_OCP_FLAG, 0, 0},
	{"vddavdd18_eic", 0, 0x10, EIC_DBNC18_LDO_AVDD18_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, 0, 0, BIT_LDO_AVDD18_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG1, BIT_LDO_AVDD18_OCP_FLAG, 0, 0},
	{"vddrf18_eic", 0, 0x10, EIC_DBNC19_LDO_RF18_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, BIT_LDO_RF18_OCP_PD_CLR,
		BIT_LDO_RF18_OCP_MODE, BIT_LDO_RF18_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG1, BIT_LDO_RF18_OCP_FLAG, 0, 0},
	{"vddldo3_eic", 0, 0x10, EIC_DBNC20_VDDLDO3_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, 0, 0, BIT_LDO_VDDLDO3_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG1, BIT_LDO_VDDLDO3_OCP_FLAG, 0, 0},
	{"vddwcn_eic", 0, 0x10, EIC_DBNC21_LDO_VDDWCN_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL1, 0, 0, BIT_LDO_VDDWCN_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG1, BIT_LDO_VDDWCN_OCP_FLAG, 0, 0},
	{"vddcamd0_eic", 0, 0x10, EIC_DBNC22_LDO_VDDCAMD1_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL2, BIT_LDO_VDDCAMD0_OCP_PD_CLR,
		BIT_LDO_VDDCAMD0_OCP_MODE, BIT_LDO_VDDCAMD0_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG1, BIT_LDO_VDDCAMD0_OCP_FLAG, 0, 0},
	{"vddcamd1_eic", 0, 0x10, EIC_DBNC23_LDO_VDDCAMD0_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL2, BIT_LDO_VDDCAMD1_OCP_PD_CLR,
		BIT_LDO_VDDCAMD1_OCP_MODE, BIT_LDO_VDDCAMD1_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG1, BIT_LDO_VDDCAMD1_OCP_FLAG, 0, 0},
	{"vddrf1v25_eic", 0, 0x10, EIC_DBNC24_LDO_VDDRF1V25_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL2, BIT_LDO_VDDCAMD0_OCP_PD_CLR,
		BIT_LDO_VDDCAMD0_OCP_MODE, BIT_LDO_VDDCAMD0_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG1, BIT_LDO_VDDRF1V25_OCP_FLAG, 0, 0},
	{"avdd12_eic", 0, 0x10, EIC_DBNC25_LDO_AVDD12_OCP, 0,
		ANA_REG_GLB_LDO_OCP_CTRL2, BIT_LDO_AVDD12_OCP_PD_CLR,
		BIT_LDO_AVDD12_OCP_MODE, BIT_LDO_AVDD12_OCP_FLAG_CLR,
		0, 0, 0, 0,
		ANA_REG_GLB_LDO_OCP_FLAG1, BIT_LDO_AVDD12_OCP_FLAG, 0, 0},
	{"dcdcgen0_eic", 0, 0x12, EIC_DBNC26_DCDC_GEN0_OCP, EIC_DBNC36_DCDC_GEN0_SCP,
		ANA_REG_GLB_DCDC_OCP_CTRL0, 0,
		BIT_DCDC_GEN0_OCP_MODE, BIT_DCDC_GEN0_OCP_CHIP_PD_FLAG_CLR,
		ANA_REG_GLB_DCDC_SCP_CTRL0, 0,
		BIT_DCDC_GEN0_SCP_MODE, BIT_DCDC_GEN0_SCP_CHIP_PD_FLAG_CLR,
		ANA_REG_GLB_DCDC_OCP_FLAG0, BIT_DCDC_GEN0_OCP_CHIP_PD_FLAG,
		ANA_REG_GLB_DCDC_SCP_FLAG0, BIT_DCDC_GEN0_SCP_CHIP_PD_FLAG},
	{"dcdcgen1_eic", 0, 0x12, EIC_DBNC27_DCDC_GEN1_OCP, EIC_DBNC37_DCDC_GEN1_SCP,
		ANA_REG_GLB_DCDC_OCP_CTRL0, 0,
		BIT_DCDC_GEN1_OCP_MODE, BIT_DCDC_GEN1_OCP_CHIP_PD_FLAG_CLR,
		ANA_REG_GLB_DCDC_SCP_CTRL0, 0,
		BIT_DCDC_GEN1_SCP_MODE, BIT_DCDC_GEN1_SCP_CHIP_PD_FLAG_CLR,
		ANA_REG_GLB_DCDC_OCP_FLAG0, BIT_DCDC_GEN1_OCP_CHIP_PD_FLAG,
		ANA_REG_GLB_DCDC_SCP_FLAG0, BIT_DCDC_GEN1_SCP_CHIP_PD_FLAG},
	{"dcdcmemq_eic", 0, 0x12, EIC_DBNC28_DCDC_MEMQ_OCP, EIC_DBNC38_DCDC_MEMQ_SCP,
		ANA_REG_GLB_DCDC_OCP_CTRL0, BIT_DCDC_MEMQ_OCP_PD_CLR,
		BIT_DCDC_MEMQ_OCP_MODE, BIT_DCDC_MEMQ_OCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_SCP_CTRL0, BIT_DCDC_MEMQ_SCP_PD_CLR,
		BIT_DCDC_MEMQ_SCP_MODE, BIT_DCDC_MEMQ_SCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_OCP_FLAG0, BIT_DCDC_MEMQ_OCP_FLAG,
		ANA_REG_GLB_DCDC_SCP_FLAG0, BIT_DCDC_MEMQ_SCP_FLAG},
	{"dcdcmem_eic", 0, 0x12, EIC_DBNC29_DCDC_MEM_OCP, EIC_DBNC39_DCDC_MEM_SCP,
		ANA_REG_GLB_DCDC_OCP_CTRL0, BIT_DCDC_MEM_OCP_PD_CLR,
		BIT_DCDC_MEM_OCP_MODE, BIT_DCDC_MEM_OCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_SCP_CTRL0, BIT_DCDC_MEM_SCP_PD_CLR,
		BIT_DCDC_MEM_SCP_MODE, BIT_DCDC_MEM_SCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_OCP_FLAG0, BIT_DCDC_MEM_OCP_FLAG,
		ANA_REG_GLB_DCDC_SCP_FLAG0, BIT_DCDC_MEM_SCP_FLAG},
	{"dcdccpu_eic", 0, 0x12, EIC_DBNC30_DCDC_CPU_OCP, EIC_DBNC40_DCDC_CPU_SCP,
		ANA_REG_GLB_DCDC_OCP_CTRL0, BIT_DCDC_CPU_OCP_PD_CLR,
		BIT_DCDC_MEMQ_OCP_MODE, BIT_DCDC_CPU_OCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_SCP_CTRL0, BIT_DCDC_CPU_SCP_PD_CLR,
		BIT_DCDC_MEMQ_SCP_MODE, BIT_DCDC_CPU_SCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_OCP_FLAG0, BIT_DCDC_CPU_OCP_FLAG,
		ANA_REG_GLB_DCDC_SCP_FLAG0, BIT_DCDC_CPU_SCP_FLAG},
	{"dcdccore_eic", 0, 0x12, EIC_DBNC31_DCDC_CORE_OCP, EIC_DBNC41_DCDC_CORE_SCP,
		ANA_REG_GLB_DCDC_OCP_CTRL0, BIT_DCDC_CORE_OCP_PD_CLR,
		BIT_DCDC_CORE_OCP_MODE, BIT_DCDC_CORE_OCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_SCP_CTRL0, BIT_DCDC_CORE_SCP_PD_CLR,
		BIT_DCDC_CORE_SCP_MODE, BIT_DCDC_CORE_SCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_OCP_FLAG0, BIT_DCDC_CORE_OCP_FLAG,
		ANA_REG_GLB_DCDC_SCP_FLAG0, BIT_DCDC_CORE_SCP_FLAG},
	{"dcdcgpu_eic", 0, 0x12, EIC_DBNC32_DCDC_GPU_OCP, EIC_DBNC42_DCDC_GPU_SCP,
		ANA_REG_GLB_DCDC_OCP_CTRL1, BIT_DCDC_GPU_OCP_PD_CLR,
		BIT_DCDC_GPU_OCP_MODE, BIT_DCDC_GPU_OCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_SCP_CTRL1, BIT_DCDC_GPU_SCP_PD_CLR,
		BIT_DCDC_GPU_SCP_MODE, BIT_DCDC_GPU_SCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_OCP_FLAG0, BIT_DCDC_GPU_OCP_FLAG,
		ANA_REG_GLB_DCDC_SCP_FLAG0, BIT_DCDC_GPU_SCP_FLAG},
	{"dcdcmodem_eic", 0, 0x12, EIC_DBNC33_DCDC_MODEM_OCP, EIC_DBNC43_DCDC_MODEM_SCP,
		ANA_REG_GLB_DCDC_OCP_CTRL1, BIT_DCDC_MODEM_OCP_PD_CLR,
		BIT_DCDC_MODEM_OCP_MODE, BIT_DCDC_MODEM_OCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_SCP_CTRL1, BIT_DCDC_MODEM_SCP_PD_CLR,
		BIT_DCDC_MODEM_SCP_MODE, BIT_DCDC_MODEM_SCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_OCP_FLAG0, BIT_DCDC_MODEM_OCP_FLAG,
		ANA_REG_GLB_DCDC_SCP_FLAG0, BIT_DCDC_MODEM_SCP_FLAG},
	{"dcdcwpa_eic", 0, 0x12, EIC_DBNC34_DCDC_WPA_OCP, EIC_DBNC44_DCDC_WPA_SCP,
		ANA_REG_GLB_DCDC_OCP_CTRL1, BIT_DCDC_WPA_OCP_PD_CLR,
		BIT_DCDC_WPA_OCP_MODE, BIT_DCDC_WPA_OCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_SCP_CTRL1, BIT_DCDC_WPA_SCP_PD_CLR,
		BIT_DCDC_WPA_SCP_MODE, BIT_DCDC_WPA_SCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_OCP_FLAG0, BIT_DCDC_WPA_OCP_FLAG,
		ANA_REG_GLB_DCDC_SCP_FLAG0, BIT_DCDC_WPA_SCP_FLAG},
	{"dcdcsram_eic", 0, 0x12, EIC_DBNC35_DCDC_SRAM_OCP, EIC_DBNC45_DCDC_SRAM_SCP,
		ANA_REG_GLB_DCDC_OCP_CTRL1, BIT_DCDC_SRAM_OCP_PD_CLR,
		BIT_DCDC_SRAM_OCP_MODE, BIT_DCDC_SRAM_OCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_SCP_CTRL1, BIT_DCDC_SRAM_SCP_PD_CLR,
		BIT_DCDC_SRAM_SCP_MODE, BIT_DCDC_SRAM_SCP_FLAG_CLR,
		ANA_REG_GLB_DCDC_OCP_FLAG0, BIT_DCDC_SRAM_OCP_FLAG,
		ANA_REG_GLB_DCDC_SCP_FLAG0, BIT_DCDC_SRAM_SCP_FLAG},
};

static __inline unsigned long eic_ffs(u64 word)
{
	int num = 0;

	if ((word & 0xffffffff) == 0) {
		num += 32;
		word >>= 32;
	}
	if ((word & 0xffff) == 0) {
		num += 16;
		word >>= 16;
	}
	if ((word & 0xff) == 0) {
		num += 8;
		word >>= 8;
	}
	if ((word & 0xf) == 0) {
		num += 4;
		word >>= 4;
	}
	if ((word & 0x3) == 0) {
		num += 2;
		word >>= 2;
	}
	if ((word & 0x1) == 0)
		num += 1;

	return num;
}

static struct eic_dbnc_desc_priv *dbnc_get(const char *dbnc_id)
{
	unsigned int i = 0;
	for (i = 0; i < DBNC_SIZE(ump518_eic_dbnc_desc); i++) {
		if (!strcmp(dbnc_id, ump518_eic_dbnc_desc[i].power))
			return &ump518_eic_dbnc_desc[i];
	}

	return NULL;
}

static int power_ocp_mode_ctrl(struct sprd_eic_dbnc *eic_dbnc, const char dbnc_id[], bool ocp_mode)
{
	int ret = 0, type = 0;
	struct eic_dbnc_desc_priv *desc = dbnc_get(dbnc_id);

	if (!desc)
		return -1;

	type = desc->type_power & (BIT(4) - 1);
	if (type == 0) {
		if (!desc->ocp_mode_bit)
			return -1;
		else {
			if (ocp_mode == 0) {
				ret = regmap_update_bits(eic_dbnc->reg_map,
						eic_dbnc->glb_base + desc->ocp_ctrl,
						desc->ocp_mode_bit, 0);
				if (ret)
					return ret;
			} else {
				ret = regmap_update_bits(eic_dbnc->reg_map,
						eic_dbnc->glb_base + desc->ocp_ctrl,
						desc->ocp_mode_bit, 1);
				if (ret)
					return ret;
			}
		}
	} else if (type == 2) {
		if (ocp_mode == 0) {
			ret = regmap_update_bits(eic_dbnc->reg_map,
					eic_dbnc->glb_base + desc->ocp_ctrl,
					desc->ocp_mode_bit, 0);
			if (ret)
				return ret;
		} else {
			ret = regmap_update_bits(eic_dbnc->reg_map,
					eic_dbnc->glb_base + desc->ocp_ctrl,
					desc->ocp_mode_bit, 1);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int power_scp_mode_ctrl(struct sprd_eic_dbnc *eic_dbnc, const char dbnc_id[], bool scp_mode)
{
	int ret = 0, type = 0;
	struct eic_dbnc_desc_priv *desc = dbnc_get(dbnc_id);

	if (!desc)
		return -1;

	type = desc->type_power & (BIT(4) - 1);
	if (type == 2) {
		if (scp_mode == 0) {
			ret = regmap_update_bits(eic_dbnc->reg_map,
					eic_dbnc->glb_base + desc->scp_ctrl,
					desc->scp_mode_bit, 0);
			if (ret)
				return ret;
		} else {
			ret = regmap_update_bits(eic_dbnc->reg_map,
					eic_dbnc->glb_base + desc->scp_ctrl,
					desc->scp_mode_bit, 1);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int __get_eic_dbnc_base_info(u32 eic_dbnc_id, struct eic_dbnc_info *info)
{
	info->bit_num  = eic_dbnc_id & 0xF;

	if (eic_dbnc_id >= SPRD_EIC_DBNC_BANK0_START && eic_dbnc_id <= SPRD_EIC_DBNC_BANK0_END)
		info->bit_bank = 0;
	else if (eic_dbnc_id >= SPRD_EIC_DBNC_BANK1_START && eic_dbnc_id <= SPRD_EIC_DBNC_BANK1_END)
		info->bit_bank = 1;
	else if (eic_dbnc_id >= SPRD_EIC_DBNC_BANK2_START && eic_dbnc_id <= SPRD_EIC_DBNC_BANK2_END)
		info->bit_bank = 2;
	else
		return -1;

	return 0;
}

static int sprd_eic_dbnc_irq_mask(struct sprd_eic_dbnc *eic_dbnc, unsigned int offset)
{
	unsigned eic_dbnc_id = offset;
	struct eic_dbnc_info eic_dbnc_info = {};
	unsigned long reg_addr = 0;
	int ret = 0;

	__get_eic_dbnc_base_info(eic_dbnc_id, &eic_dbnc_info);
	if (eic_dbnc_info.bit_bank == 0)
		reg_addr = EIC_DBNC_IE0;
	else if (eic_dbnc_info.bit_bank == 1)
		reg_addr = EIC_DBNC_IE1;
	else
		reg_addr = EIC_DBNC_IE2;

	ret = regmap_update_bits(eic_dbnc->reg_map, eic_dbnc->eic_dbnc_base + reg_addr,
			BIT(eic_dbnc_info.bit_num), 0);
	if (ret)
		return ret;

	return 0;
}

static int sprd_eic_dbnc_irq_unmask(struct sprd_eic_dbnc *eic_dbnc, unsigned int offset)
{
	unsigned eic_dbnc_id = offset;
	struct eic_dbnc_info eic_dbnc_info = {};
	unsigned long reg_addr = 0;
	int ret = 0;

	__get_eic_dbnc_base_info(eic_dbnc_id, &eic_dbnc_info);
	if (eic_dbnc_info.bit_bank == 0)
		reg_addr = EIC_DBNC_IE0;
	else if (eic_dbnc_info.bit_bank == 1)
		reg_addr = EIC_DBNC_IE1;
	else
		reg_addr = EIC_DBNC_IE2;

	ret = regmap_update_bits(eic_dbnc->reg_map, eic_dbnc->eic_dbnc_base + reg_addr,
			BIT(eic_dbnc_info.bit_num), 1);
	if (ret)
		return ret;

	return 0;
}

static int sprd_eic_dbnc_check_module_en(struct sprd_eic_dbnc *eic_dbnc)
{
	u32 module_en = 0, clk_en = 0;
	int ret = 0;

	ret = regmap_read(eic_dbnc->reg_map, eic_dbnc->glb_base + ANA_REG_GLB_MODULE_EN0,
			&module_en);
	if (ret)
		return ret;

	ret = regmap_read(eic_dbnc->reg_map, eic_dbnc->glb_base + ANA_REG_GLB_RTC_CLK_EN0, &clk_en);
	if (ret)
		return ret;

	/* If SPRD_EIC_DBNC module is not intend to use, no need to probe this driver */
	if ((module_en & BIT_EIC_DBNC_EN) && (clk_en & BIT_RTC_EIC_DBNC_EN))
		return 0;

	return -1;
}

static int sprd_eic_dbnc_check_mode_ctrl(struct sprd_eic_dbnc *eic_dbnc)
{
	int ret = 0;
	/* Before enable this Module, check what the pwr you care is choosed the
	 * right mode (ocp/scp). Generally for the system exception protection,
	 * this driver is mainly for maintenance.
	 * */

	/* Defult Record Events, PD Power Here
	 * < dbnc_id[]: "dcdc/vdd-xxx", scp/ocp_mode:0 1 >
	 * ret = power_scp_mode_ctrl(eic_dbnc, dbnc_id[], scp_mode);
	 * ret = power_ocp_mode_ctrl(eic_dbnc, dbnc_id[], ocp_mode);
	 * */
	ret = power_scp_mode_ctrl(eic_dbnc, "dcdcgen0_eic", 0x0);
	if (ret)
		return ret;

	ret = power_ocp_mode_ctrl(eic_dbnc, "dcdcgen0_eic", 0x0);
	if (ret)
		return ret;

	/* Defult IE, Mask EIC_DBNC_NUM Here
	 * < eic_dbnc_num: 208 ~ 253 >
	 * ret = sprd_eic_dbnc_irq_mask(eic_dbnc, eic_dbnc_num);
	 * */
	return ret;
}

static int sprd_eic_dbnc_irq_ack(struct sprd_eic_dbnc *eic_dbnc, unsigned int offset)
{
	unsigned eic_dbnc_id = offset;
	struct eic_dbnc_info eic_dbnc_info = {};
	unsigned long reg_addr = 0;
	int ret = 0;

	__get_eic_dbnc_base_info(eic_dbnc_id, &eic_dbnc_info);
	if (eic_dbnc_info.bit_bank == 0)
		reg_addr = EIC_DBNC_IC0;
	else if (eic_dbnc_info.bit_bank == 1)
		reg_addr = EIC_DBNC_IC1;
	else
		reg_addr = EIC_DBNC_IC2;

	ret = regmap_update_bits(eic_dbnc->reg_map, eic_dbnc->eic_dbnc_base + reg_addr,
			BIT(eic_dbnc_info.bit_num), 0);
	if (ret)
		return ret;

	return 0;
}

static irqreturn_t sprd_ump518_eic_dbnc_handler(int irq, void *data)
{
	struct sprd_eic_dbnc *eic_dbnc = data;
	int ret = 0, n = 0, eic_dbnc_id = 0;
	u64 value = 0;
	u32 tmp = 0;

	ret = regmap_read(eic_dbnc->reg_map, eic_dbnc->eic_dbnc_base + EIC_DBNC_MIS2, &tmp);
	if (ret)
		return ret;
	value = tmp;
	value = value << 32;
	ret = regmap_read(eic_dbnc->reg_map, eic_dbnc->eic_dbnc_base + EIC_DBNC_MIS1, &tmp);
	if (ret)
		return ret;
	value |= tmp << 16;
	ret = regmap_read(eic_dbnc->reg_map, eic_dbnc->eic_dbnc_base + EIC_DBNC_MIS0, &tmp);
	if (ret)
		return ret;
	value |= tmp;

	while (value) {
		n = eic_ffs(value);
		value &= ~(1UL << n);
		eic_dbnc_id = SPRD_EIC_DBNC_BANK0_START + n;
		sprd_eic_dbnc_irq_ack(eic_dbnc, eic_dbnc_id);
		sprd_eic_dbnc_irq_mask(eic_dbnc, eic_dbnc_id);
		pr_info("eic_dbnc lyx: id:%d occurs ocp/scp envents!\n", eic_dbnc_id);
		sprd_eic_dbnc_irq_unmask(eic_dbnc, eic_dbnc_id);
	}

	return IRQ_HANDLED;
}

static const struct of_device_id sprd_eic_dbnc_of_match[] = {
	{.compatible = "sprd,ump518-eic-dbnc",},
	{}
};
MODULE_DEVICE_TABLE(of, sprd_eic_dbnc_of_match);

static int sprd_ump518_eic_dbnc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sprd_eic_dbnc *sprd_eic_dbnc_dev;

	sprd_eic_dbnc_dev = devm_kzalloc(&pdev->dev, sizeof(*sprd_eic_dbnc_dev),
					GFP_KERNEL);
	if (!sprd_eic_dbnc_dev)
		return -ENOMEM;

	sprd_eic_dbnc_dev->reg_map = dev_get_regmap(dev->parent, NULL);
	if (!sprd_eic_dbnc_dev->reg_map) {
		dev_err(&pdev->dev, "NULL regmap for pmic_eic_dbnc!\n");
		return -EINVAL;
	}

	ret = of_property_read_u32_index(np, "reg", 0, &sprd_eic_dbnc_dev->eic_dbnc_base);
	if (ret) {
		dev_err(&pdev->dev, "failed to get eic_dbnc base address!\n");
		return ret;
	}

	ret = of_property_read_u32_index(np, "reg", 1, &sprd_eic_dbnc_dev->glb_base);
	if (ret) {
		dev_err(&pdev->dev, "failed to get glb base address!\n");
		return ret;
	}

	sprd_eic_dbnc_dev->irq = platform_get_irq(pdev, 0);
	if (sprd_eic_dbnc_dev->irq < 0) {
		dev_err(&pdev->dev, "failed to get eic_dbnc irq!\n");
		return sprd_eic_dbnc_dev->irq;
	}

	sprd_eic_dbnc_dev->dev = dev;
	platform_set_drvdata(pdev, sprd_eic_dbnc_dev);

	ret = sprd_eic_dbnc_check_module_en(sprd_eic_dbnc_dev);
	if (ret != 0) {
		dev_err(dev, "check_module_en failed\n");
		return ret;
	}

	ret = sprd_eic_dbnc_check_mode_ctrl(sprd_eic_dbnc_dev);
	if (ret != 0) {
		dev_err(dev, "check_mode_ctrl failed\n");
		return ret;
	}

	ret = devm_request_threaded_irq(dev, sprd_eic_dbnc_dev->irq, NULL,
					sprd_ump518_eic_dbnc_handler,
					IRQF_ONESHOT | IRQF_EARLY_RESUME,
					pdev->name, sprd_eic_dbnc_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request ump518 eic_dbnc irq\n");
		return ret;
	}

	return 0;
}

static struct platform_driver sprd_ump518_eic_dbnc_drv = {
	.driver = {
		   .name = "ump518-eic-dbnc",
		   .of_match_table = sprd_eic_dbnc_of_match,
	},
	.probe = sprd_ump518_eic_dbnc_probe,
};

module_platform_driver(sprd_ump518_eic_dbnc_drv);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Unisoc Ump518 Eic_dbnc Driver");
MODULE_AUTHOR("Yixin.Lin <Yixin.Lin@unisoc.com>");
