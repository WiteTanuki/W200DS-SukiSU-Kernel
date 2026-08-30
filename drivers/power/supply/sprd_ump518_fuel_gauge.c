// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2011 Unisoc Co., Ltd.
 * Changhua.Zhang <Changhua.Zhang@unisoc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/iio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/power_supply.h>
#include <linux/power/charger-manager.h>
#include <linux/power/sprd_battery_info.h>
#include <linux/power/sprd_fuel_gauge_core.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/rtc.h>

/* FGU registers definition */
#define UMP518_FGU_START		0x0
#define UMP518_FGU_CONFIG		0x4
#define UMP518_FGU_ADC_CONFIG0		0x8
#define UMP518_FGU_ADC_CONFIG1		0xc
#define UMP518_FGU_ADC_CONFIG2		0x10
#define UMP518_FGU_STATUS		0x14
#define UMP518_FGU_INT_EN		0x18
#define UMP518_FGU_INT_CLR		0x1c
#define UMP518_FGU_INT_RAW		0x20
#define UMP518_FGU_INT_STS		0x24
#define UMP518_FGU_VOLTAGE		0x28
#define UMP518_FGU_OCV			0x2c
#define UMP518_FGU_POCV			0x30
#define UMP518_FGU_CURRENT_H		0x34
#define UMP518_FGU_CURRENT_L		0x38
#define UMP518_FGU_HIGH_OVERLOAD	0x3c
#define UMP518_FGU_LOW_OVERLOAD		0x40
#define UMP518_FGU_CLBCNT_SET2		0x44
#define UMP518_FGU_CLBCNT_SET1		0x48
#define UMP518_FGU_CLBCNT_SET0		0x4c
#define UMP518_FGU_CLBCNT_DELT2		0x50
#define UMP518_FGU_CLBCNT_DELT1		0x54
#define UMP518_FGU_CLBCNT_DELT0		0x58
#define UMP518_FGU_CLBCNT_VAL2		0x5c
#define UMP518_FGU_CLBCNT_VAL1		0x60
#define UMP518_FGU_CLBCNT_VAL0		0x64
#define UMP518_FGU_ANA_POCI_H		0x68
#define UMP518_FGU_ANA_POCI_L		0x6c
#define UMP518_FGU_RELAX_CURT_THRE_H		0x70
#define UMP518_FGU_RELAX_CURT_THRE_L		0x74
#define UMP518_FGU_RELAX_STATE_TIME_THRE	0x78
#define UMP518_FGU_LOW_CNT_INT_THRE		0x7c
#define UMP518_FGU_LOW_CNT_TIMER		0x80
#define UMP518_FGU_CURRENT_OFFSET_H	0x84
#define UMP518_FGU_CURRENT_OFFSET_L	0x88
#define UMP518_FGU_USER_AREA_SET0		0x8c
#define UMP518_FGU_USER_AREA_CLEAR0		0x90
#define UMP518_FGU_USER_AREA_STATUS0		0x94
#define UMP518_FGU_POCI_H		0x98
#define UMP518_FGU_POCI_L		0x9c
#define UMP518_FGU_USER_AREA_SET1		0xa0
#define UMP518_FGU_USER_AREA_CLEAR1		0xa4
#define UMP518_FGU_USER_AREA_STATUS1		0xa8
#define UMP518_FGU_VOLTAGE_BUF		0xac
#define UMP518_FGU_CURRENT_BUF_H	0xcc
#define UMP518_FGU_CURRENT_BUF_L	0xd0
#define UMP518_FGU_USER_AREA_SET2		0x120
#define UMP518_FGU_USER_AREA_CLEAR2		0x124
#define UMP518_FGU_USER_AREA_STATUS2		0x128
#define UMP518_FGU_USER_AREA_SET3		0x12c
#define UMP518_FGU_USER_AREA_CLEAR3		0x130
#define UMP518_FGU_USER_AREA_STATUS3		0x134
#define UMP518_FGU_USER_AREA_SET4		0x138
#define UMP518_FGU_USER_AREA_CLEAR4		0x13c
#define UMP518_FGU_USER_AREA_STATUS4		0x140
#define UMP518_FGU_USER_AREA_SET5		0x144
#define UMP518_FGU_USER_AREA_CLEAR5		0x148
#define UMP518_FGU_USER_AREA_STATUS5		0x14c
#define UMP518_FGU_REG_MAX		0x260

/* PMIC global control registers definition */
#define UMP518_MODULE_EN0		0x1808
#define UMP518_CLK_EN0			0x1810
#define UMP518_FGU_EN			BIT(7)
#define UMP518_FGU_RTC_EN		BIT(6)

/* Efuse fgu calibration bit definition */
#define UMP518_FGU_CAL			GENMASK(8, 0)
#define UMP518_FGU_CAL_SHIFT		0

/* 518 Efuse fgu calibration bit definition */
#define UMP518_FGU_VOL_CAL		GENMASK(15, 7)
#define UMP518_FGU_VOL_CAL_SHIFT	7
#define UMP518_FGU_CUR_CAL_42MV 	GENMASK(15, 3)
#define UMP518_FGU_CUR_CAL_42MV_SHIFT	3
#define UMP518_FGU_CUR_CAL_0MV		GENMASK(15, 7)
#define UMP518_FGU_CUR_CAL_0MV_SHIFT	7

/* UMP518_FGU_CONFIG */
#define UMP518_FGU_LOW_POWER_MODE	BIT(1)
#define UMP518_FGU_DISABLE_EN		BIT(11)
#define UMP518_FGU_RELAX_CNT_MODE	0
#define UMP518_FGU_DEEP_SLEEP_MODE	1

/* UMP518_FGU_SEL_CLK_CONFIG */
#define UMP518_FGU_CLK_SEL_MASK			GENMASK(9, 5)
#define UMP518_FGU_RESET_MASK			BIT(1)

/* UMP518_FGU_INT */
#define UMP518_FGU_LOW_OVERLOAD_INT		BIT(0)
#define UMP518_FGU_HIGH_OVERLOAD_INT		BIT(1)
#define UMP518_FGU_CLBCNT_DELTA_INT		BIT(2)
#define UMP518_FGU_POWER_LOW_CNT_INT		BIT(3)
#define UMP518_FGU_LOW_OVERLOAD_INT_SHIFT	0
#define UMP518_FGU_HIGH_OVERLOAD_INT_SHIFT	1
#define UMP518_FGU_CLBCNT_DELTA_INT_SHIFT	2
#define UMP518_FGU_POWER_LOW_CNT_INT_SHIFT	3

/* UMP518_FGU_STS */
#define UMP518_FGU_CLK_SEL_FGU_STS_MASK		BIT(9)
#define UMP518_FGU_CLK_SEL_FGU_STS_SHIFT	9
#define UMP518_FGU_BATTERY_FLAG_STS_MASK	BIT(8)
#define UMP518_FGU_BATTERY_FLAG_STS_SHIFT	8
#define UMP518_FGU_INVALID_POCV_STS_MASK	BIT(7)
#define UMP518_FGU_INVALID_POCV_STS_SHIFT	7
#define UMP518_FGU_WRITE_ACTIVE_STS_MASK	BIT(0)

/* UMP518_FGU_RELAX_CNT */
#define UMP518_FGU_RELAX_CURT_THRE_MASK_L	GENMASK(15, 0)
#define UMP518_FGU_RELAX_CURT_THRE_SHIFT_L	0
#define UMP518_FGU_RELAX_CURT_THRE_MASK_H	GENMASK(4, 0)
#define UMP518_FGU_RELAX_CURT_THRE_SHIFT_H	16
#define UMP518_FGU_RELAX_CURT_THRE_OFFSET	0xf0000
#define UMP518_FGU_RELAX_POWER_STS_MASK		BIT(5)
#define UMP518_FGU_RELAX_POWER_STS_SHIFT	5
#define UMP518_FGU_RELAX_CURT_STS_MASK		BIT(4)
#define UMP518_FGU_RELAX_CURT_STS_SHIFT		4
#define UMP518_FGU_RELAX_STATE_TIME_THRE_MASK	GENMASK(15, 0)
#define UMP518_FGU_LOW_CNT_INT_THRE_MASK	GENMASK(15, 0)

#define UMP518_FGU_POCV_DISABLE_SOFTRESET	BIT(2)
#define UMP518_FGU_WRITE_SELCLB_EN		BIT(0)
#define UMP518_FGU_CLBCNT_MASK			GENMASK(15, 0)
#define UMP518_FGU_HIGH_OVERLOAD_MASK		GENMASK(12, 0)
#define UMP518_FGU_LOW_OVERLOAD_MASK		GENMASK(12, 0)

#define UMP518_FGU_CURRENT_BUFF_CNT		8
#define UMP518_FGU_VOLTAGE_BUFF_CNT		8

#define UMP518_FGU_MODE_AREA_MASK		GENMASK(15, 12)
#define UMP518_FGU_CAP_AREA_MASK		GENMASK(11, 0)
#define UMP518_FGU_CAP_INTEGER_MASK		GENMASK(7, 0)
#define UMP518_FGU_CAP_DECIMAL_MASK		GENMASK(3, 0)
#define UMP518_FGU_FIRST_POWERON		GENMASK(3, 0)
#define UMP518_FGU_DEFAULT_CAP			GENMASK(11, 0)
#define UMP518_FGU_MODE_AREA_SHIFT		12
#define UMP518_FGU_CAP_DECIMAL_SHIFT		8

#define UMP518_FGU_INT_MASK			GENMASK(3, 0)
#define UMP518_FGU_MAGIC_NUMBER			0x5a5aa5a5
#define UMP518_FGU_FCC_PERCENT			1000
#define UMP518_FGU_CLK_512K_SAMPLE_HZ		4
#define UMP518_FGU_WAIT_WRITE_ACTIVE_RETRY_CNT	3

/* relax cnt define */
#define UMP518_FGU_RELAX_CUR_THRESHOLD_MA	30
#define UMP518_FGU_RELAX_STATE_TIME_THRESHOLD	10
#define UMP518_FGU_POWER_LOW_THRESHOLD		320

static s64 init_clbcnt;
static s64 start_work_clbcnt;
static s64 latest_clbcnt;
static int fgu_clk_sample_hz;

static const struct sprd_fgu_variant_data ump518_fgu_info = {
	.module_en = UMP518_MODULE_EN0,
	.clk_en = UMP518_CLK_EN0,
	.fgu_cal = UMP518_FGU_CAL,
	.fgu_cal_shift = UMP518_FGU_CAL_SHIFT,
};

static const struct of_device_id ump518_fgu_dev_match_arr[] = {
	{ .compatible = "sprd,ump518-fgu", .data = &ump518_fgu_info}
};

static inline int ump518_fgu_adc2voltage(struct sprd_fgu_info *info, s64 adc)
{
	return DIV_S64_ROUND_CLOSEST(adc * 1000, info->vol_1000mv_adc);
}

static inline int ump518_fgu_voltage2adc(struct sprd_fgu_info *info, int vol_mv)
{
	return DIV_ROUND_CLOSEST(vol_mv * info->vol_1000mv_adc, 1000);
}

static inline int ump518_fgu_adc2current(struct sprd_fgu_info *info, s64 adc)
{
	return DIV_S64_ROUND_CLOSEST((adc / 15 - info->cur_zero_point_adc) * 1000, info->cur_1000ma_adc);
}

static inline int ump518_fgu_current2adc(struct sprd_fgu_info *info, int cur_ma)
{
	return ((cur_ma * info->cur_1000ma_adc) / 1000 + info->cur_zero_point_adc) * 15;
}

static inline int ump518_fgu_cap2mah(struct sprd_fgu_info *info, int total_mah, int cap)
{
	/*
	 * Get current capacity (mAh) = battery total capacity (mAh) *
	 * current capacity percent (capacity / 100).
	 */
	return DIV_ROUND_CLOSEST(total_mah * cap, UMP518_FGU_FCC_PERCENT);
}

static s64 ump518_fgu_cap2clbcnt(struct sprd_fgu_info *info, int total_mah, int cap)
{
	s64 cur_mah = ump518_fgu_cap2mah(info, total_mah, cap);

	/*
	 * Convert current capacity (mAh) to coulomb counter according to the
	 * formula: 1 mAh =3.6 coulomb.
	 */
	return DIV_ROUND_CLOSEST(cur_mah * 36 * info->cur_1000ma_adc * fgu_clk_sample_hz * 15, 10);
}

static int ump518_fgu_clbcnt2uah(struct sprd_fgu_info *info, s64 clbcnt)
{
	/*
	 * Convert coulomb counter to delta capacity (uAh), and set multiplier
	 * as 10 to improve the precision.
	 * formula: 1000 uAh = 3.6 coulomb
	 */
	s64 uah = DIV_ROUND_CLOSEST(clbcnt * 10 * 1000, 36 * fgu_clk_sample_hz * 15);
	if (uah > 0)
		uah = uah + info->cur_1000ma_adc / 2;
	else
		uah = uah - info->cur_1000ma_adc / 2;

	return (int)div_s64(uah, info->cur_1000ma_adc);
}

static int ump518_fgu_parse_cmdline_match(struct sprd_fgu_info *info, char *match_str,
					  char *result, int size)
{
	struct device_node *cmdline_node = NULL;
	const char *cmdline;
	char *match, *match_end;
	int len, match_str_len, ret;

	if (!result || !match_str)
		return -EINVAL;

	memset(result, '\0', size);
	match_str_len = strlen(match_str);

	cmdline_node = of_find_node_by_path("/chosen");
	ret = of_property_read_string(cmdline_node, "bootargs", &cmdline);
	if (ret) {
		dev_warn(info->dev, "%s failed to read bootargs\n", __func__);
		return ret;
	}

	match = strstr(cmdline, match_str);
	if (!match) {
		dev_warn(info->dev, "Match: %s fail in cmdline\n", match_str);
		return -EINVAL;
	}

	match_end = strstr((match + match_str_len), " ");
	if (!match_end) {
		dev_warn(info->dev, "Match end of : %s fail in cmdline\n", match_str);
		return -EINVAL;
	}

	len = match_end - (match + match_str_len);
	if (len < 0) {
		dev_warn(info->dev, "Match cmdline :%s fail, len = %d\n", match_str, len);
		return -EINVAL;
	}

	memcpy(result, (match + match_str_len), len);

	return 0;
}

static int ump518_fgu_get_calib_resist(struct sprd_fgu_info *info)
{
	char result[32] = {0};
	int ret = 0, calib_resist_ratio = 0, calib_resist = 0;
	char *str = "charge.fgu_res=";

	ret = ump518_fgu_parse_cmdline_match(info, str, result, sizeof(result));
	if (ret) {
		dev_err(info->dev, "parse_cmdline fail, ret = %d\n", ret);
		return ret;
	}

	ret = kstrtoint(result, 10, &calib_resist_ratio);
	if (ret) {
		dev_err(info->dev, "covert calib_resist_ratio fail, ret = %d, result = %s\n",
			ret, result);
		return ret;
	}

	/*
	 * To obtain the scaling factor of a calib resist from the miscdata partion,
	 * need to multiply it by the standard calib resist.
	 */
	calib_resist = calib_resist_ratio * info->standard_calib_resist / 1000;

	if ((calib_resist > info->standard_calib_resist * 8 / 10) &&
	    (calib_resist < info->standard_calib_resist * 12 / 10))
		info->calib_resist = calib_resist;

	dev_info(info->dev, "product line calib_resist = %d, dts define calib_resist = %d.\n",
		 calib_resist, info->standard_calib_resist);

	return ret;
}

static int ump518_fgu_enable_fgu_module(struct sprd_fgu_info *info, bool enable)
{
	int ret = 0;
	u32 reg_val;

	reg_val = enable ? UMP518_FGU_EN : 0;
	ret = regmap_update_bits(info->regmap, info->pdata->module_en, UMP518_FGU_EN, reg_val);
	if (ret) {
		dev_err(info->dev, "failed to %s fgu module!\n", enable ? "enable" : "disable");
		return ret;
	}

	reg_val = enable ? UMP518_FGU_RTC_EN : 0;
	ret = regmap_update_bits(info->regmap, info->pdata->clk_en, UMP518_FGU_RTC_EN, reg_val);
	if (ret)
		dev_err(info->dev, "failed to %s fgu RTC clock!\n", enable ? "enable" : "disable");

	return ret;
}

static inline int ump518_fgu_enable_high_overload_int(struct sprd_fgu_info *info, u32 reg_val)
{
	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_INT_EN,
				  UMP518_FGU_HIGH_OVERLOAD_INT, reg_val);
}

static inline int ump518_fgu_enable_low_overload_int(struct sprd_fgu_info *info, u32 reg_val)
{
	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_INT_EN,
				  UMP518_FGU_LOW_OVERLOAD_INT, reg_val);
}

static inline int ump518_fgu_enable_clbcnt_delta_int(struct sprd_fgu_info *info, u32 reg_val)
{
	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_INT_EN,
				  UMP518_FGU_CLBCNT_DELTA_INT, reg_val);
}

static inline int ump518_fgu_enable_power_low_counter_int(struct sprd_fgu_info *info, u32 reg_val)
{
	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_INT_EN,
				  UMP518_FGU_POWER_LOW_CNT_INT, reg_val);
}

static int ump518_fgu_enable_fgu_int(struct sprd_fgu_info *info,
				     enum sprd_fgu_int_command int_cmd, bool enable)
{
	int ret = 0;
	u32 reg_val;

	switch (int_cmd) {
	case SPRD_FGU_VOLT_LOW_INT_CMD:
		reg_val = enable ? UMP518_FGU_LOW_OVERLOAD_INT : 0;
		ret = ump518_fgu_enable_low_overload_int(info, reg_val);
		if (ret)
			dev_err(info->dev, "failed to %s fgu low overload int!\n",
				enable ? "enable" : "disable");
		break;
	case SPRD_FGU_VOLT_HIGH_INT_CMD:
		reg_val = enable ? UMP518_FGU_HIGH_OVERLOAD_INT : 0;
		ret = ump518_fgu_enable_high_overload_int(info, reg_val);
		if (ret)
			dev_err(info->dev, "failed to %s fgu high overload int!\n",
				enable ? "enable" : "disable");
		break;
	case SPRD_FGU_CLBCNT_DELTA_INT_CMD:
		reg_val = enable ? UMP518_FGU_CLBCNT_DELTA_INT : 0;
		ret = ump518_fgu_enable_clbcnt_delta_int(info, reg_val);
		if (ret)
			dev_err(info->dev, "failed to %s fgu clbcnt delta int!\n",
				enable ? "enable" : "disable");
		break;
	case SPRD_FGU_POWER_LOW_CNT_INT_CMD:
		reg_val = enable ? UMP518_FGU_POWER_LOW_CNT_INT : 0;
		ret = ump518_fgu_enable_power_low_counter_int(info, reg_val);
		if (ret)
			dev_err(info->dev, "failed to %s fgu power low cnt int!\n",
				enable ? "enable" : "disable");
		break;
	default:
		dev_err(info->dev, "%s failed to identify int command!\n", __func__);
		break;
	}

	return ret;
}

static inline int ump518_fgu_enable_relax_cnt_mode(struct sprd_fgu_info *info)
{
	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_CONFIG,
				  UMP518_FGU_LOW_POWER_MODE, UMP518_FGU_RELAX_CNT_MODE);
}

static inline int ump518_fgu_clr_fgu_int(struct sprd_fgu_info *info)
{
	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_INT_CLR,
				  UMP518_FGU_INT_MASK, UMP518_FGU_INT_MASK);
}

static int ump518_fgu_clr_fgu_int_bit(struct sprd_fgu_info *info,
				      enum sprd_fgu_int_command int_cmd)
{
	int ret = 0;
	u32 bit_val = 0;

	switch (int_cmd) {
	case SPRD_FGU_VOLT_LOW_INT_CMD:
		bit_val = UMP518_FGU_LOW_OVERLOAD_INT;
		break;
	case SPRD_FGU_VOLT_HIGH_INT_CMD:
		bit_val = UMP518_FGU_HIGH_OVERLOAD_INT;
		break;
	case SPRD_FGU_CLBCNT_DELTA_INT_CMD:
		bit_val = UMP518_FGU_CLBCNT_DELTA_INT;
		break;
	case SPRD_FGU_POWER_LOW_CNT_INT_CMD:
		bit_val = UMP518_FGU_POWER_LOW_CNT_INT;
		break;
	default:
		dev_err(info->dev, "%s failed to identify int command!\n", __func__);
		break;
	}

	if (bit_val) {
		ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_INT_CLR,
					 bit_val, bit_val);
		if (ret)
			dev_err(info->dev, "failed to clr fgu int, int status = %d\n", int_cmd);
	}

	return ret;
}

static int ump518_fgu_get_fgu_int(struct sprd_fgu_info *info, int *int_sts)

{
	int ret = 0, low_overload_int, high_overload_int, clbcnt_delta_int, power_low_cnt_int;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_INT_STS, int_sts);
	if (ret) {
		dev_err(info->dev, "failed to get fgu int status!\n");
		return ret;
	}

	low_overload_int = (UMP518_FGU_LOW_OVERLOAD_INT & *int_sts) >>
		UMP518_FGU_LOW_OVERLOAD_INT_SHIFT;
	high_overload_int = (UMP518_FGU_HIGH_OVERLOAD_INT & *int_sts) >>
		UMP518_FGU_HIGH_OVERLOAD_INT_SHIFT;
	clbcnt_delta_int = (UMP518_FGU_CLBCNT_DELTA_INT & *int_sts) >>
		UMP518_FGU_CLBCNT_DELTA_INT_SHIFT;
	power_low_cnt_int = (UMP518_FGU_POWER_LOW_CNT_INT & *int_sts) >>
		UMP518_FGU_POWER_LOW_CNT_INT_SHIFT;
	*int_sts = ((low_overload_int << SPRD_FGU_VOLT_LOW_INT_EVENT) |
		    (high_overload_int << SPRD_FGU_VOLT_HIGH_INT_EVENT) |
		    (clbcnt_delta_int << SPRD_FGU_CLBCNT_DELTA_INT_EVENT) |
		    (power_low_cnt_int << SPRD_FGU_POWER_LOW_CNT_INT_EVENT));

	return ret;
}

static int ump518_fgu_get_fgu_sts(struct sprd_fgu_info *info,
				  enum sprd_fgu_sts_command sts_cmd, int *fgu_sts)
{
	int ret = 0;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_STATUS, fgu_sts);
	if (ret) {
		dev_err(info->dev, "failed to get fgu status!, cmd = %d\n", sts_cmd);
		return ret;
	}

	switch (sts_cmd) {
	case SPRD_FGU_CURT_LOW_STS_CMD:
		*fgu_sts = (UMP518_FGU_RELAX_CURT_STS_MASK & *fgu_sts) >>
			UMP518_FGU_RELAX_CURT_STS_SHIFT;
		break;
	case SPRD_FGU_POWER_LOW_STS_CMD:
		*fgu_sts = (UMP518_FGU_RELAX_POWER_STS_MASK & *fgu_sts) >>
			UMP518_FGU_RELAX_POWER_STS_SHIFT;
		break;
	case SPRD_FGU_INVALID_POCV_STS_CMD:
		*fgu_sts = (UMP518_FGU_INVALID_POCV_STS_MASK & *fgu_sts) >>
			UMP518_FGU_INVALID_POCV_STS_SHIFT;
		if (!(*fgu_sts)) {
			ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_START,
						 UMP518_FGU_POCV_DISABLE_SOFTRESET,
						 UMP518_FGU_POCV_DISABLE_SOFTRESET);
			if (ret)
				dev_err(info->dev, "failed to set pocv disable softreset bit.\n");
		}
		break;
	case SPRD_FGU_BATTERY_FLAG_STS_CMD:
		*fgu_sts = (UMP518_FGU_BATTERY_FLAG_STS_MASK & *fgu_sts) >>
			UMP518_FGU_BATTERY_FLAG_STS_SHIFT;
		break;
	case SPRD_FGU_CLK_SEL_FGU_STS_CMD:
		*fgu_sts = (UMP518_FGU_CLK_SEL_FGU_STS_MASK & *fgu_sts) >>
			UMP518_FGU_CLK_SEL_FGU_STS_SHIFT;
		break;
	default:
		dev_err(info->dev, "%s failed to identify sts command!\n", __func__);
		break;
	}

	return ret;
}

static int ump518_fgu_suspend_calib_check_relax_counter_sts(struct sprd_fgu_info *info)
{
	int ret = -EINVAL, cur_sts = 0, power_sts = 0;

	ret = ump518_fgu_get_fgu_sts(info, SPRD_FGU_CURT_LOW_STS_CMD, &cur_sts);
	if (ret) {
		dev_err(info->dev, "failed to get fgu cur low status!\n");
		return ret;
	}
	if (!cur_sts) {
		dev_info(info->dev, "failed enter to realx state,"
			 "get fgu cur_low_sts = %d!\n", cur_sts);
		return -EINVAL;
	}

	ret = ump518_fgu_get_fgu_sts(info, SPRD_FGU_POWER_LOW_STS_CMD, &power_sts);
	if (ret) {
		dev_err(info->dev, "failed to get fgu power low status!\n");
		return ret;
	}
	if (!power_sts) {
		dev_info(info->dev, "failed enter to power low state,"
			 "get fgu power_low_sts = %d!\n", power_sts);
		return -EINVAL;
	}

	dev_info(info->dev, "enter to power low state, power_low_sts = %d!!!\n", power_sts);

	return 0;
}

static int ump518_fgu_set_low_overload(struct sprd_fgu_info *info, int vol)
{
	int adc;

	adc = ump518_fgu_voltage2adc(info, vol);
	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_LOW_OVERLOAD,
				  UMP518_FGU_LOW_OVERLOAD_MASK, adc);
}

static int ump518_fgu_set_high_overload(struct sprd_fgu_info *info, int vol)
{
	int adc;

	adc = ump518_fgu_voltage2adc(info, vol);
	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_HIGH_OVERLOAD,
				  UMP518_FGU_HIGH_OVERLOAD_MASK, adc);
}

static int ump518_fgu_get_calib_efuse(struct sprd_fgu_info *info,
				      char *calib_str, int *calib_data)
{
	struct nvmem_cell *cell;
	void *buf;
	size_t len = 0;

	*calib_data = 0;
	cell = nvmem_cell_get(info->dev, calib_str);
	if (IS_ERR(cell))
		return PTR_ERR(cell);

	buf = nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);

	if (IS_ERR(buf))
		return PTR_ERR(buf);

	memcpy(calib_data, buf, min(len, sizeof(u32)));

	kfree(buf);

	return 0;
}

static int ump518_fgu_calibration(struct sprd_fgu_info *info)
{
	int ret = 0, calib_data1, calib_data2, calib_data3, cal_4200mv, cal_42mv, cal_0mv;

	ret = ump518_fgu_get_calib_resist(info);
	if (ret)
		dev_info(info->dev, "failed to get calib resist from production, "
			 "use the value defined by dts!!!\n");

	/* block38 */
	ret = ump518_fgu_get_calib_efuse(info, "fgu_calib1", &calib_data1);
	if (ret) {
		dev_err(info->dev, "failed to get calib1 efuse\n");
		return ret;
	}

	/* block39 */
	ret = ump518_fgu_get_calib_efuse(info, "fgu_calib2", &calib_data2);
	if (ret) {
		dev_err(info->dev, "failed to get calib2 efuse\n");
		return ret;
	}

	/* block48 */
	ret = ump518_fgu_get_calib_efuse(info, "fgu_calib3", &calib_data3);
	if (ret) {
		dev_err(info->dev, "failed to get calib3 efuse\n");
		return ret;
	}

	/* get current 42mv calibration efuse value*/
	cal_42mv = ((calib_data1 & UMP518_FGU_CUR_CAL_42MV) >>
		    UMP518_FGU_CUR_CAL_42MV_SHIFT) + 104857 - 4096;

	/* get current 0mv calibration efuse value*/
	cal_0mv = ((calib_data3 & UMP518_FGU_CUR_CAL_0MV) >>
		   UMP518_FGU_CUR_CAL_0MV_SHIFT) + 65536 - 256;
	info->cur_zero_point_adc = cal_0mv;

	info->cur_1mv_adc = DIV_ROUND_CLOSEST(cal_42mv - cal_0mv, 42);
	/* unit uA , to avoid being rounded to 0, need to * 1000000 */
	info->cur_1code_lsb =
		DIV_ROUND_CLOSEST(42 * 1000000, ((cal_42mv - cal_0mv) * info->calib_resist / 1000));
	/* calib_resist = 2mΩ */
	info->cur_1000ma_adc =
		DIV_ROUND_CLOSEST(((cal_42mv - cal_0mv) * info->calib_resist / 1000), 42);

	/* get voltage 4200mv calibration efuse value*/
	cal_4200mv = ((calib_data2 & UMP518_FGU_VOL_CAL) >>
		      UMP518_FGU_VOL_CAL_SHIFT) + 6963 - 4096 - 256;
	info->vol_1000mv_adc = DIV_ROUND_CLOSEST(cal_4200mv * 10, 42);

	dev_info(info->dev, "%s cal_0mv = %d, cal_42mv = %d, cur_1mv_adc = %d, "
		 "cur_1code_lsb = %d, cur_1000ma_adc = %d, cal_4200mv = %d, vol_1000mv_adc = %d\n",
		 __func__, cal_0mv, cal_42mv, info->cur_1mv_adc, info->cur_1code_lsb,
		 info->cur_1000ma_adc, cal_4200mv, info->vol_1000mv_adc);

	return 0;
}

static int ump518_fgu_get_vbat_now(struct sprd_fgu_info *info, int *val)
{
	int ret, vol = 0;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_VOLTAGE, &vol);
	if (ret)
		return ret;

	/*
	* It is ADC values reading from registers which need to convert to
	* corresponding voltage values.
	*/
	*val = ump518_fgu_adc2voltage(info, vol);

	return 0;
}

/* @val: average value of battery voltage in mV */
static int ump518_fgu_get_vbat_avg(struct sprd_fgu_info *info, int *val)
{
	int ret, i;
	u32 vol_adc = 0;

	*val = 0;
	for (i = 0; i < UMP518_FGU_VOLTAGE_BUFF_CNT; i++) {
		ret = regmap_read(info->regmap,
				  info->base + UMP518_FGU_VOLTAGE_BUF + i * 4,
				  &vol_adc);
		if (ret)
			return ret;

		/*
		 * It is ADC values reading from registers which need to convert to
		 * corresponding voltage values.
		 */
		*val += ump518_fgu_adc2voltage(info, vol_adc);
	}

	*val /= 8;

	return 0;
}

/* @val: buff value of battery voltage in mV */
static int ump518_fgu_get_vbat_buf(struct sprd_fgu_info *info, int index, int *val)
{
	int ret = 0, vol = 0;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_VOLTAGE_BUF + index * 4, &vol);
	if (ret)
		return ret;
	/*
	* It is ADC values reading from registers which need to convert to
	* corresponding voltage values.
	*/
	*val = ump518_fgu_adc2voltage(info, vol);

	return ret;
}

/* @val: value of battery current in mA*/
static int ump518_fgu_get_current_now(struct sprd_fgu_info *info, int *val)
{
	int ret = 0;
	u32 cur_adc_h = 0, cur_adc_l = 0, cur_adc = 0;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_CURRENT_H, &cur_adc_h);
	if (ret)
		return ret;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_CURRENT_L, &cur_adc_l);
	if (ret)
		return ret;

	cur_adc = (cur_adc_h << 16) | cur_adc_l;
	/*
	 * It is ADC values reading from registers which need to convert to
	 * corresponding current values (unit mA).
	 */
	*val = ump518_fgu_adc2current(info, (s64)cur_adc);

	return ret;
}

/* @val: average value of battery current in mA */
static int ump518_fgu_get_current_avg(struct sprd_fgu_info *info, int *val)
{
	int ret = 0, i;
	u32 cur_adc_h = 0, cur_adc_l = 0, cur_adc = 0;

	*val = 0;

	for (i = 0; i < UMP518_FGU_CURRENT_BUFF_CNT; i++) {
		ret = regmap_read(info->regmap,
				  info->base + UMP518_FGU_CURRENT_BUF_H + i * 8,
				  &cur_adc_h);
		if (ret)
			return ret;

		ret = regmap_read(info->regmap,
				  info->base + UMP518_FGU_CURRENT_BUF_L + i * 8,
				  &cur_adc_l);
		if (ret)
			return ret;

		cur_adc = (cur_adc_h << 16) | cur_adc_l;

		/*
		 * It is ADC values reading from registers which need to convert to
		 * corresponding current values (unit mA).
		 */
		*val += ump518_fgu_adc2current(info, (s64)cur_adc);
	}

	*val /= 8;

	return ret;
}

/* @val: buf value of battery current in mA */
static int ump518_fgu_get_current_buf(struct sprd_fgu_info *info, int index, int *val)
{
	int ret = 0, cur_adc_l = 0, cur_adc_h = 0, cur_adc = 0;

	ret = regmap_read(info->regmap,
			  info->base + UMP518_FGU_CURRENT_BUF_H + index * 8, &cur_adc_h);
	if (ret)
		return ret;

	ret = regmap_read(info->regmap,
			  info->base + UMP518_FGU_CURRENT_BUF_L + index * 8, &cur_adc_l);
	if (ret)
		return ret;

	cur_adc = (cur_adc_h << 16) | cur_adc_l;

	/*
	 * It is ADC values reading from registers which need to convert to
	 * corresponding current values (unit mA).
	 */
	*val = ump518_fgu_adc2current(info, (s64)cur_adc);

	return ret;
}

/*
 * After system booting on, the FGU_ANA_POCI register saved
 * the first sampled open circuit current.
 * @val: value of battery current in mA*
 */
static int ump518_fgu_get_poci(struct sprd_fgu_info *info, int *val)
{
	int ret = 0;
	u32 cur_adc_h = 0, cur_adc_l = 0, cur_adc = 0;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_ANA_POCI_H, &cur_adc_h);
	if (ret)
		return ret;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_ANA_POCI_L, &cur_adc_l);
	if (ret)
		return ret;

	cur_adc = (cur_adc_h << 16) | cur_adc_l;
	/*
	 * It is ADC values reading from registers which need to convert to
	 * corresponding current values (unit mA).
	 */
	*val = ump518_fgu_adc2current(info, (s64)cur_adc);

	return ret;
}

/*
 * Should get the OCV from FGU_POCV register at the system
 * beginning. It is ADC values reading from registers which need to
 * convert the corresponding voltage.
 * @val: value of battery voltage in mV
 */
static int ump518_fgu_get_pocv(struct sprd_fgu_info *info, int *val)
{
	int ret = 0;
	u32 vol_adc = 0;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_POCV, &vol_adc);
	if (ret) {
		dev_err(info->dev, "Failed to read FGU_POCV, ret = %d\n", ret);
		return ret;
	}
	/*
	 * It is ADC values reading from registers which need to convert to
	 * corresponding voltage values.
	 */
	*val = ump518_fgu_adc2voltage(info, vol_adc);

	return ret;
}
static bool ump518_fgu_is_first_poweron(struct sprd_fgu_info *info)
{
	int ret;
	u32 status = 0, cap, mode;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_USER_AREA_STATUS0, &status);
	if (ret)
		return false;

	/*
	 * We use low 12 bits to save the last battery capacity and high 4 bits
	 * to save the system boot mode.
	 */
	mode = (status & UMP518_FGU_MODE_AREA_MASK) >> UMP518_FGU_MODE_AREA_SHIFT;
	cap = status & UMP518_FGU_CAP_AREA_MASK;

	/*
	 * When FGU has been powered down, the user area registers became
	 * default value (0xffff), which can be used to valid if the system is
	 * first power on or not.
	 */
	if (mode == UMP518_FGU_FIRST_POWERON || cap == UMP518_FGU_DEFAULT_CAP)
		return true;

	return false;
}

static int ump518_fgu_save_boot_mode(struct sprd_fgu_info *info, int boot_mode)
{
	int ret;

	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_USER_AREA_CLEAR0,
				 UMP518_FGU_MODE_AREA_MASK,
				 UMP518_FGU_MODE_AREA_MASK);
	if (ret) {
		dev_err(info->dev, "%d Failed to write mode user clr, ret = %d\n", __LINE__, ret);
		return ret;
	}

	/*
	 * Since the user area registers are put on power always-on region,
	 * then these registers changing time will be a little long. Thus
	 * here we should delay 200us to wait until values are updated
	 * successfully according to the datasheet.
	 */
	usleep_range(200, 210);

	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_USER_AREA_SET0,
				 UMP518_FGU_MODE_AREA_MASK,
				 boot_mode << UMP518_FGU_MODE_AREA_SHIFT);
	if (ret) {
		dev_err(info->dev, "Failed to write mode user set, ret = %d\n", ret);
		return ret;
	};

	/*
	 * Since the user area registers are put on power always-on region,
	 * then these registers changing time will be a little long. Thus
	 * here we should delay 200us to wait until values are updated
	 * successfully according to the datasheet.
	 */
	usleep_range(200, 210);

	/*
	 * According to the datasheet, we should set the USER_AREA_CLEAR to 0 to
	 * make the user area data available, otherwise we can not save the user
	 * area data.
	 */
	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_USER_AREA_CLEAR0,
				 UMP518_FGU_MODE_AREA_MASK, 0);
	if (ret) {
		dev_err(info->dev, "%d Failed to write mode user clr, ret = %d\n", __LINE__, ret);
		return ret;
	}

	/*
	 * Since the user area registers are put on power always-on region,
	 * then these registers changing time will be a little long. Thus
	 * here we should delay 200us to wait until values are updated
	 * successfully.
	 */
	usleep_range(200, 210);

	return ret;

}

static int ump518_fgu_read_last_cap(struct sprd_fgu_info *info, int *cap)
{
	int ret;
	unsigned int value = 0;

	ret = regmap_read(info->regmap,
			  info->base + UMP518_FGU_USER_AREA_STATUS0, &value);
	if (ret)
		return ret;

	*cap = (value & UMP518_FGU_CAP_INTEGER_MASK) * 10;
	*cap += (value >> UMP518_FGU_CAP_DECIMAL_SHIFT) & UMP518_FGU_CAP_DECIMAL_MASK;

	return 0;
}

static int ump518_fgu_read_normal_temperature_cap(struct sprd_fgu_info *info, int *cap)
{
	int ret;
	unsigned int value = 0;

	ret = regmap_read(info->regmap,
			  info->base + UMP518_FGU_USER_AREA_STATUS1, &value);
	if (ret)
		return ret;

	*cap = (value & UMP518_FGU_CAP_INTEGER_MASK) * 10;
	*cap += (value >> UMP518_FGU_CAP_DECIMAL_SHIFT) & UMP518_FGU_CAP_DECIMAL_MASK;

	return 0;
}

static int ump518_fgu_save_last_cap(struct sprd_fgu_info *info, int cap)
{
	int ret;
	u32 value;

	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_USER_AREA_CLEAR0,
				 UMP518_FGU_CAP_AREA_MASK,
				 UMP518_FGU_CAP_AREA_MASK);
	if (ret) {
		dev_err(info->dev, "%d Failed to write user clr, ret = %d\n", __LINE__, ret);
		return ret;
	}

	/*
	 * Since the user area registers are put on power always-on region,
	 * then these registers changing time will be a little long. Thus
	 * here we should delay 200us to wait until values are updated
	 * successfully according to the datasheet.
	 */
	usleep_range(200, 210);

	value = (cap / 10) & UMP518_FGU_CAP_INTEGER_MASK;
	value |= ((cap % 10) & UMP518_FGU_CAP_DECIMAL_MASK) << UMP518_FGU_CAP_DECIMAL_SHIFT;

	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_USER_AREA_SET0,
				 UMP518_FGU_CAP_AREA_MASK, value);
	if (ret) {
		dev_err(info->dev, "Failed to write user set, ret = %d\n", ret);
		return ret;
	}

	/*
	 * Since the user area registers are put on power always-on region,
	 * then these registers changing time will be a little long. Thus
	 * here we should delay 200us to wait until values are updated
	 * successfully according to the datasheet.
	 */
	usleep_range(200, 210);

	/*
	 * According to the datasheet, we should set the USER_AREA_CLEAR to 0 to
	 * make the user area data available, otherwise we can not save the user
	 * area data.
	 */
	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_USER_AREA_CLEAR0,
				 UMP518_FGU_CAP_AREA_MASK, 0);
	if (ret) {
		dev_err(info->dev, "%d Failed to write user clr, ret = %d\n", __LINE__, ret);
		return ret;
	}

	/*
	 * Since the user area registers are put on power always-on region,
	 * then these registers changing time will be a little long. Thus
	 * here we should delay 200us to wait until values are updated
	 * successfully.
	 */
	usleep_range(200, 210);

	return ret;
}

/*
 * We get the percentage at the current temperature by multiplying
 * the percentage at normal temperature by the temperature conversion
 * factor, and save the percentage before conversion in the rtc register
 */
static int ump518_fgu_save_normal_temperature_cap(struct sprd_fgu_info *info, int cap)
{
	int ret = 0;
	u32 value;

	if (cap == UMP518_FGU_MAGIC_NUMBER) {
		dev_info(info->dev, "normal_cap = %#x\n", cap);
		return ret;
	}

	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_USER_AREA_CLEAR1,
				 UMP518_FGU_CAP_AREA_MASK,
				 UMP518_FGU_CAP_AREA_MASK);
	if (ret) {
		dev_err(info->dev, "%d Failed to write user clr1, ret = %d\n", __LINE__, ret);
		return ret;
	}
	/*
	 * Since the user area registers are put on power always-on region,
	 * then these registers changing time will be a little long. Thus
	 * here we should delay 200us to wait until values are updated
	 * successfully.
	 */
	usleep_range(200, 210);

	value = (cap / 10) & UMP518_FGU_CAP_INTEGER_MASK;
	value |= ((cap % 10) & UMP518_FGU_CAP_DECIMAL_MASK) << UMP518_FGU_CAP_DECIMAL_SHIFT;

	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_USER_AREA_SET1,
				 UMP518_FGU_CAP_AREA_MASK, value);
	if (ret) {
		dev_err(info->dev, "Failed to write user set1, ret = %d\n", ret);
		return ret;
	}

	/*
	 * Since the user area registers are put on power always-on region,
	 * then these registers changing time will be a little long. Thus
	 * here we should delay 200us to wait until values are updated
	 * successfully.
	 */
	usleep_range(200, 210);

	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_USER_AREA_CLEAR1,
				 UMP518_FGU_CAP_AREA_MASK, 0);
	if (ret) {
		dev_err(info->dev, "%d Failed to write user clr1, ret = %d\n", __LINE__, ret);
		return ret;
	}

	/*
	 * Since the user area registers are put on power always-on region,
	 * then these registers changing time will be a little long. Thus
	 * here we should delay 200us to wait until values are updated
	 * successfully.
	 */
	usleep_range(200, 210);

	return ret;
}

static int ump518_fgu_get_clbcnt(struct sprd_fgu_info *info, s64 *clb_cnt)
{
	int ret = 0, cc0 = 0, cc1 = 0, cc2 = 0;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_CLBCNT_VAL0, &cc0);
	if (ret)
		return ret;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_CLBCNT_VAL1, &cc1);
	if (ret)
		return ret;

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_CLBCNT_VAL2, &cc2);
	if (ret)
		return ret;

	*clb_cnt = cc0 | (cc1 << 16);
	*clb_cnt |= (s64)cc2 << 32;

	return ret;
}

static int ump518_fgu_set_clbcnt(struct sprd_fgu_info *info, s64 clbcnt)
{
	int ret;

	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_CLBCNT_SET0,
				 UMP518_FGU_CLBCNT_MASK, clbcnt);
	if (ret)
		return ret;

	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_CLBCNT_SET1,
				 UMP518_FGU_CLBCNT_MASK,
				 clbcnt >> 16);
	if (ret)
		return ret;

	ret = regmap_update_bits(info->regmap,
				 info->base + UMP518_FGU_CLBCNT_SET2,
				 UMP518_FGU_CLBCNT_MASK,
				 clbcnt >> 32);
	if (ret)
		return ret;

	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_START,
				  UMP518_FGU_WRITE_SELCLB_EN, UMP518_FGU_WRITE_SELCLB_EN);
}

static int ump518_fgu_reset_cc_mah(struct sprd_fgu_info *info, int total_mah, int init_cap)
{
	int ret = 0;

	init_clbcnt = ump518_fgu_cap2clbcnt(info, total_mah, init_cap);
	start_work_clbcnt = latest_clbcnt = init_clbcnt;
	ret = ump518_fgu_set_clbcnt(info, init_clbcnt);
	if (ret)
		dev_err(info->dev, "failed to initialize coulomb counter\n");

	return ret;
}

static int ump518_fgu_get_cc_uah(struct sprd_fgu_info *info, int *cc_uah, bool is_adjust)
{
	int ret = 0;
	s64 cur_clbcnt, delta_clbcnt;

	ret = ump518_fgu_get_clbcnt(info, &cur_clbcnt);
	if (ret) {
		dev_err(info->dev, "%s failed to get cur_clbcnt!\n", __func__);
		return ret;
	}

	latest_clbcnt = cur_clbcnt;

	if (is_adjust)
		delta_clbcnt = cur_clbcnt - init_clbcnt;
	else
		delta_clbcnt = cur_clbcnt - start_work_clbcnt;

	*cc_uah = ump518_fgu_clbcnt2uah(info, delta_clbcnt);

	return ret;
}

static int ump518_fgu_adjust_cap(struct sprd_fgu_info *info, int cap)
{
	int ret;

	ret = ump518_fgu_get_clbcnt(info, &init_clbcnt);
	if (ret)
		dev_err(info->dev, "%s failed to get cur_clbcnt!\n", __func__);

	return cap;
}

static int ump518_fgu_set_cap_delta_thre(struct sprd_fgu_info *info, int total_mah, int cap)
{
	int ret = 0;
	s64 delta_clbcnt;

	delta_clbcnt = ump518_fgu_cap2clbcnt(info, total_mah, cap);

	ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_CLBCNT_DELT0,
				 UMP518_FGU_CLBCNT_MASK, delta_clbcnt);
	if (ret) {
		dev_err(info->dev, "failed to set delta0 coulomb counter\n");
		return ret;
	}

	ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_CLBCNT_DELT1,
				 UMP518_FGU_CLBCNT_MASK, delta_clbcnt >> 16);
	if (ret) {
		dev_err(info->dev, "failed to set delta1 coulomb counter\n");
		return ret;
	}

	ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_CLBCNT_DELT2,
				 UMP518_FGU_CLBCNT_MASK, delta_clbcnt >> 32);
	if (ret)
		dev_err(info->dev, "failed to set delta2 coulomb counter\n");

	return ret;
}

static int ump518_fgu_set_relax_cur_thre(struct sprd_fgu_info *info, int relax_cur_threshold)
{
	int ret = 0, relax_cur_threshold_adc;

	relax_cur_threshold_adc = ump518_fgu_current2adc(info, relax_cur_threshold);
	ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_RELAX_CURT_THRE_H,
				 UMP518_FGU_RELAX_CURT_THRE_MASK_H,
				 (relax_cur_threshold_adc - UMP518_FGU_RELAX_CURT_THRE_OFFSET) >>
				 UMP518_FGU_RELAX_CURT_THRE_SHIFT_H);
	if (ret)
		return ret;

	ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_RELAX_CURT_THRE_L,
				 UMP518_FGU_RELAX_CURT_THRE_MASK_L,
				 (relax_cur_threshold_adc - UMP518_FGU_RELAX_CURT_THRE_OFFSET) >>
				 UMP518_FGU_RELAX_CURT_THRE_SHIFT_L);

	return ret;
}

static inline int ump518_fgu_set_relax_state_time_thre(struct sprd_fgu_info *info, int time)
{
	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_RELAX_STATE_TIME_THRE,
				  UMP518_FGU_RELAX_STATE_TIME_THRE_MASK, time);
}

static inline int ump518_fgu_set_power_low_counter_thre(struct sprd_fgu_info *info, int cnt)
{
	return regmap_update_bits(info->regmap, info->base + UMP518_FGU_LOW_CNT_INT_THRE,
				  UMP518_FGU_LOW_CNT_INT_THRE_MASK, cnt);
}

static int ump518_fgu_relax_mode_config(struct sprd_fgu_info *info)
{
	int ret = 0;

	ret = ump518_fgu_set_relax_cur_thre(info, info->slp_cap_calib.relax_cur_threshold);
	if (ret) {
		dev_err(info->dev, "Sleep calib Fail to set relax_cur_thre, ret= %d\n", ret);
		return ret;
	}

	ret = ump518_fgu_set_relax_state_time_thre(info, info->slp_cap_calib.relax_state_time_threshold);
	if (ret) {
		dev_err(info->dev, "Sleep calib Fail to set relax_state_time_thre, ret= %d\n", ret);
		return ret;
	}

	ret = ump518_fgu_set_power_low_counter_thre(info, info->slp_cap_calib.power_low_counter_threshold);
	if (ret)
		dev_err(info->dev, "Sleep calib Fail to set power_low_counter_thre, ret= %d\n", ret);

	dev_info(info->dev, "%s %d Sleep calib mode config done!!!\n", __func__, __LINE__);

	return ret;
}

static inline int ump518_fgu_get_reg_val(struct sprd_fgu_info *info, int offset, int *reg_val)
{
	return regmap_read(info->regmap, info->base + offset, reg_val);
}

static int ump518_fgu_switch_512k_clk(struct sprd_fgu_info *info)
{
	int ret = 0, cnt = UMP518_FGU_WAIT_WRITE_ACTIVE_RETRY_CNT, reg_val;

	ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_CONFIG,
				 UMP518_FGU_DISABLE_EN, UMP518_FGU_DISABLE_EN);
	if (ret) {
		dev_err(info->dev, "%s failed to disable fgu module!!!\n", __func__);
		return ret;
	}

	ret = regmap_read(info->regmap, info->base + UMP518_FGU_STATUS, &reg_val);
	if (ret) {
		dev_err(info->dev, "%s %d failed to get fgu_sts!!!\n", __func__, __LINE__);
		return ret;
	}

	while ((reg_val & UMP518_FGU_WRITE_ACTIVE_STS_MASK) && cnt--) {
		udelay(50);
		ret = regmap_read(info->regmap, info->base + UMP518_FGU_STATUS, &reg_val);
		if (ret) {
			dev_err(info->dev, "%s %d failed to get fgu_sts!!!\n", __func__, __LINE__);
			return ret;
		}
	}

	if (cnt <= 0) {
		dev_err(info->dev, "%s %d failed to switch 512k clk!!!\n", __func__, __LINE__);
		return -EINVAL;
	}

	ret = regmap_update_bits(info->regmap, info->pdata->clk_en, UMP518_FGU_RTC_EN, 0);
	if (ret) {
		dev_err(info->dev, "%s failed to disable fgu RTC clock!!!\n", __func__);
		return ret;
	}

	ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_ADC_CONFIG1,
				 UMP518_FGU_CLK_SEL_MASK, 0);
	if (ret) {
		dev_err(info->dev, "failed to select 512k fgu clk!!!\n");
		return ret;
	}

	ret = regmap_update_bits(info->regmap, info->pdata->clk_en,
				 UMP518_FGU_RTC_EN, UMP518_FGU_RTC_EN);
	if (ret) {
		dev_err(info->dev, "%s failed to enable fgu RTC clock!!!\n", __func__);
		return ret;
	}

	ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_START,
				 UMP518_FGU_RESET_MASK, UMP518_FGU_RESET_MASK);
	if (ret) {
		dev_err(info->dev, "failed to reset fgu module!!!\n");
		return ret;
	}

	udelay(100);

	ret = regmap_update_bits(info->regmap, info->base + UMP518_FGU_CONFIG,
				 UMP518_FGU_DISABLE_EN, 0);
	if (ret) {
		dev_err(info->dev, "%s failed to enable fgu module!!!\n", __func__);
		return ret;
	}

	/* switch the 512kHZ clk is complete, need to wait for a voltage
	 * sample period (delay_time > 250ms) before reading the voltage.
	 */
	mdelay(252);

	return ret;
}

static int ump518_fgu_get_fgu_sample_clock(struct sprd_fgu_info *info)
{
	int ret = 0, fgu_clk_sts = 0;;

	ret = ump518_fgu_get_fgu_sts(info, SPRD_FGU_CLK_SEL_FGU_STS_CMD, &fgu_clk_sts);
	if (ret) {
		dev_err(info->dev, "%s %d failed get fgu clk sel sts!!!", __func__, __LINE__);
		return ret;
	}

	fgu_clk_sample_hz = UMP518_FGU_CLK_512K_SAMPLE_HZ;

	if (!fgu_clk_sts) {
		ret = ump518_fgu_switch_512k_clk(info);
		if (ret) {
			dev_err(info->dev, "%s failed switch fgu clk to 512kHZ!!!", __func__);
			return ret;
		}

		ret = ump518_fgu_get_fgu_sts(info, SPRD_FGU_CLK_SEL_FGU_STS_CMD, &fgu_clk_sts);
		if (ret || !fgu_clk_sts) {
			dev_err(info->dev, "%s %d failed get fgu clk sel sts or "
				"failed set fgu clk to 512kHZ, ret = %d, fgu_clk_sts = %d!!!",
				__func__, __LINE__, ret, fgu_clk_sts);
			return -EINVAL;
		}
	}

	dev_info(info->dev, "%s the current clock frequency is 512KHZ", __func__);

	return ret;
}

static inline int ump518_fgu_set_reg_val(struct sprd_fgu_info *info, int offset, int reg_val)
{
	return regmap_write(info->regmap, info->base + offset, reg_val);
}

static void ump518_fgu_hw_init(struct sprd_fgu_info *info, struct power_supply *psy)
{
	if (!info) {
		pr_err("%s:line%d: NULL pointer!!!\n", __func__, __LINE__);
		return;
	}
}

static void ump518_fgu_dump_fgu_info(struct sprd_fgu_info *info,
				     enum sprd_fgu_dump_fgu_info_level dump_level)
{
	switch (dump_level) {
	case DUMP_FGU_INFO_LEVEL_0:
		dev_info(info->dev, "dump_level = %d is too low and has no premission to dump fgu info!!!");
		break;
	case DUMP_FGU_INFO_LEVEL_1:
		dev_info(info->dev, "ump518_fgu_info : init_clbcnt = %lld, "
			 "start_work_clbcnt = %lld, cur_clbcnt = %lld, "
			 "cur_1000ma_adc = %d, vol_1000mv_adc = %d, "
			 "fgu_clk_sample_hz = %d, calib_resist = %d\n",
			 init_clbcnt, start_work_clbcnt, latest_clbcnt,
			 info->cur_1000ma_adc, info->vol_1000mv_adc,
			 fgu_clk_sample_hz, info->calib_resist);
		break;
	default:
		dev_err(info->dev, "failed to identify dump_level or dump_level is greater than %d!\n",
			DUMP_FGU_INFO_LEVEL_1);
		break;
	}
}

static void ump518_fgu_remove(struct sprd_fgu_info *info)
{
	if (!info) {
		pr_err("%s:line%d: NULL pointer!!!\n", __func__, __LINE__);
		return;
	}

}

static void ump518_fgu_shutdown(struct sprd_fgu_info *info)
{
	if (!info) {
		pr_err("%s:line%d: NULL pointer!!!\n", __func__, __LINE__);
		return;
	}
}

struct sprd_fgu_device_ops ump518_fgu_dev_ops = {
	.enable_fgu_module = ump518_fgu_enable_fgu_module,
	.get_fgu_sts = ump518_fgu_get_fgu_sts,
	.clr_fgu_int = ump518_fgu_clr_fgu_int,
	.clr_fgu_int_bit = ump518_fgu_clr_fgu_int_bit,
	.enable_relax_cnt_mode = ump518_fgu_enable_relax_cnt_mode,
	.set_low_overload = ump518_fgu_set_low_overload,
	.set_high_overload = ump518_fgu_set_high_overload,
	.enable_fgu_int = ump518_fgu_enable_fgu_int,
	.get_fgu_int = ump518_fgu_get_fgu_int,
	.suspend_calib_check_relax_counter_sts = ump518_fgu_suspend_calib_check_relax_counter_sts,
	.cap2mah = ump518_fgu_cap2mah,
	.get_vbat_now = ump518_fgu_get_vbat_now,
	.get_vbat_avg = ump518_fgu_get_vbat_avg,
	.get_vbat_buf = ump518_fgu_get_vbat_buf,
	.get_current_now = ump518_fgu_get_current_now,
	.get_current_avg = ump518_fgu_get_current_avg,
	.get_current_buf = ump518_fgu_get_current_buf,
	.reset_cc_mah = ump518_fgu_reset_cc_mah,
	.get_cc_uah = ump518_fgu_get_cc_uah,
	.adjust_cap = ump518_fgu_adjust_cap,
	.set_cap_delta_thre = ump518_fgu_set_cap_delta_thre,
	.relax_mode_config = ump518_fgu_relax_mode_config,
	.get_poci = ump518_fgu_get_poci,
	.get_pocv = ump518_fgu_get_pocv,
	.fgu_calibration = ump518_fgu_calibration,
	.is_first_poweron = ump518_fgu_is_first_poweron,
	.save_boot_mode = ump518_fgu_save_boot_mode,
	.read_last_cap = ump518_fgu_read_last_cap,
	.save_last_cap = ump518_fgu_save_last_cap,
	.read_normal_temperature_cap = ump518_fgu_read_normal_temperature_cap,
	.save_normal_temperature_cap = ump518_fgu_save_normal_temperature_cap,
	.get_reg_val = ump518_fgu_get_reg_val,
	.set_reg_val = ump518_fgu_set_reg_val,
	.hw_init = ump518_fgu_hw_init,
	.dump_fgu_info = ump518_fgu_dump_fgu_info,
	.remove = ump518_fgu_remove,
	.shutdown = ump518_fgu_shutdown,
};

struct sprd_fgu_info *sprd_fgu_info_register(struct device *dev)
{
	struct sprd_fgu_info *info = NULL;
	int ret = 0, i;
	const char *value = {0};
	struct device_node *np = dev->of_node;

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_err(dev, "%s: %d Fail to malloc memory for fgu_info\n", __func__, __LINE__);
		return ERR_PTR(-ENOMEM);
	}

	info->ops = devm_kzalloc(dev, sizeof(*info->ops), GFP_KERNEL);
	if (!info->ops) {
		dev_err(dev, "%s: %d Fail to malloc memory for ops\n", __func__, __LINE__);
		return ERR_PTR(-ENOMEM);
	}

	info->dev = dev;

	info->regmap = dev_get_regmap(dev->parent, NULL);
	if (!info->regmap) {
		dev_err(dev, "%s: %d failed to get regmap\n", __func__, __LINE__);
		return ERR_PTR(-ENODEV);
	}

	ret = of_property_read_string_index(np, "compatible", 0, &value);
	if (ret) {
		dev_err(dev, "read_string failed!\n");
		return ERR_PTR(ret);
	}

	for (i = 0; i < ARRAY_SIZE(ump518_fgu_dev_match_arr); i++) {
		if (strcmp(ump518_fgu_dev_match_arr[i].compatible, value) == 0) {
			info->pdata = ump518_fgu_dev_match_arr[i].data;
			break;
		}
	}

	ret = device_property_read_u32(dev, "reg", &info->base);
	if (ret) {
		dev_err(dev, "failed to get fgu address\n");
		return ERR_PTR(ret);
	}

	ret = device_property_read_u32(dev, "sprd,calib-resistance-micro-ohms",
				       &info->calib_resist);
	if (ret) {
		dev_err(dev, "failed to get fgu calibration resistance\n");
		return ERR_PTR(ret);
	}
	info->standard_calib_resist = info->calib_resist;

	/* parse sleep calibration parameters from dts */
	info->slp_cap_calib.support_slp_calib =
		device_property_read_bool(dev, "sprd,capacity-sleep-calibration");
	if (!info->slp_cap_calib.support_slp_calib) {
		dev_warn(dev, "Do not support sleep calibration function\n");
	} else {
		ret = device_property_read_u32(dev, "sprd,relax-current-threshold",
					       &info->slp_cap_calib.relax_cur_threshold);
		if (ret)
			dev_warn(dev, "no relax_current_threshold support\n");

		ret = device_property_read_u32(dev, "sprd,relax-state-time-threshold",
					       &info->slp_cap_calib.relax_state_time_threshold);
		if (ret)
			dev_warn(dev, "no relax_state_time_threshold support\n");

		ret = device_property_read_u32(dev, "sprd,power-low-counter-threshold",
					       &info->slp_cap_calib.power_low_counter_threshold);
		if (ret)
			dev_warn(dev, "no power_low_counter_threshold support\n");

		if (info->slp_cap_calib.relax_cur_threshold == 0)
			info->slp_cap_calib.relax_cur_threshold = UMP518_FGU_RELAX_CUR_THRESHOLD_MA;

		if (info->slp_cap_calib.relax_state_time_threshold < UMP518_FGU_RELAX_STATE_TIME_THRESHOLD)
			info->slp_cap_calib.relax_state_time_threshold = UMP518_FGU_RELAX_STATE_TIME_THRESHOLD;

		if (info->slp_cap_calib.power_low_counter_threshold < UMP518_FGU_POWER_LOW_THRESHOLD)
			info->slp_cap_calib.power_low_counter_threshold = UMP518_FGU_POWER_LOW_THRESHOLD;
	}

	ret = ump518_fgu_get_fgu_sample_clock(info);
	if (ret) {
		dev_err(dev, "%s failed get fgu sample clock!!!", __func__);
		return ERR_PTR(ret);
	}

	info->ops = &ump518_fgu_dev_ops;

	return info;
}
EXPORT_SYMBOL_GPL(sprd_fgu_info_register);

MODULE_LICENSE("GPL v2");
