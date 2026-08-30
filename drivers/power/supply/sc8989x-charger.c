/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (c) 2022 Southchip Semiconductor Technology(Shanghai) Co., Ltd.
*/
#include <linux/acpi.h>
#include <linux/alarmtimer.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/power/charger-manager.h>
#include <linux/power/sprd_battery_info.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/types.h>


#define SC8989X_DRV_VERSION             "1.0.0_SC"

#define SC8989X_REG_00                  0x00
#define REG00_EN_HIZ_MASK               BIT(7)
#define REG00_EN_HIZ_SHIFT              7
#define REG00_EN_HIZ                    1
#define REG00_EXIT_HIZ                  0
#define REG00_EN_ILIM_MASK              BIT(6)
#define REG00_EN_ILIM_SHIFT             6
#define REG00_EN_ILIM_DISABLE           0
#define REG00_IINDPM_MASK               GENMASK(5, 0)
#define REG00_IINDPM_SHIFT              0
#define REG00_IINDPM_BASE               100
#define REG00_IINDPM_LSB                50
#define REG00_IINDPM_MIN                100
#define REG00_IINDPM_MAX                3250

#define SC8989X_REG_01                  0x01

#define SC8989X_REG_02                  0x02

#define SC8989X_REG_03                  0x03
#define REG03_WDT_RST_MASK              BIT(6)
#define REG03_WDT_RST_SHIFT             6
#define REG03_WDT_RESET                 1
#define REG03_OTG_MASK                  BIT(5)
#define REG03_OTG_SHIFT                 5
#define REG03_OTG_ENABLE                1
#define REG03_OTG_DISABLE               0
#define REG03_CHG_MASK                  BIT(4)
#define REG03_CHG_SHIFT                 4
#define REG03_CHG_ENABLE                1
#define REG03_CHG_DISABLE               0


#define SC8989X_REG_04                  0x04
#define REG04_ICC_MASK                  GENMASK(6, 0)
#define REG04_ICC_SHIFT                 0
#define REG04_ICC_BASE                  0
#define REG04_ICC_LSB                   60
#define REG04_ICC_MIN                   0
#define REG04_ICC_MAX                   5040

#define SC8989X_REG_05                  0x05
#define REG05_ITC_MASK                  GENMASK(7, 4)
#define REG05_ITC_SHIFT                 4
#define REG05_ITC_BASE                  60
#define REG05_ITC_LSB                   60
#define REG05_ITC_MIN                   60
#define REG05_ITC_MAX                   960
#define REG05_ITERM_MASK                GENMASK(3, 0)
#define REG05_ITERM_SHIFT               0
#define REG05_ITERM_BASE                30
#define REG05_ITERM_LSB                 60
#define REG05_ITERM_MIN                 30
#define REG05_ITERM_MAX                 960

#define SC8989X_REG_06                  0x06
#define REG06_VREG_MASK                 GENMASK(7, 2)
#define REG06_VREG_SHIFT                2
#define REG06_VREG_BASE                 3840
#define REG06_VREG_LSB                  16
#define REG06_VREG_MIN                  3840
#define REG06_VREG_MAX                  4856
#define REG06_VBAT_LOW_MASK             BIT(1)
#define REG06_VBAT_LOW_SHIFT            1
#define REG06_VBAT_LOW_2P8V             0
#define REG06_VBAT_LOW_3P0V             1
#define REG06_VRECHG_MASK               BIT(0)
#define REG06_VRECHG_SHIFT              0
#define REG06_VRECHG_100MV              0
#define REG06_VRECHG_200MV              1

#define SC8989X_REG_07                  0x07
#define REG07_TWD_MASK                  GENMASK(5, 4)
#define REG07_TWD_SHIFT                 4
#define REG07_TWD_DISABLE               0
#define REG07_TWD_40S                   1
#define REG07_TWD_80S                   2
#define REG07_TWD_160S                  3
#define REG07_EN_TIMER_MASK             BIT(3)
#define REG07_EN_TIMER_SHIFT            3
#define REG07_CHG_TIMER_ENABLE          1
#define REG07_CHG_TIMER_DISABLE         0


#define SC8989X_REG_08                  0x08

#define SC8989X_REG_09                  0x09
#define REG09_BATFET_DIS_MASK           BIT(5)
#define REG09_BATFET_DIS_SHIFT          5
#define REG09_BATFET_ENABLE             0
#define REG09_BATFET_DISABLE            1

#define SC8989X_REG_0A                  0x0A
#define REG0A_BOOSTV_MASK               GENMASK(7, 4)
#define REG0A_BOOSTV_SHIFT              4
#define REG0A_BOOSTV_LIM_MASK           GENMASK(2, 0)
#define REG0A_BOOSTV_LIM_SHIFT          0
#define REG0A_BOOSTV_LIM_1P2A           0x02

#define SC8989X_REG_0B                  0x0B

#define SC8989X_REG_0C                  0x0C
#define REG0C_OTG_FAULT                 BIT(6)

#define SC8989X_REG_0D                  0x0D
#define REG0D_FORCE_VINDPM_MASK         BIT(7)
#define REG0D_FORCE_VINDPM_SHIFT        7
#define REG0D_FORCE_VINDPM_ENABLE       1

#define REG0D_VINDPM_MASK               GENMASK(6, 0)
#define REG0D_VINDPM_BASE               2600
#define REG0D_VINDPM_LSB                100
#define REG0D_VINDPM_MIN                3900
#define REG0D_VINDPM_MAX                15300

#define SC8989X_REG_0E                  0x0E
#define SC8989X_REG_0F                  0x0F
#define SC8989X_REG_10                  0x10
#define SC8989X_REG_11                  0x11
#define SC8989X_REG_12                  0x12
#define SC8989X_REG_13                  0x13

#define SC8989X_REG_14                  0x14
#define REG14_REG_RESET_MASK            BIT(7)
#define REG14_REG_RESET_SHIFT           7
#define REG14_REG_RESET                 1
#define REG14_VENDOR_ID_MASK            GENMASK(5, 3)
#define REG14_VENDOR_ID_SHIFT           3
#define SC8989X_VENDOR_ID               4

#define SC8989X_REG_NUM                 21

/* Other Realted Definition*/
#define SC8989X_BATTERY_NAME            "sc27xx-fgu"

#define BIT_DP_DM_BC_ENB                BIT(0)

#define SC8989X_WDT_VALID_MS            50

#define SC8989X_WDG_TIMER_MS            15000
#define SC8989X_OTG_VALID_MS            500
#define SC8989X_OTG_RETRY_TIMES         10

#define SC8989X_DISABLE_PIN_MASK        BIT(0)
#define SC8989X_DISABLE_PIN_MASK_2721   BIT(15)

#define SC8989X_FAST_CHG_VOL_MAX        10500000
#define SC8989X_NORMAL_CHG_VOL_MAX      6500000

#define SC8989X_WAKE_UP_MS              2000

struct sc8989x_charger_sysfs {
    char *name;
    struct attribute_group attr_g;
    struct device_attribute attr_sc8989x_dump_reg;
    struct device_attribute attr_sc8989x_lookup_reg;
    struct device_attribute attr_sc8989x_sel_reg_id;
    struct device_attribute attr_sc8989x_reg_val;
    struct attribute *attrs[5];

    struct sc8989x_charger_info *info;
};

struct sc8989x_charge_current {
    int sdp_limit;
    int sdp_cur;
    int dcp_limit;
    int dcp_cur;
    int cdp_limit;
    int cdp_cur;
    int unknown_limit;
    int unknown_cur;
    int fchg_limit;
    int fchg_cur;
};

struct sc8989x_charger_info {
    struct i2c_client *client;
    struct device *dev;
    struct power_supply *psy_usb;
    struct sc8989x_charge_current cur;
    struct mutex lock;
    struct mutex input_limit_cur_lock;
    bool charging;
    bool is_charger_online;
    struct delayed_work otg_work;
    struct delayed_work wdt_work;
    struct delayed_work dump_work;
    struct regmap *pmic;
    u32 charger_detect;
    u32 charger_pd;
    u32 charger_pd_mask;
    struct gpio_desc *gpiod;
    u32 last_limit_current;
    u32 role;
    bool need_disable_Q1;
    int termination_cur;
    int vol_max_mv;
    u32 actual_limit_current;
    bool otg_enable;
    struct alarm wdg_timer;
    struct sc8989x_charger_sysfs *sysfs;
    int reg_id;
};

struct sc8989x_charger_reg_tab {
    int id;
    u32 addr;
    char *name;
};

static struct sc8989x_charger_reg_tab reg_tab[SC8989X_REG_NUM + 1] = {
    {0, SC8989X_REG_00, "Setting Input Limit Current reg"},
    {1, SC8989X_REG_01, "Setting Vindpm_OS reg"},
    {2, SC8989X_REG_02, "Related Function Enable reg"},
    {3, SC8989X_REG_03, "Related Function Config reg"},
    {4, SC8989X_REG_04, "Setting Charge Limit Current reg"},
    {5, SC8989X_REG_05, "Setting Terminal Current reg"},
    {6, SC8989X_REG_06, "Setting Charge Limit Voltage reg"},
    {7, SC8989X_REG_07, "Related Function Config reg"},
    {8, SC8989X_REG_08, "IR Compensation Resistor Setting reg"},
    {9, SC8989X_REG_09, "Related Function Config reg"},
    {10, SC8989X_REG_0A, "Boost Mode Related Setting reg"},
    {11, SC8989X_REG_0B, "Status reg"},
    {12, SC8989X_REG_0C, "Fault reg"},
    {13, SC8989X_REG_0D, "Setting Vindpm reg"},
    {14, SC8989X_REG_0E, "ADC Conversion of Battery Voltage reg"},
    {15, SC8989X_REG_0F, "ADDC Conversion of System Voltage reg"},
    {16, SC8989X_REG_10, "ADC Conversion of TS Voltage as Percentage of REGN reg"},
    {17, SC8989X_REG_11, "ADC Conversion of VBUS voltage reg"},
    {18, SC8989X_REG_12, "ICHGR Setting reg"},
    {19, SC8989X_REG_13, "IDPM Limit Setting reg"},
    {20, SC8989X_REG_14, "Related Function Config reg"},
    {21, 0, "null"},
};

static int sc8989x_charger_set_limit_current(struct sc8989x_charger_info *info,
        u32 limit_cur, bool enable);

static int sc8989x_read(struct sc8989x_charger_info *info, u8 reg, u8 *data)
{
    int ret;

    ret = i2c_smbus_read_byte_data(info->client, reg);
    if (ret < 0)
        return ret;

    *data = ret;
    return 0;
}

static int sc8989x_write(struct sc8989x_charger_info *info, u8 reg, u8 data)
{
    return i2c_smbus_write_byte_data(info->client, reg, data);
}

static int sc8989x_update_bits(struct sc8989x_charger_info *info, u8 reg,
                               u8 mask, u8 data)
{
    u8 v;
    int ret;

    ret = sc8989x_read(info, reg, &v);
    if (ret < 0)
        return ret;

    v &= ~mask;
    v |= (data & mask);

    return sc8989x_write(info, reg, v);
}

static void sc8989x_charger_dump_register(struct sc8989x_charger_info *info)
{
    int i, ret, len, idx = 0;
    u8 reg_val;
    char buf[512];

    memset(buf, '\0', sizeof(buf));
    for (i = 0; i < SC8989X_REG_NUM; i++) {
        ret = sc8989x_read(info, reg_tab[i].addr, &reg_val);
        if (ret == 0) {
            len = snprintf(buf + idx, sizeof(buf) - idx,
                           "[REG_0x%.2x]=0x%.2x; ", reg_tab[i].addr,
                           reg_val);
            idx += len;
        }
    }

    dev_info(info->dev, "%s: %s", __func__, buf);
}

static bool sc8989x_charger_is_bat_present(struct sc8989x_charger_info *info)
{
    struct power_supply *psy;
    union power_supply_propval val;
    bool present = false;
    int ret;

    psy = power_supply_get_by_name(SC8989X_BATTERY_NAME);
    if (!psy) {
        dev_err(info->dev, "Failed to get psy of sc27xx_fgu\n");
        return present;
    }
    ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_PRESENT,
                                    &val);
    if (!ret && val.intval)
        present = true;
    power_supply_put(psy);

    if (ret)
        dev_err(info->dev, "Failed to get property of present:%d\n", ret);

    return present;
}

static int sc8989x_charger_is_fgu_present(struct sc8989x_charger_info *info)
{
    struct power_supply *psy;

    psy = power_supply_get_by_name(SC8989X_BATTERY_NAME);
    if (!psy) {
        dev_err(info->dev, "Failed to find psy of sc27xx_fgu\n");
        return -ENODEV;
    }
    power_supply_put(psy);

    return 0;
}

static int sc8989x_charger_set_vindpm(struct sc8989x_charger_info *info, u32 vol)
{
    u8 reg_val;
    int ret;

    ret = sc8989x_update_bits(info, SC8989X_REG_0D, REG0D_FORCE_VINDPM_MASK,
                              REG0D_FORCE_VINDPM_ENABLE << REG0D_FORCE_VINDPM_SHIFT);
    if (ret) {
        dev_err(info->dev, "set force vindpm failed\n");
        return ret;
    }

    if (vol < REG0D_VINDPM_MIN)
        vol = REG0D_VINDPM_MIN;
    else if (vol > REG0D_VINDPM_MAX)
        vol = REG0D_VINDPM_MAX;
    reg_val = (vol - REG0D_VINDPM_BASE) / REG0D_VINDPM_LSB;

    return sc8989x_update_bits(info, SC8989X_REG_0D,
                               REG0D_FORCE_VINDPM_MASK, reg_val);
}

static int sc8989x_charger_set_termina_vol(struct sc8989x_charger_info *info, u32 vol)
{
    u8 reg_val;

    if (vol < REG06_VREG_MIN)
        vol = REG06_VREG_MIN;
    else if (vol >= REG06_VREG_MAX)
        vol = REG06_VREG_MAX;
    reg_val = (vol - REG06_VREG_BASE) / REG06_VREG_LSB;

    return sc8989x_update_bits(info, SC8989X_REG_06, REG06_VREG_MASK,
                               reg_val << REG06_VREG_SHIFT);
}

static int sc8989x_charger_set_termina_cur(struct sc8989x_charger_info *info, u32 cur)
{
    u8 reg_val;

    if (cur <= REG05_ITERM_MIN)
        cur = REG05_ITERM_MIN;
    else if (cur >= REG05_ITERM_MAX)
        cur = REG05_ITERM_MAX;
    reg_val = (cur - REG05_ITERM_BASE) / REG05_ITERM_LSB;

    return sc8989x_update_bits(info, SC8989X_REG_05, REG05_ITERM_MASK, reg_val);
}

static int sc8989x_charger_hw_init(struct sc8989x_charger_info *info)
{
    struct sprd_battery_info bat_info = {};
    int voltage_max_microvolt, termination_cur;
    int ret;

    ret = sprd_battery_get_battery_info(info->psy_usb, &bat_info);
    if (ret) {
        dev_warn(info->dev, "no battery information is supplied\n");

        info->cur.sdp_limit = 500000;
        info->cur.sdp_cur = 500000;
        info->cur.dcp_limit = 1500000;
        info->cur.dcp_cur = 1500000;
        info->cur.cdp_limit = 1000000;
        info->cur.cdp_cur = 1000000;
        info->cur.unknown_limit = 500000;
        info->cur.unknown_cur = 500000;

        /*
         * If no battery information is supplied, we should set
         * default charge termination current to 120 mA, and default
         * charge termination voltage to 4.44V.
         */
        voltage_max_microvolt = 4440;
        termination_cur = 120;
        info->termination_cur = termination_cur;
    } else {
        info->cur.sdp_limit = bat_info.cur.sdp_limit;
        info->cur.sdp_cur = bat_info.cur.sdp_cur;
        info->cur.dcp_limit = bat_info.cur.dcp_limit;
        info->cur.dcp_cur = bat_info.cur.dcp_cur;
        info->cur.cdp_limit = bat_info.cur.cdp_limit;
        info->cur.cdp_cur = bat_info.cur.cdp_cur;
        info->cur.unknown_limit = bat_info.cur.unknown_limit;
        info->cur.unknown_cur = bat_info.cur.unknown_cur;
        info->cur.fchg_limit = bat_info.cur.fchg_limit;
        info->cur.fchg_cur = bat_info.cur.fchg_cur;

        voltage_max_microvolt = bat_info.constant_charge_voltage_max_uv / 1000;
        termination_cur = bat_info.charge_term_current_ua / 1000;
        info->termination_cur = termination_cur;
        sprd_battery_put_battery_info(info->psy_usb, &bat_info);
    }

    ret = sc8989x_update_bits(info, SC8989X_REG_14, REG14_REG_RESET_MASK,
                              REG14_REG_RESET << REG14_REG_RESET_SHIFT);
    if (ret) {
        dev_err(info->dev, "reset sc8989x failed\n");
        return ret;
    }

    ret = sc8989x_charger_set_vindpm(info, info->vol_max_mv);
    if (ret) {
        dev_err(info->dev, "set sc8989x vindpm vol failed\n");
        return ret;
    }

    ret = sc8989x_charger_set_termina_vol(info, info->vol_max_mv);
    if (ret) {
        dev_err(info->dev, "set sc8989x terminal vol failed\n");
        return ret;
    }

    ret = sc8989x_charger_set_termina_cur(info, info->termination_cur);
    if (ret) {
        dev_err(info->dev, "set sc8989x terminal cur failed\n");
        return ret;
    }

    ret = sc8989x_charger_set_limit_current(info, info->cur.unknown_cur, false);
    if (ret)
        dev_err(info->dev, "set sc8989x limit current failed\n");

    ret = sc8989x_update_bits(info, SC8989X_REG_0A, REG0A_BOOSTV_LIM_MASK,
                              REG0A_BOOSTV_LIM_1P2A);
    if (ret) {
        dev_err(info->dev, "set sc8989x boost limit current failed\n");
        return ret;
    }

    return ret;
}

static int sc8989x_charger_get_charge_voltage(struct sc8989x_charger_info *info,
        u32 *charge_vol)
{
    struct power_supply *psy;
    union power_supply_propval val;
    int ret;

    psy = power_supply_get_by_name(SC8989X_BATTERY_NAME);
    if (!psy) {
        dev_err(info->dev, "failed to get SC8989X_BATTERY_NAME\n");
        return -ENODEV;
    }

    ret = power_supply_get_property(psy,
                                    POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
                                    &val);
    power_supply_put(psy);
    if (ret) {
        dev_err(info->dev, "failed to get CONSTANT_CHARGE_VOLTAGE\n");
        return ret;
    }

    *charge_vol = val.intval;

    return 0;
}

static int sc8989x_charger_start_charge(struct sc8989x_charger_info *info)
{
    int ret;

    ret = sc8989x_update_bits(info, SC8989X_REG_00,
                              REG00_EN_HIZ_MASK, REG00_EXIT_HIZ);
    if (ret)
        dev_err(info->dev, "disable HIZ mode failed\n");

    ret = sc8989x_update_bits(info, SC8989X_REG_07, REG07_TWD_MASK,
                              REG07_TWD_40S << REG07_TWD_SHIFT);
    if (ret) {
        dev_err(info->dev, "Failed to enable sc8989x watchdog\n");
        return ret;
    }

    ret = regmap_update_bits(info->pmic, info->charger_pd,
                             info->charger_pd_mask, 0);
    if (ret) {
        dev_err(info->dev, "enable sc8989x charge failed\n");
        return ret;
    }

    ret = sc8989x_charger_set_limit_current(info,
                                            info->last_limit_current, false);
    if (ret) {
        dev_err(info->dev, "failed to set limit current\n");
        return ret;
    }

    ret = sc8989x_charger_set_termina_cur(info, info->termination_cur);
    if (ret)
        dev_err(info->dev, "set sc8989x terminal cur failed\n");

    schedule_delayed_work(&info->dump_work, HZ*15);
    return ret;
}

static void sc8989x_charger_stop_charge(struct sc8989x_charger_info *info, bool present)
{
    int ret;

    if (!present || info->need_disable_Q1) {
        ret = sc8989x_update_bits(info, SC8989X_REG_00, REG00_EN_HIZ_MASK,
                                  REG00_EN_HIZ << REG00_EN_HIZ_SHIFT);
        if (ret)
            dev_err(info->dev, "enable HIZ mode failed\n");
        info->need_disable_Q1 = false;
    }

    ret = regmap_update_bits(info->pmic, info->charger_pd,
                             info->charger_pd_mask,
                             info->charger_pd_mask);
    if (ret)
        dev_err(info->dev, "disable sc8989x charge failed\n");

    ret = sc8989x_update_bits(info, SC8989X_REG_07, REG07_TWD_MASK,
                              REG07_TWD_DISABLE);
    if (ret)
        dev_err(info->dev, "Failed to disable sc8989x watchdog\n");

    cancel_delayed_work_sync(&info->dump_work);
}

static int sc8989x_charger_set_current(struct sc8989x_charger_info *info,
                                       u32 cur)
{
    u8 reg_val;
    int ret;

    cur = cur / 1000;
    if (cur <= REG04_ICC_MIN) {
        cur = REG04_ICC_MIN;
    } else if (cur >= REG04_ICC_MAX) {
        cur = REG04_ICC_MAX;
    }

    reg_val = cur / REG04_ICC_LSB;

    ret = sc8989x_update_bits(info, SC8989X_REG_04, REG04_ICC_MASK, reg_val);
    dev_err(info->dev, "current = %d, reg_val = %x\n", cur, reg_val);
    return ret;
}

static int sc8989x_charger_get_current(struct sc8989x_charger_info *info,
                                       u32 *cur)
{
    u8 reg_val;
    int ret;

    ret = sc8989x_read(info, SC8989X_REG_04, &reg_val);
    if (ret < 0)
        return ret;

    reg_val &= REG04_ICC_MASK;
    *cur = reg_val * REG04_ICC_LSB * 1000;

    return 0;
}

static u32 sc8989x_charger_get_limit_current(struct sc8989x_charger_info *info,
        u32 *limit_cur)
{
    u8 reg_val;
    int ret;

    ret = sc8989x_read(info, SC8989X_REG_00, &reg_val);
    if (ret < 0)
        return ret;

    reg_val &= REG00_IINDPM_MASK;
    *limit_cur = reg_val * REG00_IINDPM_LSB + REG00_IINDPM_BASE;
    if (*limit_cur >= REG00_IINDPM_MAX)
        *limit_cur = REG00_IINDPM_MAX * 1000;
    else
        *limit_cur = *limit_cur * 1000;

    return 0;
}

static int sc8989x_charger_set_limit_current(struct sc8989x_charger_info *info,
        u32 limit_cur, bool enable)
{
    u8 reg_val;
    int ret = 0;

    mutex_lock(&info->input_limit_cur_lock);
    if (enable) {
        ret = sc8989x_charger_get_limit_current(info, &limit_cur);
        if (ret) {
            dev_err(info->dev, "get limit cur failed\n");
            goto out;
        }

        if (limit_cur == info->actual_limit_current)
            goto out;
        limit_cur = info->actual_limit_current;
    }

    ret = sc8989x_update_bits(info, SC8989X_REG_00, REG00_EN_ILIM_MASK,
                              REG00_EN_ILIM_DISABLE);
    if (ret) {
        dev_err(info->dev, "disable en_ilim failed\n");
        goto out;
    }

    limit_cur = limit_cur / 1000;
    if (limit_cur >= REG00_IINDPM_MAX)
        limit_cur = REG00_IINDPM_MAX;
    if (limit_cur <= REG00_IINDPM_MIN)
        limit_cur = REG00_IINDPM_MIN;

    info->last_limit_current = limit_cur * 1000;
    reg_val = (limit_cur - REG00_IINDPM_BASE) / REG00_IINDPM_LSB;
    info->actual_limit_current =
        (reg_val * REG00_IINDPM_LSB + REG00_IINDPM_BASE) * 1000;
    ret = sc8989x_update_bits(info, SC8989X_REG_00, REG00_IINDPM_MASK, reg_val);
    if (ret)
        dev_err(info->dev, "set sc8989x limit cur failed\n");

    dev_info(info->dev, "set limit current reg_val = %#x, actual_limit_cur = %d\n",
             reg_val, info->actual_limit_current);

out:
    mutex_unlock(&info->input_limit_cur_lock);

    return ret;
}

static inline int sc8989x_charger_get_health(struct sc8989x_charger_info *info,
        u32 *health)
{
    *health = POWER_SUPPLY_HEALTH_GOOD;

    return 0;
}

static int sc8989x_charger_feed_watchdog(struct sc8989x_charger_info *info)
{
    int ret;

    ret = sc8989x_update_bits(info, SC8989X_REG_03, REG03_WDT_RST_MASK,
                              REG03_WDT_RESET << REG03_WDT_RST_SHIFT);
    if (ret) {
        dev_err(info->dev, "reset sc8989x failed\n");
        return ret;
    }

    if (info->otg_enable)
        return 0;

    ret = sc8989x_charger_set_limit_current(info, 0, true);
    if (ret)
        dev_err(info->dev, "set limit cur failed\n");

    return ret;
}

static inline int sc8989x_charger_get_status(struct sc8989x_charger_info *info)
{
    if (info->charging)
        return POWER_SUPPLY_STATUS_CHARGING;
    else
        return POWER_SUPPLY_STATUS_NOT_CHARGING;
}

static int sc8989x_charger_set_status(struct sc8989x_charger_info *info,
                                      int val, u32 input_vol, bool bat_present)
{
    int ret = 0;

    if (val == CM_FAST_CHARGE_OVP_DISABLE_CMD) {
        if (input_vol > SC8989X_FAST_CHG_VOL_MAX)
            info->need_disable_Q1 = true;
    } else if (val == false) {
        if (input_vol > SC8989X_NORMAL_CHG_VOL_MAX)
            info->need_disable_Q1 = true;
    }

    if (val > CM_FAST_CHARGE_NORMAL_CMD)
        return 0;

    if (!val && info->charging) {
        sc8989x_charger_stop_charge(info, bat_present);
        info->charging = false;
    } else if (val && !info->charging) {
        ret = sc8989x_charger_start_charge(info);
        if (ret)
            dev_err(info->dev, "start charge failed\n");
        else
            info->charging = true;
    }

    return ret;
}

static ssize_t sc8989x_reg_val_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
    struct sc8989x_charger_sysfs *sc8989x_sysfs =
        container_of(attr, struct sc8989x_charger_sysfs,
                     attr_sc8989x_reg_val);
    struct sc8989x_charger_info *info = sc8989x_sysfs->info;
    u8 val;
    int ret;

    if (!info)
        return sprintf(buf, "%s sc8989x_sysfs->info is null\n", __func__);

    ret = sc8989x_read(info, reg_tab[info->reg_id].addr, &val);
    if (ret) {
        dev_err(info->dev, "fail to get sc8989x_REG_0x%.2x value, ret = %d\n",
                reg_tab[info->reg_id].addr, ret);
        return sprintf(buf, "fail to get sc8989x_REG_0x%.2x value\n",
                       reg_tab[info->reg_id].addr);
    }

    return sprintf(buf, "sc8989x_REG_0x%.2x = 0x%.2x\n",
                   reg_tab[info->reg_id].addr, val);
}

static ssize_t sc8989x_reg_val_store(struct device *dev,
                                     struct device_attribute *attr,
                                     const char *buf, size_t count)
{
    struct sc8989x_charger_sysfs *sc8989x_sysfs =
        container_of(attr, struct sc8989x_charger_sysfs,
                     attr_sc8989x_reg_val);
    struct sc8989x_charger_info *info = sc8989x_sysfs->info;
    u8 val;
    int ret;

    if (!info) {
        dev_err(dev, "%s sc8989x_sysfs->info is null\n", __func__);
        return count;
    }

    ret =  kstrtou8(buf, 16, &val);
    if (ret) {
        dev_err(info->dev, "fail to get addr, ret = %d\n", ret);
        return count;
    }

    ret = sc8989x_write(info, reg_tab[info->reg_id].addr, val);
    if (ret) {
        dev_err(info->dev, "fail to wite 0x%.2x to REG_0x%.2x, ret = %d\n",
                val, reg_tab[info->reg_id].addr, ret);
        return count;
    }

    dev_info(info->dev, "wite 0x%.2x to REG_0x%.2x success\n", val,
             reg_tab[info->reg_id].addr);
    return count;
}

static ssize_t sc8989x_reg_id_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count)
{
    struct sc8989x_charger_sysfs *sc8989x_sysfs =
        container_of(attr, struct sc8989x_charger_sysfs,
                     attr_sc8989x_sel_reg_id);
    struct sc8989x_charger_info *info = sc8989x_sysfs->info;
    int ret, id;

    if (!info) {
        dev_err(dev, "%s sc8989x_sysfs->info is null\n", __func__);
        return count;
    }

    ret =  kstrtoint(buf, 10, &id);
    if (ret) {
        dev_err(info->dev, "%s store register id fail\n", sc8989x_sysfs->name);
        return count;
    }

    if (id < 0 || id >= SC8989X_REG_NUM) {
        dev_err(info->dev, "%s store register id fail, id = %d is out of range\n",
                sc8989x_sysfs->name, id);
        return count;
    }

    info->reg_id = id;

    dev_info(info->dev, "%s store register id = %d success\n", sc8989x_sysfs->name, id);
    return count;
}

static ssize_t sc8989x_reg_id_show(struct device *dev,
                                   struct device_attribute *attr,
                                   char *buf)
{
    struct sc8989x_charger_sysfs *sc8989x_sysfs =
        container_of(attr, struct sc8989x_charger_sysfs,
                     attr_sc8989x_sel_reg_id);
    struct sc8989x_charger_info *info = sc8989x_sysfs->info;

    if (!info)
        return sprintf(buf, "%s sc8989x_sysfs->info is null\n", __func__);

    return sprintf(buf, "Cuurent register id = %d\n", info->reg_id);
}

static ssize_t sc8989x_reg_table_show(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
    struct sc8989x_charger_sysfs *sc8989x_sysfs =
        container_of(attr, struct sc8989x_charger_sysfs,
                     attr_sc8989x_lookup_reg);
    struct sc8989x_charger_info *info = sc8989x_sysfs->info;
    int i, len, idx = 0;
    char reg_tab_buf[2000];

    if (!info)
        return sprintf(buf, "%s sc8989x_sysfs->info is null\n", __func__);

    memset(reg_tab_buf, '\0', sizeof(reg_tab_buf));
    len = snprintf(reg_tab_buf + idx, sizeof(reg_tab_buf) - idx,
                   "Format: [id] [addr] [desc]\n");
    idx += len;

    for (i = 0; i < SC8989X_REG_NUM; i++) {
        len = snprintf(reg_tab_buf + idx, sizeof(reg_tab_buf) - idx,
                       "[%d] [REG_0x%.2x] [%s]; \n",
                       reg_tab[i].id, reg_tab[i].addr, reg_tab[i].name);
        idx += len;
    }

    return sprintf(buf, "%s\n", reg_tab_buf);
}

static ssize_t sc8989x_dump_reg_show(struct device *dev,
                                     struct device_attribute *attr,
                                     char *buf)
{
    struct sc8989x_charger_sysfs *sc8989x_sysfs =
        container_of(attr, struct sc8989x_charger_sysfs,
                     attr_sc8989x_dump_reg);
    struct sc8989x_charger_info *info = sc8989x_sysfs->info;

    if (!info)
        return sprintf(buf, "%s sc8989x_sysfs->info is null\n", __func__);

    sc8989x_charger_dump_register(info);

    return sprintf(buf, "%s\n", sc8989x_sysfs->name);
}

static int sc8989x_register_sysfs(struct sc8989x_charger_info *info)
{
    struct sc8989x_charger_sysfs *sc8989x_sysfs;
    int ret;

    sc8989x_sysfs = devm_kzalloc(info->dev, sizeof(*sc8989x_sysfs), GFP_KERNEL);
    if (!sc8989x_sysfs)
        return -ENOMEM;

    info->sysfs = sc8989x_sysfs;
    sc8989x_sysfs->name = "sc8989x_sysfs";
    sc8989x_sysfs->info = info;
    sc8989x_sysfs->attrs[0] = &sc8989x_sysfs->attr_sc8989x_dump_reg.attr;
    sc8989x_sysfs->attrs[1] = &sc8989x_sysfs->attr_sc8989x_lookup_reg.attr;
    sc8989x_sysfs->attrs[2] = &sc8989x_sysfs->attr_sc8989x_sel_reg_id.attr;
    sc8989x_sysfs->attrs[3] = &sc8989x_sysfs->attr_sc8989x_reg_val.attr;
    sc8989x_sysfs->attrs[4] = NULL;
    sc8989x_sysfs->attr_g.name = "debug";
    sc8989x_sysfs->attr_g.attrs = sc8989x_sysfs->attrs;

    sysfs_attr_init(&sc8989x_sysfs->attr_sc8989x_dump_reg.attr);
    sc8989x_sysfs->attr_sc8989x_dump_reg.attr.name = "sc8989x_dump_reg";
    sc8989x_sysfs->attr_sc8989x_dump_reg.attr.mode = 0444;
    sc8989x_sysfs->attr_sc8989x_dump_reg.show = sc8989x_dump_reg_show;

    sysfs_attr_init(&sc8989x_sysfs->attr_sc8989x_lookup_reg.attr);
    sc8989x_sysfs->attr_sc8989x_lookup_reg.attr.name = "sc8989x_lookup_reg";
    sc8989x_sysfs->attr_sc8989x_lookup_reg.attr.mode = 0444;
    sc8989x_sysfs->attr_sc8989x_lookup_reg.show = sc8989x_reg_table_show;

    sysfs_attr_init(&sc8989x_sysfs->attr_sc8989x_sel_reg_id.attr);
    sc8989x_sysfs->attr_sc8989x_sel_reg_id.attr.name = "sc8989x_sel_reg_id";
    sc8989x_sysfs->attr_sc8989x_sel_reg_id.attr.mode = 0644;
    sc8989x_sysfs->attr_sc8989x_sel_reg_id.show = sc8989x_reg_id_show;
    sc8989x_sysfs->attr_sc8989x_sel_reg_id.store = sc8989x_reg_id_store;

    sysfs_attr_init(&sc8989x_sysfs->attr_sc8989x_reg_val.attr);
    sc8989x_sysfs->attr_sc8989x_reg_val.attr.name = "sc8989x_reg_val";
    sc8989x_sysfs->attr_sc8989x_reg_val.attr.mode = 0644;
    sc8989x_sysfs->attr_sc8989x_reg_val.show = sc8989x_reg_val_show;
    sc8989x_sysfs->attr_sc8989x_reg_val.store = sc8989x_reg_val_store;

    ret = sysfs_create_group(&info->psy_usb->dev.kobj, &sc8989x_sysfs->attr_g);
    if (ret < 0)
        dev_err(info->dev, "Cannot create sysfs , ret = %d\n", ret);

    return ret;
}

static int sc8989x_charger_usb_get_property(struct power_supply *psy,
        enum power_supply_property psp,
        union power_supply_propval *val)
{
    struct sc8989x_charger_info *info = power_supply_get_drvdata(psy);
    u32 cur, health, enabled = 0;
    int ret = 0;

    if (!info)
        return -ENOMEM;

    mutex_lock(&info->lock);

    switch (psp) {
    case POWER_SUPPLY_PROP_STATUS:
        val->intval = sc8989x_charger_get_status(info);
        break;

    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
        if (!info->charging) {
            val->intval = 0;
        } else {
            ret = sc8989x_charger_get_current(info, &cur);
            if (ret)
                goto out;

            val->intval = cur;
        }
        break;

    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
        if (!info->charging) {
            val->intval = 0;
        } else {
            ret = sc8989x_charger_get_limit_current(info, &cur);
            if (ret)
                goto out;

            val->intval = cur;
        }
        break;

    case POWER_SUPPLY_PROP_HEALTH:
        if (info->charging) {
            val->intval = 0;
        } else {
            ret = sc8989x_charger_get_health(info, &health);
            if (ret)
                goto out;

            val->intval = health;
        }
        break;

    case POWER_SUPPLY_PROP_CALIBRATE:
        ret = regmap_read(info->pmic, info->charger_pd, &enabled);
        if (ret) {
            dev_err(info->dev, "get sc8989x charge status failed\n");
            goto out;
        }

        val->intval = !(enabled & info->charger_pd_mask);
        break;
    default:
        ret = -EINVAL;
    }

out:
    mutex_unlock(&info->lock);
    return ret;
}

static int sc8989x_charger_usb_set_property(struct power_supply *psy,
        enum power_supply_property psp,
        const union power_supply_propval *val)
{
    struct sc8989x_charger_info *info = power_supply_get_drvdata(psy);
    int ret = 0;
    u32 input_vol = 0;
    bool present = false;

    if (!info)
        return -ENOMEM;

    /*
     * It can cause the sysdum due to deadlock, that get value from fgu when
     * psp == POWER_SUPPLY_PROP_STATUS of psp == POWER_SUPPLY_PROP_CALIBRATE.
     */
    if (psp == POWER_SUPPLY_PROP_STATUS || psp == POWER_SUPPLY_PROP_CALIBRATE) {
        present = sc8989x_charger_is_bat_present(info);
        ret = sc8989x_charger_get_charge_voltage(info, &input_vol);
        if (ret) {
            input_vol = 0;
            dev_err(info->dev, "failed to get charge voltage, ret = %d\n", ret);
        }
    }

    mutex_lock(&info->lock);

    switch (psp) {
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
        ret = sc8989x_charger_set_current(info, val->intval);
        if (ret < 0)
            dev_err(info->dev, "set charge current failed\n");
        break;

    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
        ret = sc8989x_charger_set_limit_current(info, val->intval, false);
        if (ret < 0)
            dev_err(info->dev, "set input current limit failed\n");
        break;

    case POWER_SUPPLY_PROP_STATUS:
        ret = sc8989x_charger_set_status(info, val->intval, input_vol, present);
        if (ret < 0)
            dev_err(info->dev, "set charge status failed\n");
        break;

    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
        ret = sc8989x_charger_set_termina_vol(info, val->intval / 1000);
        if (ret < 0)
            dev_err(info->dev, "failed to set terminate voltage\n");
        break;

    case POWER_SUPPLY_PROP_CALIBRATE:
        if (val->intval == true) {
            ret = sc8989x_charger_start_charge(info);
            if (ret)
                dev_err(info->dev, "start charge failed\n");
        } else if (val->intval == false) {
            sc8989x_charger_stop_charge(info, present);
        }
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        info->is_charger_online = val->intval;
        if (val->intval == true) {
            schedule_delayed_work(&info->wdt_work, 0);
        } else {
            info->actual_limit_current = 0;
            cancel_delayed_work_sync(&info->wdt_work);
        }
        break;

    default:
        ret = -EINVAL;
    }

    mutex_unlock(&info->lock);
    return ret;
}

static int sc8989x_charger_property_is_writeable(struct power_supply *psy,
        enum power_supply_property psp)
{
    int ret;

    switch (psp) {
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
    case POWER_SUPPLY_PROP_STATUS:
    case POWER_SUPPLY_PROP_PRESENT:
    case POWER_SUPPLY_PROP_CALIBRATE:
        ret = 1;
        break;

    default:
        ret = 0;
    }

    return ret;
}

static enum power_supply_property sc8989x_usb_props[] = {
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
    POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_CALIBRATE,
};

static const struct power_supply_desc sc8989x_charger_desc = {
    .name            = "sc8989x_charger",
    .type            = POWER_SUPPLY_TYPE_USB,
    .properties        = sc8989x_usb_props,
    .num_properties        = ARRAY_SIZE(sc8989x_usb_props),
    .get_property        = sc8989x_charger_usb_get_property,
    .set_property        = sc8989x_charger_usb_set_property,
    .property_is_writeable    = sc8989x_charger_property_is_writeable,
};


static void sc8989x_charger_feed_watchdog_work(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct sc8989x_charger_info *info = container_of(dwork,
                                        struct sc8989x_charger_info,
                                        wdt_work);
    int ret;

    ret = sc8989x_charger_feed_watchdog(info);
    if (ret)
        schedule_delayed_work(&info->wdt_work, HZ * 5);
    else
        schedule_delayed_work(&info->wdt_work, HZ * 15);
}

#if CONFIG_REGULATOR
static bool sc8989x_charger_check_otg_valid(struct sc8989x_charger_info *info)
{
    int ret;
    u8 value = 0;
    bool status = false;

    ret = sc8989x_read(info, SC8989X_REG_03, &value);
    if (ret) {
        dev_err(info->dev, "get sc8989x charger otg valid status failed\n");
        return status;
    }

    if (value & REG03_OTG_MASK)
        status = true;
    else
        dev_err(info->dev, "otg is not valid, REG_3 = 0x%x\n", value);

    return status;
}

static bool sc8989x_charger_check_otg_fault(struct sc8989x_charger_info *info)
{
    int ret;
    u8 value = 0;
    bool status = true;

    ret = sc8989x_read(info, SC8989X_REG_0C, &value);
    if (ret) {
        dev_err(info->dev, "get sc8989x charger otg fault status failed\n");
        return status;
    }

    if (!(value & REG0C_OTG_FAULT))
        status = false;
    else
        dev_err(info->dev, "boost fault occurs, REG_0C = 0x%x\n",
                value);

    return status;
}

static void sc8989x_charger_otg_work(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct sc8989x_charger_info *info = container_of(dwork,
                                        struct sc8989x_charger_info, otg_work);
    bool otg_valid = sc8989x_charger_check_otg_valid(info);
    bool otg_fault;
    int ret, retry = 0;

    if (otg_valid)
        goto out;

    do {
        otg_fault = sc8989x_charger_check_otg_fault(info);
        if (!otg_fault) {
            ret = sc8989x_update_bits(info, SC8989X_REG_03,
                                      REG03_CHG_MASK,
                                      REG03_CHG_DISABLE << REG03_CHG_SHIFT);

            ret = sc8989x_update_bits(info, SC8989X_REG_03,
                                      REG03_OTG_MASK,
                                      REG03_OTG_ENABLE << REG03_OTG_SHIFT);
            if (ret)
                dev_err(info->dev, "restart sc8989x charger otg failed\n");
        }

        otg_valid = sc8989x_charger_check_otg_valid(info);
    } while (!otg_valid && retry++ < SC8989X_OTG_RETRY_TIMES);

    if (retry >= SC8989X_OTG_RETRY_TIMES) {
        dev_err(info->dev, "Restart OTG failed\n");
        return;
    }

out:
    schedule_delayed_work(&info->otg_work, msecs_to_jiffies(1500));
}

static int sc8989x_charger_enable_otg(struct regulator_dev *dev)
{
    struct sc8989x_charger_info *info = rdev_get_drvdata(dev);
    int ret;

    mutex_lock(&info->lock);

    /*
     * Disable charger detection function in case
     * affecting the OTG timing sequence.
     */
    ret = regmap_update_bits(info->pmic, info->charger_detect,
                             BIT_DP_DM_BC_ENB, BIT_DP_DM_BC_ENB);
    if (ret) {
        dev_err(info->dev, "failed to disable bc1.2 detect function.\n");
        mutex_unlock(&info->lock);
        return ret;
    }

    ret = sc8989x_update_bits(info, SC8989X_REG_03, REG03_CHG_MASK,
                              REG03_CHG_DISABLE << REG03_CHG_SHIFT);

    ret = sc8989x_update_bits(info, SC8989X_REG_03, REG03_OTG_MASK,
                              REG03_OTG_ENABLE << REG03_OTG_SHIFT);

    if (ret) {
        dev_err(info->dev, "enable sc8989x otg failed\n");
        regmap_update_bits(info->pmic, info->charger_detect,
                           BIT_DP_DM_BC_ENB, 0);
        mutex_unlock(&info->lock);
        return ret;
    }

    info->otg_enable = true;
    schedule_delayed_work(&info->wdt_work,
                          msecs_to_jiffies(SC8989X_WDT_VALID_MS));
    schedule_delayed_work(&info->otg_work,
                          msecs_to_jiffies(SC8989X_OTG_VALID_MS));

    mutex_unlock(&info->lock);

    return 0;
}

static int sc8989x_charger_disable_otg(struct regulator_dev *dev)
{
    struct sc8989x_charger_info *info = rdev_get_drvdata(dev);
    int ret;

    mutex_lock(&info->lock);

    info->otg_enable = false;
    cancel_delayed_work_sync(&info->wdt_work);
    cancel_delayed_work_sync(&info->otg_work);
    ret = sc8989x_update_bits(info, SC8989X_REG_03, REG03_CHG_MASK,
                              REG03_CHG_ENABLE << REG03_CHG_SHIFT);
    ret = sc8989x_update_bits(info, SC8989X_REG_03,
                              REG03_OTG_MASK, REG03_OTG_DISABLE);
    if (ret) {
        dev_err(info->dev, "disable sc8989x otg failed\n");
        mutex_unlock(&info->lock);
        return ret;
    }

    /* Enable charger detection function to identify the charger type */
    ret = regmap_update_bits(info->pmic, info->charger_detect,
                              BIT_DP_DM_BC_ENB, 0);
    if (ret)
        dev_err(info->dev, "enable BC1.2 failed\n");

    mutex_unlock(&info->lock);

    return ret;
}

static int sc8989x_charger_vbus_is_enabled(struct regulator_dev *dev)
{
    struct sc8989x_charger_info *info = rdev_get_drvdata(dev);
    int ret;
    u8 val;

    mutex_lock(&info->lock);
    ret = sc8989x_read(info, SC8989X_REG_03, &val);
    if (ret) {
        dev_err(info->dev, "failed to get sc8989x otg status\n");
        mutex_unlock(&info->lock);
        return ret;
    }

    val &= REG03_OTG_MASK;

    mutex_unlock(&info->lock);

    return val;
}

static const struct regulator_ops sc8989x_charger_vbus_ops = {
    .enable = sc8989x_charger_enable_otg,
    .disable = sc8989x_charger_disable_otg,
    .is_enabled = sc8989x_charger_vbus_is_enabled,
};

static const struct regulator_desc sc8989x_charger_vbus_desc = {
    .name = "sc8989x_otg_vbus",
    .of_match = "sc8989x_otg_vbus",
    .type = REGULATOR_VOLTAGE,
    .owner = THIS_MODULE,
    .ops = &sc8989x_charger_vbus_ops,
    .fixed_uV = 5000000,
    .n_voltages = 1,
};

static int sc8989x_charger_register_vbus_regulator(struct sc8989x_charger_info *info)
{
    struct regulator_config cfg = { };
    struct regulator_dev *reg;
    int ret = 0;

    cfg.dev = info->dev;
    cfg.driver_data = info;
    reg = devm_regulator_register(info->dev,
                                  &sc8989x_charger_vbus_desc, &cfg);
    if (IS_ERR(reg)) {
        ret = PTR_ERR(reg);
        dev_err(info->dev, "Can't register regulator:%d\n", ret);
    }

    return ret;
}

#else
static int sc8989x_charger_register_vbus_regulator(struct sc8989x_charger_info *info)
{
    return 0;
}
#endif

static void sc8989x_charger_dump_reg_work(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct sc8989x_charger_info *info = container_of(dwork,
                                        struct sc8989x_charger_info, dump_work);

    sc8989x_charger_dump_register(info);

    schedule_delayed_work(&info->dump_work, HZ * 15);
}

static int sc8989x_charger_probe(struct i2c_client *client,
                                 const struct i2c_device_id *id)
{
    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
    struct device *dev = &client->dev;
    struct power_supply_config charger_cfg = { };
    struct sc8989x_charger_info *info;
    struct device_node *regmap_np;
    struct platform_device *regmap_pdev;
    int ret;
    bool bat_present;

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(dev, "No support for SMBUS_BYTE_DATA\n");
        return -ENODEV;
    }

    pr_info("%s (%s): initializing...\n", __func__, SC8989X_DRV_VERSION);

    info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
    if (!info)
        return -ENOMEM;
    info->client = client;
    info->dev = dev;

    alarm_init(&info->wdg_timer, ALARM_BOOTTIME, NULL);

    mutex_init(&info->lock);
    mutex_init(&info->input_limit_cur_lock);

    INIT_DELAYED_WORK(&info->otg_work, sc8989x_charger_otg_work);
    INIT_DELAYED_WORK(&info->wdt_work, sc8989x_charger_feed_watchdog_work);
    INIT_DELAYED_WORK(&info->dump_work, sc8989x_charger_dump_reg_work);

    i2c_set_clientdata(client, info);

    ret = sc8989x_charger_is_fgu_present(info);
    if (ret) {
        dev_err(dev, "sc27xx_fgu not ready.\n");
        return -EPROBE_DEFER;
    }

    ret = sc8989x_charger_register_vbus_regulator(info);
    if (ret) {
        dev_err(dev, "failed to register vbus regulator.\n");
        return ret;
    }

    regmap_np = of_find_compatible_node(NULL, NULL, "sprd,sc27xx-syscon");
    if (!regmap_np)
        regmap_np = of_find_compatible_node(NULL, NULL, "sprd,ump962x-syscon");

    if (regmap_np) {
        if (of_device_is_compatible(regmap_np->parent, "sprd,sc2721"))
            info->charger_pd_mask = SC8989X_DISABLE_PIN_MASK_2721;
        else
            info->charger_pd_mask = SC8989X_DISABLE_PIN_MASK;
    } else {
        dev_err(dev, "unable to get syscon node\n");
        return -ENODEV;
    }

    ret = of_property_read_u32_index(regmap_np, "reg", 1,
                                     &info->charger_detect);
    if (ret) {
        dev_err(dev, "failed to get charger_detect\n");
        return -EINVAL;
    }

    ret = of_property_read_u32_index(regmap_np, "reg", 2,
                                     &info->charger_pd);
    if (ret) {
        dev_err(dev, "failed to get charger_pd reg\n");
        return ret;
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
                    &sc8989x_charger_desc,
                    &charger_cfg);

    if (IS_ERR(info->psy_usb)) {
        dev_err(dev, "failed to register power supply\n");
        ret = PTR_ERR(info->psy_usb);
        goto err_mutex_lock;
    }

    ret = sc8989x_charger_hw_init(info);
    if (ret) {
        dev_err(dev, "failed to sc8989x_charger_hw_init\n");
        goto err_mutex_lock;
    }

    bat_present = sc8989x_charger_is_bat_present(info);
    sc8989x_charger_stop_charge(info, bat_present);

    device_init_wakeup(info->dev, true);

    ret = sc8989x_register_sysfs(info);
    if (ret) {
        dev_err(info->dev, "register sysfs fail, ret = %d\n", ret);
        goto err_sysfs;
    }

    ret = sc8989x_update_bits(info, SC8989X_REG_07, REG07_TWD_MASK,
                              REG07_TWD_40S << REG07_TWD_SHIFT);
    if (ret) {
        dev_err(info->dev, "Failed to enable sc8989x watchdog\n");
        return ret;
    }

    return 0;

err_sysfs:
    sysfs_remove_group(&info->psy_usb->dev.kobj, &info->sysfs->attr_g);
err_mutex_lock:
    mutex_destroy(&info->lock);

    return ret;
}

static void sc8989x_charger_shutdown(struct i2c_client *client)
{
    struct sc8989x_charger_info *info = i2c_get_clientdata(client);
    int ret = 0;

    cancel_delayed_work_sync(&info->wdt_work);
    cancel_delayed_work_sync(&info->dump_work);
    if (info->otg_enable) {
        info->otg_enable = false;
        cancel_delayed_work_sync(&info->otg_work);
        ret = sc8989x_update_bits(info, SC8989X_REG_03,
                                  REG03_OTG_MASK,
                                  0);
        if (ret)
            dev_err(info->dev, "disable sc8989x otg failed ret = %d\n", ret);

        /* Enable charger detection function to identify the charger type */
        ret = regmap_update_bits(info->pmic, info->charger_detect,
                                 BIT_DP_DM_BC_ENB, 0);
        if (ret)
            dev_err(info->dev,
                    "enable charger detection function failed ret = %d\n", ret);
    }
}

static int sc8989x_charger_remove(struct i2c_client *client)
{
    struct sc8989x_charger_info *info = i2c_get_clientdata(client);

    cancel_delayed_work_sync(&info->dump_work);
    cancel_delayed_work_sync(&info->wdt_work);
    cancel_delayed_work_sync(&info->otg_work);

    return 0;
}

#ifdef CONFIG_PM_SLEEP
static int sc8989x_charger_suspend(struct device *dev)
{
    struct sc8989x_charger_info *info = dev_get_drvdata(dev);
    ktime_t now, add;
    unsigned int wakeup_ms = SC8989X_WDG_TIMER_MS;

    if (info->otg_enable || info->is_charger_online)
        /* feed watchdog first before suspend */
        sc8989x_charger_feed_watchdog(info);

    if (!info->otg_enable)
        return 0;

    cancel_delayed_work_sync(&info->dump_work);
    cancel_delayed_work_sync(&info->wdt_work);

    now = ktime_get_boottime();
    add = ktime_set(wakeup_ms / MSEC_PER_SEC,
                    (wakeup_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
    alarm_start(&info->wdg_timer, ktime_add(now, add));

    return 0;
}

static int sc8989x_charger_resume(struct device *dev)
{
    struct sc8989x_charger_info *info = dev_get_drvdata(dev);

    if (info->otg_enable || info->is_charger_online)
        /* feed watchdog first before suspend */
        sc8989x_charger_feed_watchdog(info);

    if (!info->otg_enable)
        return 0;

    alarm_cancel(&info->wdg_timer);

    schedule_delayed_work(&info->wdt_work, HZ * 15);
    schedule_delayed_work(&info->dump_work, HZ * 15);

    return 0;
}
#endif

static const struct dev_pm_ops sc8989x_charger_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(sc8989x_charger_suspend,
                            sc8989x_charger_resume)
};

static const struct i2c_device_id sc8989x_i2c_id[] = {
    {"sc8989x_chg", 0},
    {}
};

static const struct of_device_id sc8989x_charger_of_match[] = {
    { .compatible = "sc,sc8989x_chg", },
    { }
};

MODULE_DEVICE_TABLE(of, sc8989x_charger_of_match);

static struct i2c_driver sc8989x_charger_driver = {
    .driver = {
        .name = "sc8989x_chg",
        .of_match_table = sc8989x_charger_of_match,
        .pm = &sc8989x_charger_pm_ops,
    },
    .probe = sc8989x_charger_probe,
    .shutdown = sc8989x_charger_shutdown,
    .remove = sc8989x_charger_remove,
    .id_table = sc8989x_i2c_id,
};

module_i2c_driver(sc8989x_charger_driver);

MODULE_AUTHOR("Aiden Yu<Aiden-yu@southchip.com>");
MODULE_DESCRIPTION("SC8989X Charger Driver");
MODULE_LICENSE("GPL v2");

