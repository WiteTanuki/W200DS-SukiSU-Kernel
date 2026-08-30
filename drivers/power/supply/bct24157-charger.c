/*
 * Driver for the FAIRCHILD bct24157 charger.
 * Author: Mark A. Greer <mgreer@animalcreek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/alarmtimer.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/power/charger-manager.h>
#include <linux/power/sprd_battery_info.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/slab.h>
#include <linux/usb/phy.h>
#include <uapi/linux/usb/charger.h>

#define BCT24157_REG_NUM	17
#define BCT24157_REG_0	0x00
#define BCT24157_REG_1	0x01
#define BCT24157_REG_2	0x02
#define BCT24157_REG_3	0x03
#define BCT24157_REG_4	0x04
#define BCT24157_REG_5	0x05
#define BCT24157_REG_6	0x06
#define BCT24157_REG_7	0x07
#define BCT24157_REG_8	0x08
#define BCT24157_REG_9	0x09
#define BCT24157_REG_A	0x0A
#define BCT24157_REG_B	0x0B
#define BCT24157_REG_C	0x0C
#define BCT24157_REG_D	0x0D
#define BCT24157_REG_E	0x0E
#define BCT24157_REG_F	0x0F
#define BCT24157_REG_10	0x10

#define BCT24157_BATTERY_NAME				"sc27xx-fgu"
#define BIT_DP_DM_BC_ENB				BIT(0)
//#define BCT24157_OTG_VALID_MS				500
//#define BCT24157_FEED_WATCHDOG_VALID_MS			50
#define BCT24157_OTG_ALARM_TIMER_MS			15000

/* BCT24157_REG_0 (0x00) */
#define	BCT24157_CHARGE_EN_MASK					GENMASK(6, 6)
#define	BCT24157_CHARGE_EN_SHIFT				6
#define BCT24157_CHARGE_EN						0
#define BCT24157_CHARGE_DISABLE					1
#define BCT24157_OTG_EN_MASK					GENMASK(7, 7)
#define BCT24157_OTG_EN							BIT(7)

/* BCT24157_REG_2 (0x02) */
#define BCT24157_CHARGE_CURRENT_MASK			GENMASK(3, 0)
#define BCT24157_CHARGE_CURRENT_300MA		0x03
#define BCT24157_CHARGE_CURRENT_400MA		0x04
#define BCT24157_CHARGE_CURRENT_500MA		0x05
#define BCT24157_CHARGE_CURRENT_600MA		0x06
#define BCT24157_CHARGE_CURRENT_700MA		0x07
#define BCT24157_CHARGE_CURRENT_800MA		0x08
#define BCT24157_CHARGE_CURRENT_900MA		0x09
#define BCT24157_CHARGE_CURRENT_1000MA		0x0A
#define BCT24157_CHARGE_CURRENT_1100MA		0x0B
#define BCT24157_CHARGE_CURRENT_1200MA		0x0C
#define BCT24157_CHARGE_CURRENT_1300MA		0x0D
#define BCT24157_CHARGE_CURRENT_1400MA		0x0E
#define BCT24157_CHARGE_CURRENT_1500MA		0x0F

#define BCT24157_CHARGE_TERMINAL_CURRENT_MASK		GENMASK(7, 5)
#define BCT24157_CHARGE_TERMINAL_CURRENT_SHIFT		5
#define BCT24157_CHARGE_TERMINAL_CURRENT_100MA		0x01
#define BCT24157_CHARGE_TERMINAL_CURRENT_150MA		0x02
#define BCT24157_CHARGE_TERMINAL_CURRENT_200MA		0x03
#define BCT24157_CHARGE_TERMINAL_CURRENT_250MA		0x04
#define BCT24157_CHARGE_TERMINAL_CURRENT_300MA		0x05
#define BCT24157_CHARGE_TERMINAL_CURRENT_350MA		0x06
#define BCT24157_CHARGE_TERMINAL_CURRENT_400MA		0x07

/* BCT24157_REG_3 (0x03) */
#define BCT24157_CONSTANT_VOLTAGE_MAX_MASK			GENMASK(2, 0)
#define BCT24157_CONSTANT_VOLTAGE_MAX_4200MV		0x00
#define BCT24157_CONSTANT_VOLTAGE_MAX_4250MV		0x01
#define BCT24157_CONSTANT_VOLTAGE_MAX_4300MV		0x02
#define BCT24157_CONSTANT_VOLTAGE_MAX_4350MV		0x03
#define BCT24157_CONSTANT_VOLTAGE_MAX_4400MV		0x04
#define BCT24157_CONSTANT_VOLTAGE_MAX_4450MV		0x05

#define BCT24157_BOOST_VOLTAGE_MAX_MASK			GENMASK(7, 6)
#define BCT24157_BOOST_VOLTAGE_MAX_SHIFT		6
#define BCT24157_BOOST_VOLTAGE_MAX_4800MV		0x00
#define BCT24157_BOOST_VOLTAGE_MAX_5000MV		0x01
#define BCT24157_BOOST_VOLTAGE_MAX_5200MV		0x02
#define BCT24157_BOOST_VOLTAGE_MAX_5400MV		0x03

#define BCT24157_REG_FAULT_MASK				0x7
#define BCT24157_OTG_TIMER_FAULT			0x6
#define BCT24157_REG_HZ_MODE_MASK			GENMASK(1, 1)
#define BCT24157_REG_OPA_MODE_MASK			GENMASK(0, 0)

#define BCT24157_REG_SAFETY_VOL_MASK			GENMASK(3, 0)
#define BCT24157_REG_SAFETY_CUR_MASK			GENMASK(6, 4)

#define BCT24157_REG_RESET_MASK				GENMASK(7, 7)
#define BCT24157_REG_RESET				BIT(7)

#define BCT24157_REG_WEAK_VOL_THRESHOLD_MASK		GENMASK(5, 4)

#define BCT24157_REG_IO_LEVEL_MASK			GENMASK(5, 5)

#define BCT24157_REG_VSP_MASK				GENMASK(2, 0)
#define BCT24157_REG_VSP				(BIT(2) | BIT(0))

#define BCT24157_REG_TERMINAL_CURRENT_MASK		GENMASK(3, 3)
#define BCT24157_REG_TERMINAL_VOLTAGE_MASK		GENMASK(7, 2)
#define BCT24157_REG_TERMINAL_VOLTAGE_SHIFT		2

#define BCT24157_REG_CHARGE_CONTROL_MASK		GENMASK(2, 2)
#define BCT24157_REG_CHARGE_DISABLE			BIT(2)
#define BCT24157_REG_CHARGE_ENABLE			0

#define BCT24157_REG_CURRENT_MASK			GENMASK(6, 4)
#define BCT24157_REG_CURRENT_MASK_SHIFT			4

//#define BCT24157_REG_LIMIT_CURRENT_MASK			GENMASK(7, 6)
//#define BCT24157_REG_LIMIT_CURRENT_SHIFT		6

#define BCT24157_REG_RECHARGE_MASK			GENMASK(6, 4)
#define BCT24157_REG_RECHARGE_MASK_SHIFT			4

#define BCT24157_DISABLE_PIN_MASK_2730			BIT(0)
#define BCT24157_DISABLE_PIN_MASK_2721			BIT(15)
#define BCT24157_DISABLE_PIN_MASK_2720			BIT(0)

struct bct24157_charger_reg_tab {
	int id;
	u32 addr;
	char *name;
};

static struct bct24157_charger_reg_tab reg_tab[BCT24157_REG_NUM + 1] = {
	{0, BCT24157_REG_0, "Setting Input Limit Current reg"},
	{1, BCT24157_REG_1, "Setting Vindpm_OS reg"},
	{2, BCT24157_REG_2, "Related Function Enable reg"},
	{3, BCT24157_REG_3, "Related Function Config reg"},
	{4, BCT24157_REG_4, "Setting Charge Limit Current reg"},
	{5, BCT24157_REG_5, "Setting Terminal Current reg"},
	{6, BCT24157_REG_6, "Setting Charge Limit Voltage reg"},
	{7, BCT24157_REG_7, "Related Function Config reg"},
	{8, BCT24157_REG_8, "IR Compensation Resistor Setting reg"},
	{9, BCT24157_REG_9, "Related Function Config reg"},
	{10, BCT24157_REG_A, "Boost Mode Related Setting reg"},
	{11, BCT24157_REG_B, "Status reg"},
	{12, BCT24157_REG_C, "Fault reg"},
	{13, BCT24157_REG_D, "Setting Vindpm reg"},
	{14, BCT24157_REG_E, "ADC Conversion of Battery Voltage reg"},
	{15, BCT24157_REG_F, "ADDC Conversion of System Voltage reg"},
	{16, BCT24157_REG_10, "ADC Conversion of TS Voltage as Percentage of REGN reg"},
	{17, 0, "null"},
};

struct bct24157_charge_current {
	int sdp_limit;
	int sdp_cur;
	int dcp_limit;
	int dcp_cur;
	int cdp_limit;
	int cdp_cur;
	int unknown_limit;
	int unknown_cur;
};

struct bct24157_charger_info {
	struct i2c_client *client;
	struct device *dev;
	struct usb_phy *usb_phy;
	struct notifier_block usb_notify;
	struct power_supply *psy_usb;
	struct bct24157_charge_current cur;
	struct work_struct work;
	struct mutex lock;
	bool charging;
	u32 limit;
//	struct delayed_work otg_work;
//	struct delayed_work wdt_work;
	struct regmap *pmic;
	u32 charger_detect;
	u32 charger_pd;
	u32 charger_pd_mask;
	struct gpio_desc *gpiod;
	struct extcon_dev *edev;
	bool otg_enable;
	struct alarm otg_timer;
};

static int bct24157_read(struct bct24157_charger_info *info, u8 cmd, u8 *data)
{
    unsigned char pBuff;
	unsigned char puSendCmd[1];
	puSendCmd[0] = cmd;

	pr_info("%s:enter, i2c_addr=0x%x\n", __func__, info->client->addr);
	if ( i2c_master_send(info->client, puSendCmd, 1) < 0 ) {
		pr_info("%s:send failed!!\n", __func__);
		return -1;
	}

	if ( i2c_master_recv(info->client, &pBuff, 1) < 0 ) {
		pr_info("%s:recv failed!!\n", __func__);
		return -1;
	}
	*data = pBuff;

    return 0;
}

static int bct24157_write(struct bct24157_charger_info *info, u8 cmd, u8 data)
{
	unsigned char write_buf[2] = {cmd, data};

	pr_info("%s:enter, i2c_addr=0x%x\n", __func__, info->client->addr);
	if (i2c_master_send(info->client, write_buf, 2) < 0) {
		pr_info("%s:send failed!!\n", __func__);
		return -1;
	}

	return 0;
}

static int bct24157_update_bits(struct bct24157_charger_info *info, u8 reg, u8 mask, u8 data)
{
	u8 v;
	int ret;

	ret = bct24157_read(info, reg, &v);
	pr_info("%s:reg=%d, v=0x%x\n", __func__, reg, v);
	if (ret < 0){
		pr_info("%s:bct24157_read failed\n", __func__);
		return ret;
	}
	v &= ~mask;
	v |= (data & mask);

	return bct24157_write(info, reg, v);
}

static void bct24157_charger_dump_register(struct bct24157_charger_info *info)
{
	int i, ret, len, idx = 0;
	u8 reg_val;
	char buf[512];

	memset(buf, '\0', sizeof(buf));
	for (i = 0; i < BCT24157_REG_NUM; i++) {
		ret = bct24157_read(info, reg_tab[i].addr, &reg_val);
		if (ret == 0) {
			len = snprintf(buf + idx, sizeof(buf) - idx,
							"[REG_0x%.2x]=0x%.2x; ", reg_tab[i].addr,
							reg_val);
			idx += len;
		}
	}

	dev_info(info->dev, "%s: %s", __func__, buf);
}

static int bct24157_charger_set_termina_current(struct bct24157_charger_info *info, u32 cur)
{
	u8 reg_val;

	pr_info("%s:enter\n", __func__);
	if (cur < 150000)
		reg_val = BCT24157_CHARGE_TERMINAL_CURRENT_100MA;
	else if (cur < 200000)
		reg_val = BCT24157_CHARGE_TERMINAL_CURRENT_150MA;
	else if (cur < 250000)
		reg_val = BCT24157_CHARGE_TERMINAL_CURRENT_200MA;
	else if (cur < 300000)
		reg_val = BCT24157_CHARGE_TERMINAL_CURRENT_250MA;
	else if (cur < 350000)
		reg_val = BCT24157_CHARGE_TERMINAL_CURRENT_300MA;
	else if (cur < 400000)
		reg_val = BCT24157_CHARGE_TERMINAL_CURRENT_350MA;
	else if (cur == 400000)
		reg_val = BCT24157_CHARGE_TERMINAL_CURRENT_400MA;
	else
		reg_val = BCT24157_CHARGE_TERMINAL_CURRENT_400MA;

	return bct24157_update_bits(info, BCT24157_REG_2, BCT24157_CHARGE_TERMINAL_CURRENT_MASK,
				reg_val << BCT24157_CHARGE_TERMINAL_CURRENT_SHIFT);
}

static int bct24157_charger_get_termina_vol(struct bct24157_charger_info *info, u32 *vol)
{
	u8 reg_val;
	int ret;

	ret = bct24157_read(info, BCT24157_REG_3, &reg_val);
	if (ret < 0)
		return ret;

	reg_val &= BCT24157_CONSTANT_VOLTAGE_MAX_MASK;
	switch (reg_val) {
	case BCT24157_CONSTANT_VOLTAGE_MAX_4450MV:
		*vol = 4450;
		break;
	case BCT24157_CONSTANT_VOLTAGE_MAX_4400MV:
		*vol = 4400;
		break;
	case BCT24157_CONSTANT_VOLTAGE_MAX_4350MV:
		*vol = 4350;
		break;
	case BCT24157_CONSTANT_VOLTAGE_MAX_4300MV:
		*vol = 4300;
		break;
	case BCT24157_CONSTANT_VOLTAGE_MAX_4250MV:
		*vol = 4250;
		break;
	case BCT24157_CONSTANT_VOLTAGE_MAX_4200MV:
		*vol = 4200;
		break;
	default:
		*vol = 4200;
	}
	pr_info("%s:enter, vol=%d\n", __func__, *vol);

	return 0;
}

static int bct24157_charger_set_recharge_vol(struct bct24157_charger_info *info)
{
	return bct24157_update_bits(info, BCT24157_REG_4, BCT24157_REG_RECHARGE_MASK, 0x1 << BCT24157_REG_RECHARGE_MASK_SHIFT);
}

static int bct24157_charger_set_termina_vol(struct bct24157_charger_info *info, u32 vol)
{
	u8 reg_val;

	pr_info("%s:enter, vol=%d\n", __func__, vol);
	if (vol >= 4450)
		reg_val = BCT24157_CONSTANT_VOLTAGE_MAX_4450MV;
	else if (vol >= 4400)
		reg_val = BCT24157_CONSTANT_VOLTAGE_MAX_4400MV;
	else if (vol >= 4350)
		reg_val = BCT24157_CONSTANT_VOLTAGE_MAX_4350MV;
	else if (vol >= 4300)
		reg_val = BCT24157_CONSTANT_VOLTAGE_MAX_4300MV;
	else if (vol >= 4250)
		reg_val = BCT24157_CONSTANT_VOLTAGE_MAX_4250MV;
	else
		reg_val = BCT24157_CONSTANT_VOLTAGE_MAX_4200MV;

	return bct24157_update_bits(info, BCT24157_REG_3, BCT24157_CONSTANT_VOLTAGE_MAX_MASK, reg_val);
}

static int bct24157_charger_set_boost_vol(struct bct24157_charger_info *info, u32 vol)
{
	u8 reg_val;

	pr_info("%s:enter, vol=%d\n", __func__, vol);
	if (vol >= 5400)
		reg_val = BCT24157_BOOST_VOLTAGE_MAX_5400MV;
	else if (vol >= 5200)
		reg_val = BCT24157_BOOST_VOLTAGE_MAX_5200MV;
	else if (vol >= 5000)
		reg_val = BCT24157_BOOST_VOLTAGE_MAX_5000MV;
	else
		reg_val = BCT24157_BOOST_VOLTAGE_MAX_4800MV;

	return bct24157_update_bits(info, BCT24157_REG_3, BCT24157_BOOST_VOLTAGE_MAX_MASK,
				reg_val << BCT24157_BOOST_VOLTAGE_MAX_SHIFT);
}

static int bct24157_charger_hw_init(struct bct24157_charger_info *info)
{
	struct sprd_battery_info bat_info = { };
	int voltage_max_microvolt, current_max_ua;
	int ret;

	pr_info("%s:enter\n", __func__);
	ret = sprd_battery_get_battery_info(info->psy_usb, &bat_info);
	if (ret) {
		dev_warn(info->dev, "no battery information is supplied\n");

		/*
		 * If no battery information is supplied, we should set
		 * default charge termination current to 100 mA, and default
		 * charge termination voltage to 4.2V.
		 */
		info->cur.sdp_limit = 500000;
		info->cur.sdp_cur = 500000;
		info->cur.dcp_limit = 5000000;
		info->cur.dcp_cur = 500000;
		info->cur.cdp_limit = 5000000;
		info->cur.cdp_cur = 1500000;
		info->cur.unknown_limit = 5000000;
		info->cur.unknown_cur = 500000;
	} else {
		info->cur.sdp_limit = bat_info.cur.sdp_limit;
		info->cur.sdp_cur = bat_info.cur.sdp_cur;
		info->cur.dcp_limit = bat_info.cur.dcp_limit;
		info->cur.dcp_cur = bat_info.cur.dcp_cur;
		info->cur.cdp_limit = bat_info.cur.cdp_limit;
		info->cur.cdp_cur = bat_info.cur.cdp_cur;
		info->cur.unknown_limit = bat_info.cur.unknown_limit;
		info->cur.unknown_cur = bat_info.cur.unknown_cur;

		voltage_max_microvolt =
			bat_info.constant_charge_voltage_max_uv / 1000;
		current_max_ua = bat_info.constant_charge_current_max_ua / 1000;
		sprd_battery_put_battery_info(info->psy_usb, &bat_info);

		ret = bct24157_charger_set_termina_vol(info, voltage_max_microvolt);
		if (ret) {
			dev_err(info->dev, "set bct24157 terminal vol failed\n");
			return ret;
		}
		ret = bct24157_charger_set_recharge_vol(info);
		if (ret) {
			dev_err(info->dev, "set bct24157 recharge vol failed\n");
			return ret;
		}
		ret = bct24157_charger_set_termina_current(info, bat_info.charge_term_current_ua);
		if (ret) {
			dev_err(info->dev, "set bct24157 terminal vol failed\n");
			return ret;
		}
		ret = bct24157_charger_set_boost_vol(info, 5200);
		if (ret) {
			dev_err(info->dev, "set bct24157 boost vol failed\n");
			return ret;
		}
	}

	bct24157_charger_dump_register(info);
	return ret;
}

static int bct24157_charger_start_charge(struct bct24157_charger_info *info)
{
	int ret;

	ret = bct24157_update_bits(info, BCT24157_REG_0, BCT24157_CHARGE_EN_MASK, BCT24157_CHARGE_EN);
	if (ret)
		dev_err(info->dev, "enable bct24157 charge failed\n");

	return ret;
}

static void bct24157_charger_stop_charge(struct bct24157_charger_info *info)
{
	int ret;

	ret = bct24157_update_bits(info, BCT24157_REG_0, BCT24157_CHARGE_EN_MASK, BCT24157_CHARGE_DISABLE);
	if (ret)
		dev_err(info->dev, "disable bct24157 charge failed\n");
}

static int bct24157_charger_is_enabled(struct bct24157_charger_info *info)
{
	int ret;
	u8 val;

	ret = bct24157_read(info, BCT24157_REG_0, &val);
	if (ret) {
		pr_info("%s read failed\n", __func__);
		return 0;
	}
	pr_info("%s reg0=%d\n", __func__, val);
	val &= BCT24157_CHARGE_EN_MASK;
	val = val >> BCT24157_CHARGE_EN_SHIFT;

	ret = val ? 0 : 1;
	pr_info("%s %s\n", __func__, ret ? "enabled" : "disabled");

	return ret;
}

static int bct24157_charger_set_current(struct bct24157_charger_info *info, u32 cur)
{
	u8 reg_val;

	pr_info("%s:enter\n", __func__);
	if (cur < 400000)
		reg_val = BCT24157_CHARGE_CURRENT_300MA;
	else if (cur < 500000)
		reg_val = BCT24157_CHARGE_CURRENT_400MA;
	else if (cur < 600000)
		reg_val = BCT24157_CHARGE_CURRENT_500MA;
	else if (cur < 700000)
		reg_val = BCT24157_CHARGE_CURRENT_600MA;
	else if (cur < 800000)
		reg_val = BCT24157_CHARGE_CURRENT_700MA;
	else if (cur < 900000)
		reg_val = BCT24157_CHARGE_CURRENT_800MA;
	else if (cur < 1000000)
		reg_val = BCT24157_CHARGE_CURRENT_900MA;
	else if (cur < 1100000)
		reg_val = BCT24157_CHARGE_CURRENT_1000MA;
	else if (cur < 1200000)
		reg_val = BCT24157_CHARGE_CURRENT_1100MA;
	else if (cur < 1300000)
		reg_val = BCT24157_CHARGE_CURRENT_1200MA;
	else if (cur < 1400000)
		reg_val = BCT24157_CHARGE_CURRENT_1300MA;
	else if (cur < 1500000)
		reg_val = BCT24157_CHARGE_CURRENT_1400MA;
	else if (cur == 1500000)
		reg_val = BCT24157_CHARGE_CURRENT_1500MA;
	else
		reg_val = BCT24157_CHARGE_CURRENT_500MA;

	return bct24157_update_bits(info, BCT24157_REG_2,BCT24157_CHARGE_CURRENT_MASK, reg_val);
}

static int bct24157_charger_get_current(struct bct24157_charger_info *info, u32 *cur)
{
	u8 reg_val;
	int ret;

	ret = bct24157_read(info, BCT24157_REG_2, &reg_val);
	if (ret < 0)
		return ret;

	reg_val &= BCT24157_CHARGE_CURRENT_MASK;

	switch (reg_val) {
	case BCT24157_CHARGE_CURRENT_300MA:
		*cur = 300000;
		break;
	case BCT24157_CHARGE_CURRENT_400MA:
		*cur = 400000;
		break;
	case BCT24157_CHARGE_CURRENT_500MA:
		*cur = 500000;
		break;
	case BCT24157_CHARGE_CURRENT_600MA:
		*cur = 600000;
		break;
	case BCT24157_CHARGE_CURRENT_700MA:
		*cur = 700000;
		break;
	case BCT24157_CHARGE_CURRENT_800MA:
		*cur = 800000;
		break;
	case BCT24157_CHARGE_CURRENT_900MA:
		*cur = 900000;
		break;
	case BCT24157_CHARGE_CURRENT_1000MA:
		*cur = 1000000;
		break;
	case BCT24157_CHARGE_CURRENT_1100MA:
		*cur = 1100000;
		break;
	case BCT24157_CHARGE_CURRENT_1200MA:
		*cur = 1200000;
		break;
	case BCT24157_CHARGE_CURRENT_1300MA:
		*cur = 1300000;
		break;
	case BCT24157_CHARGE_CURRENT_1400MA:
		*cur = 1400000;
		break;
	case BCT24157_CHARGE_CURRENT_1500MA:
		*cur = 1500000;
		break;
	default:
		*cur = 500000;
	}
	return 0;
}

static int bct24157_charger_get_health(struct bct24157_charger_info *info, u32 *health)
{
	*health = POWER_SUPPLY_HEALTH_GOOD;

	return 0;
}

static int bct24157_charger_get_status(struct bct24157_charger_info *info)
{
	if (info->charging == true)
		return POWER_SUPPLY_STATUS_CHARGING;
	else
		return POWER_SUPPLY_STATUS_NOT_CHARGING;
}

static int bct24157_charger_set_status(struct bct24157_charger_info *info,
				       int val)
{
	int ret = 0;

	pr_info("%s:val=%d, info->charging=%d\n", __func__, val, info->charging);
	if (!val && info->charging) {
		bct24157_charger_stop_charge(info);
		info->charging = false;
	} else if (val && !info->charging) {
		ret = bct24157_charger_start_charge(info);
		if (ret)
			dev_err(info->dev, "start charge failed\n");
		else
			info->charging = true;
	}
	bct24157_charger_dump_register(info);

	return ret;
}

static int bct24157_charger_usb_get_property(struct power_supply *psy,
					     enum power_supply_property psp,
					     union power_supply_propval *val)
{
	struct bct24157_charger_info *info = power_supply_get_drvdata(psy);
	u32 cur, health, vol;
	enum usb_charger_type type;
	int ret = 0;

	pr_info("%s:enter, psp=%d\n", __func__, psp);

	mutex_lock(&info->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = bct24157_charger_get_status(info);
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		if (!info->charging) {
			val->intval = 0;
		} else {
			ret = bct24157_charger_get_current(info, &cur);
			if (ret)
				goto out;

			val->intval = cur;
		}
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		ret = bct24157_charger_get_termina_vol(info, &vol);
		if (ret)
			goto out;

		val->intval = vol;
		break;

	case POWER_SUPPLY_PROP_CALIBRATE:
		val->intval = bct24157_charger_is_enabled(info);
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		if (!info->charging) {
			val->intval = 0;
		} else {
			ret = 0;
			val->intval = 1500000;
		}
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		if (info->charging) {
			val->intval = 0;
		} else {
			ret = bct24157_charger_get_health(info, &health);
			if (ret)
				goto out;

			val->intval = health;
		}
		break;

	case POWER_SUPPLY_PROP_USB_TYPE:
		type = info->usb_phy->chg_type;

		switch (type) {
		case SDP_TYPE:
			val->intval = POWER_SUPPLY_USB_TYPE_SDP;
			break;

		case DCP_TYPE:
			val->intval = POWER_SUPPLY_USB_TYPE_DCP;
			break;

		case CDP_TYPE:
			val->intval = POWER_SUPPLY_USB_TYPE_CDP;
			break;

		default:
			val->intval = POWER_SUPPLY_USB_TYPE_UNKNOWN;
		}

		break;

	default:
		ret = -EINVAL;
	}

out:
	mutex_unlock(&info->lock);
	return ret;
}

static int bct24157_charger_usb_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct bct24157_charger_info *info = power_supply_get_drvdata(psy);
	int ret;

	pr_info("%s:enter, psp=%d\n", __func__, psp);

	mutex_lock(&info->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = bct24157_charger_set_current(info, val->intval);
		if (ret < 0)
			dev_err(info->dev, "set charge current failed\n");
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = 0;
		break;

	case POWER_SUPPLY_PROP_STATUS:
		ret = bct24157_charger_set_status(info, val->intval);
		if (ret < 0)
			dev_err(info->dev, "set charge status failed\n");
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		ret = bct24157_charger_set_termina_vol(info, val->intval / 1000);
		if (ret < 0)
			dev_err(info->dev, "failed to set terminate voltage\n");
		break;

	case POWER_SUPPLY_PROP_CALIBRATE:
		ret = 0;
		if (val->intval == true) {
			ret = bct24157_charger_start_charge(info);
			if (ret)
				dev_err(info->dev, "failed to start charge\n");
		} else if (val->intval == false)
			bct24157_charger_stop_charge(info);

		dev_err(info->dev, "POWER_SUPPLY_PROP_CHARGING_ENABLED: %s\n",
			 val->intval ? "enable" : "disable");
		break;

	default:
		ret = -EINVAL;
	}

	mutex_unlock(&info->lock);
	return ret;
}

static int bct24157_charger_property_is_writeable(struct power_supply *psy,
						enum power_supply_property psp)
{
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
	case POWER_SUPPLY_PROP_CALIBRATE:
	case POWER_SUPPLY_PROP_STATUS:
		ret = 1;
		break;

	default:
		ret = 0;
	}

	return ret;
}

static enum power_supply_usb_type bct24157_charger_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,
	POWER_SUPPLY_USB_TYPE_C,
	POWER_SUPPLY_USB_TYPE_PD,
	POWER_SUPPLY_USB_TYPE_PD_DRP,
	POWER_SUPPLY_USB_TYPE_APPLE_BRICK_ID
};

static enum power_supply_property bct24157_usb_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_USB_TYPE,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CALIBRATE,
};

static const struct power_supply_desc bct24157_charger_desc = {
	.name			= "bct24157_charger",
	.type			= POWER_SUPPLY_TYPE_UNKNOWN,
	.properties		= bct24157_usb_props,
	.num_properties		= ARRAY_SIZE(bct24157_usb_props),
	.get_property		= bct24157_charger_usb_get_property,
	.set_property		= bct24157_charger_usb_set_property,
	.property_is_writeable	= bct24157_charger_property_is_writeable,
	.usb_types		= bct24157_charger_usb_types,
	.num_usb_types		= ARRAY_SIZE(bct24157_charger_usb_types),
};

#ifdef CONFIG_REGULATOR
static int bct24157_charger_enable_otg(struct regulator_dev *dev)
{
	struct bct24157_charger_info *info = rdev_get_drvdata(dev);
	int ret;

	pr_info("%s:enter\n", __func__);
	/*
	 * Disable charger detection function in case
	 * affecting the OTG timing sequence.
	 */
	ret = regmap_update_bits(info->pmic, info->charger_detect,
				 BIT_DP_DM_BC_ENB, BIT_DP_DM_BC_ENB);
	if (ret) {
		dev_err(info->dev, "failed to disable bc1.2 detect function.\n");
		return ret;
	}

	ret = bct24157_update_bits(info, BCT24157_REG_0, BCT24157_OTG_EN_MASK, BCT24157_OTG_EN);
	if (ret) {
		dev_err(info->dev, "enable bct24157 otg failed\n");
		regmap_update_bits(info->pmic, info->charger_detect,
				   BIT_DP_DM_BC_ENB, 0);
		return ret;
	}

	info->otg_enable = true;

	return 0;
}

static int bct24157_charger_disable_otg(struct regulator_dev *dev)
{
	struct bct24157_charger_info *info = rdev_get_drvdata(dev);
	int ret;
	u8 reg_val=0xF;	//Isaac add for test

	pr_info("%s:enter, BCT24157_OTG_EN_MASK=0x%lx, BCT24157_OTG_EN=0x%lx\n", __func__, BCT24157_OTG_EN_MASK, BCT24157_OTG_EN);
	info->otg_enable = false;
	ret = bct24157_update_bits(info, BCT24157_REG_0, BCT24157_OTG_EN_MASK, 0 << 7);
	if (ret) {
		pr_info("disable bct24157 otg failed, but we ignore it!!!!\n");
		ret = bct24157_read(info, BCT24157_REG_0, &reg_val);
		pr_info("%s:reg_val=0x%x\n", __func__, reg_val);
	}

	/* Enable charger detection function to identify the charger type */
	return regmap_update_bits(info->pmic, info->charger_detect,
				  BIT_DP_DM_BC_ENB, 0);
}

static int bct24157_charger_vbus_is_enabled(struct regulator_dev *dev)
{
	struct bct24157_charger_info *info = rdev_get_drvdata(dev);
	int ret;
	u8 val;

	pr_info("%s:enter\n", __func__);
	/* ret = bct24157_read(info, BCT24157_REG_1, &val);
	if (ret) {
		pr_info("Isaac read bct24157 BCT24157_REG_1 failed, but we ignore it!!!!\n");
		return 0;
	} */

	ret = bct24157_read(info, BCT24157_REG_0, &val);
	if (ret) {
		pr_info("read bct24157 BCT24157_REG_0 failed, but we ignore it!!!!\n");
		return 0;
	}

	val &= BCT24157_OTG_EN_MASK;

	return val;
}

static const struct regulator_ops bct24157_charger_vbus_ops = {
	.enable = bct24157_charger_enable_otg,
	.disable = bct24157_charger_disable_otg,
	.is_enabled = bct24157_charger_vbus_is_enabled,
};

static const struct regulator_desc bct24157_charger_vbus_desc = {
	.name = "otg-vbus",
	.of_match = "otg-vbus",
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
	.ops = &bct24157_charger_vbus_ops,
	.fixed_uV = 5000000,
	.n_voltages = 1,
};

static int bct24157_charger_register_vbus_regulator(struct bct24157_charger_info *info)
{
	struct regulator_config cfg = { };
	struct regulator_dev *reg;
	int ret = 0;

	cfg.dev = info->dev;
	cfg.driver_data = info;
	reg = devm_regulator_register(info->dev,
				      &bct24157_charger_vbus_desc, &cfg);
	if (IS_ERR(reg)) {
		ret = PTR_ERR(reg);
		dev_err(info->dev, "Can't register regulator:%d\n", ret);
	}
	
	pr_info("%s:finished\n", __func__);
	return ret;
}

#else
static int
bct24157_charger_register_vbus_regulator(struct bct24157_charger_info *info)
{
	return 0;
}
#endif
//QinJinke
static int BCT24157_CheckDeviceID(struct i2c_client *client, u8 reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	pr_info("%s:%d\n", __func__, ret);
	if ((ret == 163) || (ret == 147))//0xA3/0x93-->147
		return ret;

	return 0;
}
static int bct24157_i2c_test(struct i2c_client *client)
{
	int i;
	int err = -1;
	for (i = 0;i < 2; i++){
		err = BCT24157_CheckDeviceID(client,BCT24157_REG_4);
		if (!err)
			break;
	}
	return err;
}
static int bct24157_charger_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct device *dev = &client->dev;
	struct power_supply_config charger_cfg = { };
	struct bct24157_charger_info *info;
	struct device_node *regmap_np;
	struct platform_device *regmap_pdev;
	int ret;

	pr_info("%s:enter\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev, "No support for SMBUS_BYTE_DATA\n");
		return -ENODEV;
	}

	if(!bct24157_i2c_test(client)) {
		dev_err(dev, "i2c communication fail!\n");
		return -ENODEV;
	}

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	pr_info("%s:before client!!!\n", __func__);
	info->client = client;
	info->dev = dev;
	pr_info("%s:after client!!!\n", __func__);

	alarm_init(&info->otg_timer, ALARM_BOOTTIME, NULL);

	mutex_init(&info->lock);

	i2c_set_clientdata(client, info);

	info->usb_phy = devm_usb_get_phy_by_phandle(dev, "phys", 0);
	if (IS_ERR(info->usb_phy)) {
		dev_err(dev, "failed to find USB phy\n");
		return PTR_ERR(info->usb_phy);
	}

	info->edev = extcon_get_edev_by_phandle(info->dev, 0);
	if (IS_ERR(info->edev)) {
		dev_err(dev, "failed to find vbus extcon device.\n");
		return PTR_ERR(info->edev);
	}

	ret = bct24157_charger_register_vbus_regulator(info);
	if (ret) {
		dev_err(dev, "failed to register vbus regulator.\n");
		return ret;
	}

	regmap_np = of_find_compatible_node(NULL, NULL, "sprd,sc27xx-syscon");
	if (!regmap_np) {
		dev_err(dev, "unable to get syscon node\n");
		return -ENODEV;
	}

	ret = of_property_read_u32_index(regmap_np, "reg", 1, &info->charger_detect);	//reg = <0xc00>, <0xecc>, <0xec0>;
	if (ret) {
		dev_err(dev, "failed to get charger_detect\n");
		return -EINVAL;
	}

	ret = of_property_read_u32_index(regmap_np, "reg", 2, &info->charger_pd);		//reg = <0xc00>, <0xecc>, <0xec0>;
	if (ret) {
		dev_err(dev, "failed to get charger_pd reg\n");
		return ret;
	}

	if (of_device_is_compatible(regmap_np->parent, "sprd,sc2730"))
		info->charger_pd_mask = BCT24157_DISABLE_PIN_MASK_2730;
	else if (of_device_is_compatible(regmap_np->parent, "sprd,sc2721"))
		info->charger_pd_mask = BCT24157_DISABLE_PIN_MASK_2721;
	else if (of_device_is_compatible(regmap_np->parent, "sprd,sc2720"))
		info->charger_pd_mask = BCT24157_DISABLE_PIN_MASK_2720;
	else {
		dev_err(dev, "failed to get charger_pd mask\n");
		return -EINVAL;
	}

	regmap_pdev = of_find_device_by_node(regmap_np);
	if (!regmap_pdev) {
		of_node_put(regmap_np);
		dev_err(dev, "unable to get syscon device\n");
		return -ENODEV;
	}

	of_node_put(regmap_np);
	info->pmic = dev_get_regmap(regmap_pdev->dev.parent, NULL);
	if (!info->pmic) {
		dev_err(dev, "unable to get pmic regmap device\n");
		return -ENODEV;
	}

	charger_cfg.drv_data = info;
	charger_cfg.of_node = dev->of_node;
	info->psy_usb = devm_power_supply_register(dev,
						   &bct24157_charger_desc,
						   &charger_cfg);
	if (IS_ERR(info->psy_usb)) {
		dev_err(dev, "failed to register power supply\n");
		return PTR_ERR(info->psy_usb);
	}

	ret = bct24157_charger_hw_init(info);
	if (ret)
		return ret;

	bct24157_charger_stop_charge(info);

	return 0;
}

static int bct24157_charger_remove(struct i2c_client *client)
{
	struct bct24157_charger_info *info = i2c_get_clientdata(client);

	usb_unregister_notifier(info->usb_phy, &info->usb_notify);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int bct24157_charger_suspend(struct device *dev)
{
	struct bct24157_charger_info *info = dev_get_drvdata(dev);
	ktime_t now, add;
	unsigned int wakeup_ms = BCT24157_OTG_ALARM_TIMER_MS;

	if (!info->otg_enable)
		return 0;

	pr_info("%s:enter\n", __func__);

	now = ktime_get_boottime();
	add = ktime_set(wakeup_ms / MSEC_PER_SEC,
			(wakeup_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
	alarm_start(&info->otg_timer, ktime_add(now, add));

	return 0;
}

static int bct24157_charger_resume(struct device *dev)
{
	struct bct24157_charger_info *info = dev_get_drvdata(dev);

	if (!info->otg_enable)
		return 0;

	alarm_cancel(&info->otg_timer);

	return 0;
}
#endif

static const struct dev_pm_ops bct24157_charger_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(bct24157_charger_suspend,
				bct24157_charger_resume)
};

static const struct i2c_device_id bct24157_i2c_id[] = {
	{"bct24157_chg", 0},
	{}
};

static const struct of_device_id bct24157_charger_of_match[] = {
	{ .compatible = "boardchip,bct24157_chg", },
	{ .compatible = "prisemi,psc5415z_chg", },
	{ }
};

MODULE_DEVICE_TABLE(of, bct24157_charger_of_match);

static struct i2c_driver bct24157_charger_driver = {
	.driver = {
		.name = "bct24157_chg",
		.of_match_table = bct24157_charger_of_match,
		.pm = &bct24157_charger_pm_ops,
	},
	.probe = bct24157_charger_probe,
	.remove = bct24157_charger_remove,
	.id_table = bct24157_i2c_id,
};

module_i2c_driver(bct24157_charger_driver);
MODULE_DESCRIPTION("BCT24157 Charger Driver");
MODULE_LICENSE("GPL v2");
