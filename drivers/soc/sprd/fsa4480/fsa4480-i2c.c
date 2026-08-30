/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/soc/sprd/fsa4480-i2c.h>

#include <linux/extcon.h>

/*#define FSA4480_ENABLE_AUTO_DETECT*/

#define FSA4480_I2C_NAME	"fsa4480-driver"

#define FSA4480_DEVICE_INFO 0x0
#define FSA4480_SWITCH_SETTINGS 0x04
#define FSA4480_SWITCH_CONTROL  0x05
#define FSA4480_SWITCH_STATUS1  0x07
#define FSA4480_SLOW_L          0x08
#define FSA4480_SLOW_R          0x09
#define FSA4480_SLOW_MIC        0x0A
#define FSA4480_SLOW_SENSE      0x0B
#define FSA4480_SLOW_GND        0x0C
#define FSA4480_DELAY_L_R       0x0D
#define FSA4480_DELAY_L_MIC     0x0E
#define FSA4480_DELAY_L_SENSE   0x0F
#define FSA4480_DELAY_L_AGND    0x10
#define FSA4480_FUNC_EN         0x12
#define FSA4480_JACK_STAT       0x17
#define FSA4480_RESET           0x1E
#define FSA4480_CURR_SRC        0x1F

#define FSA4480_I2C_RETRIES     0x05
#define FSA4480_I2C_WRITE_READ_BACK

enum {
	TYPEC_UNATTACHED        = 0,
	TYPEC_ATTACHED_AUDIO    = 1,
};

struct fsa4480_priv {
	struct regmap *regmap;
	struct device *dev;
	struct power_supply *usb_psy;
	struct notifier_block psy_nb;
	atomic_t usbc_mode;
	struct work_struct usbc_analog_work;
	struct blocking_notifier_head fsa4480_notifier;
	struct mutex notification_lock;
	struct extcon_dev *edev;
};

struct fsa4480_reg_val {
	u16 reg;
	u8 val;
};


static const struct regmap_config fsa4480_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = FSA4480_RESET,
};

static const struct fsa4480_reg_val fsa_reg_i2c_defaults[] = {
	{FSA4480_SLOW_L, 0x00},
	{FSA4480_SLOW_R, 0x00},
	{FSA4480_SLOW_MIC, 0x00},
	{FSA4480_SLOW_SENSE, 0x00},
	{FSA4480_SLOW_GND, 0x00},
	{FSA4480_DELAY_L_R, 0x00},
	{FSA4480_DELAY_L_MIC, 0x00},
	{FSA4480_DELAY_L_SENSE, 0x00},
	{FSA4480_DELAY_L_AGND, 0x09},
	/*{FSA4480_CURR_SRC, 0x0F},*/
	{FSA4480_SWITCH_SETTINGS, 0x98},
};

static void fsa4480_usbc_update_settings(struct fsa4480_priv *fsa_priv,
		u32 switch_control, u32 switch_enable)
{
#ifdef FSA4480_I2C_WRITE_READ_BACK
	int retry_cnt = 0;
	u32 reg_switch_enable = 0;
	u32 reg_switch_control = 0;
#endif

	if (!fsa_priv->regmap) {
		dev_err(fsa_priv->dev, "%s: regmap invalid\n", __func__);
		return;
	}
#ifdef FSA4480_I2C_WRITE_READ_BACK
	for (retry_cnt = 0; retry_cnt < FSA4480_I2C_RETRIES; retry_cnt++) {
#endif

		regmap_write(fsa_priv->regmap, FSA4480_SWITCH_SETTINGS, 0x80);
		regmap_write(fsa_priv->regmap, FSA4480_SWITCH_CONTROL, switch_control);
		/* FSA4480 chip hardware requirement */
		usleep_range(50, 55);
		regmap_write(fsa_priv->regmap, FSA4480_SWITCH_SETTINGS, switch_enable);

#ifdef FSA4480_I2C_WRITE_READ_BACK
		regmap_read(fsa_priv->regmap, FSA4480_SWITCH_SETTINGS, &reg_switch_enable);
		regmap_read(fsa_priv->regmap, FSA4480_SWITCH_CONTROL, &reg_switch_control);
		if (reg_switch_enable == switch_enable
			&& reg_switch_control == switch_control) {
			break;
		}
		msleep_interruptible(10);
	}
	if (retry_cnt < FSA4480_I2C_RETRIES) {
		dev_info(fsa_priv->dev, "update regs(0x%x, 0x%x) successfully, retry %d",
				switch_enable, switch_control, retry_cnt);
	} else {
		dev_info(fsa_priv->dev, "update regs(0x%x, 0x%x) failed", switch_enable, switch_control);
	}
#endif
}

static int fsa4480_usbc_analog_setup_switches(struct fsa4480_priv *fsa_priv)
{
#ifdef FSA4480_ENABLE_AUTO_DETECT
	u32 switch_enable = 0;
	u32 jack_status = 0;
#endif
	int rc = 0;
	int mode = 0;
	struct device *dev;

	if (!fsa_priv)
		return -EINVAL;
	dev = fsa_priv->dev;
	if (!dev)
		return -EINVAL;

	mutex_lock(&fsa_priv->notification_lock);
	mode = atomic_read(&(fsa_priv->usbc_mode));


	dev_info(dev, "%s: mode = %d\n", __func__, mode);

	switch (mode) {
	/* add all modes FSA should notify for in here */
	case TYPEC_ATTACHED_AUDIO:
		/* activate switches */
		fsa4480_usbc_update_settings(fsa_priv, 0x00, 0x9F);

#if !defined(FSA4480_ENABLE_AUTO_DETECT)
		msleep_interruptible(30);
#else
		msleep_interruptible(1);
		regmap_write(fsa_priv->regmap, FSA4480_FUNC_EN, 0x0D/*0x1*/);
		msleep_interruptible(30);
		regmap_read(fsa_priv->regmap, FSA4480_JACK_STAT, &jack_status);

		switch (jack_status) {
		case 0x08:
		case 0x04:
			dev_info(dev, "%s: 4 pole detected, jack_status=0x%x\n", __func__, jack_status);
			break;
		case 0x02:
			dev_info(dev, "%s: 3 pole detected, jack_status=0x%x\n", __func__, jack_status);
			regmap_read(fsa_priv->regmap, FSA4480_SWITCH_SETTINGS, &switch_enable);
			switch_enable |= 0x7;
			regmap_write(fsa_priv->regmap, FSA4480_SWITCH_SETTINGS, (switch_enable & 0x7F));
			msleep_interruptible(50);
			regmap_write(fsa_priv->regmap, FSA4480_SWITCH_SETTINGS, (switch_enable | 0x80));
			break;
		default:
			dev_info(dev, "%s: No audio accessory was recognized. jack_status=0x%x\n",
				__func__, jack_status);
		}
#endif

		/* notify call chain on event */
		blocking_notifier_call_chain(&fsa_priv->fsa4480_notifier, mode, NULL);
		break;
	case TYPEC_UNATTACHED:
		/* notify call chain on event */
		blocking_notifier_call_chain(&fsa_priv->fsa4480_notifier,
				mode/*POWER_SUPPLY_TYPEC_NONE*/, NULL);

		/* deactivate switches */
		fsa4480_usbc_update_settings(fsa_priv, 0x18, 0x98);

		break;
	default:
		/* ignore other usb connection modes */
		break;
	}

	mutex_unlock(&fsa_priv->notification_lock);
	return rc;
}

/*
 * fsa4480_reg_notifier - register notifier block with fsa driver
 *
 * @nb - notifier block of fsa4480
 * @node - phandle node to fsa4480 device
 *
 * Returns 0 on success, or error code
 */
int fsa4480_reg_notifier(struct notifier_block *nb,
			 struct device_node *node)
{
	int rc = 0;
	struct i2c_client *client = of_find_i2c_device_by_node(node);
	struct fsa4480_priv *fsa_priv;

	if (!client)
		return -EINVAL;

	fsa_priv = (struct fsa4480_priv *)i2c_get_clientdata(client);
	if (!fsa_priv)
		return -EINVAL;

	rc = blocking_notifier_chain_register
				(&fsa_priv->fsa4480_notifier, nb);
	if (rc)
		return rc;

	/*
	 * as part of the init sequence check if there is a connected
	 * USB C analog adapter
	 */
	dev_dbg(fsa_priv->dev, "%s: verify if USB adapter is already inserted\n",
		__func__);
	rc = fsa4480_usbc_analog_setup_switches(fsa_priv);

	return rc;
}
EXPORT_SYMBOL(fsa4480_reg_notifier);

/*
 * fsa4480_unreg_notifier - unregister notifier block with fsa driver
 *
 * @nb - notifier block of fsa4480
 * @node - phandle node to fsa4480 device
 *
 * Returns 0 on pass, or error code
 */
int fsa4480_unreg_notifier(struct notifier_block *nb,
			     struct device_node *node)
{
	struct i2c_client *client = of_find_i2c_device_by_node(node);
	struct fsa4480_priv *fsa_priv;

	if (!client)
		return -EINVAL;

	fsa_priv = (struct fsa4480_priv *)i2c_get_clientdata(client);
	if (!fsa_priv)
		return -EINVAL;

	fsa4480_usbc_update_settings(fsa_priv, 0x18, 0x98);
	return blocking_notifier_chain_unregister
					(&fsa_priv->fsa4480_notifier, nb);
}
EXPORT_SYMBOL(fsa4480_unreg_notifier);

static int fsa4480_validate_display_port_settings(struct fsa4480_priv *fsa_priv)
{
	u32 switch_status = 0;

	regmap_read(fsa_priv->regmap, FSA4480_SWITCH_STATUS1, &switch_status);

	if ((switch_status != 0x23) && (switch_status != 0x1C)) {
		pr_err("AUX SBU1/2 switch status is invalid = %u\n",
				switch_status);
		return -EIO;
	}

	return 0;
}
/*
 * fsa4480_switch_event - configure FSA switch position based on event
 *
 * @node - phandle node to fsa4480 device
 * @event - fsa_function enum
 *
 * Returns int on whether the switch happened or not
 */
int fsa4480_switch_event(struct device_node *node,
			 enum fsa_function event)
{
	int switch_control = 0;
	struct i2c_client *client = of_find_i2c_device_by_node(node);
	struct fsa4480_priv *fsa_priv;
	unsigned long state;
#if 0
	u32 reg_value = 0;
#endif

/* ZTE_chenjun */
	pr_info("%s: event %d\n", __func__, event);

	if (!client)
		return -EINVAL;

	fsa_priv = (struct fsa4480_priv *)i2c_get_clientdata(client);
	if (!fsa_priv)
		return -EINVAL;
	if (!fsa_priv->regmap)
		return -EINVAL;

	switch (event) {
	case FSA_MIC_GND_SWAP:
		if (atomic_read(&(fsa_priv->usbc_mode)) == TYPEC_ATTACHED_AUDIO) {
			mutex_lock(&fsa_priv->notification_lock);
			regmap_read(fsa_priv->regmap, FSA4480_SWITCH_CONTROL,
					&switch_control);
			if ((switch_control & 0x07) == 0x07)
				switch_control = 0x0;
			else
				switch_control = 0x7;
			fsa4480_usbc_update_settings(fsa_priv, switch_control, 0x9F);
			mutex_unlock(&fsa_priv->notification_lock);
		} else {
			pr_info("%s: NOT in audio accessory mode, request is ignored", __func__);
		}
		break;
	case FSA_SET_MIC_TO_SBU1:
	case FSA_SET_MIC_TO_SBU2:
		if (atomic_read(&(fsa_priv->usbc_mode)) == TYPEC_ATTACHED_AUDIO) {
			mutex_lock(&fsa_priv->notification_lock);
			fsa4480_usbc_update_settings(fsa_priv, ((event == FSA_SET_MIC_TO_SBU1) ? 0x7 : 0x0), 0x9F);
			mutex_unlock(&fsa_priv->notification_lock);
		} else {
			pr_info("%s: NOT in audio accessory mode, request is ignored", __func__);
		}
		break;
	case FSA_USBC_ORIENTATION_CC1:
		fsa4480_usbc_update_settings(fsa_priv, 0x18, 0xF8);
		return fsa4480_validate_display_port_settings(fsa_priv);
	case FSA_USBC_ORIENTATION_CC2:
		fsa4480_usbc_update_settings(fsa_priv, 0x78, 0xF8);
		return fsa4480_validate_display_port_settings(fsa_priv);
	case FSA_USBC_DISPLAYPORT_DISCONNECTED:
		fsa4480_usbc_update_settings(fsa_priv, 0x18, 0x98);
		break;
	case FSA_USBC_AUDIO_ATTACHED:
	case FSA_USBC_AUDIO_UNATTACHED:
		state = (event == FSA_USBC_AUDIO_ATTACHED) ? 1 : 0;
		if (atomic_read(&(fsa_priv->usbc_mode)) != state) {
			atomic_set(&(fsa_priv->usbc_mode), state);
			fsa4480_usbc_analog_setup_switches(fsa_priv);
		}
		break;
	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(fsa4480_switch_event);

static void fsa4480_usbc_analog_work_fn(struct work_struct *work)
{
	struct fsa4480_priv *fsa_priv =
		container_of(work, struct fsa4480_priv, usbc_analog_work);

	if (!fsa_priv) {
		pr_err("%s: fsa container invalid\n", __func__);
		return;
	}
	fsa4480_usbc_analog_setup_switches(fsa_priv);
	pm_relax(fsa_priv->dev);
}

static void fsa4480_update_reg_defaults(struct regmap *regmap)
{
	u8 i;

	for (i = 0; i < ARRAY_SIZE(fsa_reg_i2c_defaults); i++)
		regmap_write(regmap, fsa_reg_i2c_defaults[i].reg,
				   fsa_reg_i2c_defaults[i].val);
}

static void fsa4480_read_chip_id(struct regmap *regmap)
{
	u32 chid_id = 0;

	regmap_read(regmap, FSA4480_DEVICE_INFO, &chid_id);
	if (0x49 == chid_id) {
		pr_info("%s, the chip product is HL5280\n", __func__);
	} else if (0x09 == chid_id) {
		pr_info("%s, the chip product is FSA4480\n", __func__);
	}  else if (0x11 == chid_id) {
		pr_info("%s, the chip product is WAS4780\n", __func__);
	} else {
		pr_info("%s, the chip product id:%d is unsupported\n", __func__, chid_id);
	}
}

#if 0
static int fsa4480_typec_headset_notifier(struct notifier_block *nb,
					unsigned long state, void *data)
{
	struct fsa4480_priv *fsa_priv =
			container_of(nb, struct fsa4480_priv, psy_nb);
	struct device *dev;

	if (!fsa_priv)
		return -EINVAL;

	dev = fsa_priv->dev;

	pr_info("%s: state=%d\n", __func__, state);

	if (atomic_read(&(fsa_priv->usbc_mode)) != state) {
		atomic_set(&(fsa_priv->usbc_mode), state);
		dev_dbg(dev, "%s: queueing usbc_analog_work\n",	__func__);
		pm_stay_awake(fsa_priv->dev);
		schedule_work(&fsa_priv->usbc_analog_work);
	}
	return NOTIFY_OK;
}
#endif

static ssize_t fsa4480_reg_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	int err = 0;
	u8 reg = 0x0;
	u32 reg_value = 0x0;
	ssize_t buf_size = 0;
	struct fsa4480_priv *fsa_priv = dev_get_drvdata(dev);

	if (!fsa_priv) {
		pr_info("fsa_priv is null");
		return -EINVAL;
	}
	for (reg = 0x0; reg <= FSA4480_CURR_SRC; reg++) {
		err = regmap_read(fsa_priv->regmap, reg, &reg_value);
		if (err < 0) {
			pr_info("%s: failed to read reg %#04x\n", __func__, reg);
		}
		buf_size += snprintf(buf + buf_size, PAGE_SIZE - buf_size, "reg:0x%02x=0x%04x\n", reg, reg_value);
	}
	return buf_size;
}

static ssize_t fsa4480_reg_store(struct device *dev,
					struct device_attribute *attr, const char *buf,
					size_t count)
{
	int err = 0;
	struct fsa4480_priv *fsa_priv = dev_get_drvdata(dev);
	u32 databuf[2] = {0};

	if (!fsa_priv) {
		pr_info("fsa_priv is null");
		return -EINVAL;
	}

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2) {
		err = regmap_write(fsa_priv->regmap, databuf[0], databuf[1]);
		if (err < 0) {
			pr_info("%s: failed to write reg %#04x\n", __func__, databuf[0]);
		}
	}
	return count;
}

static struct device_attribute fsa4480_reg_attr =
	__ATTR(reg, S_IWUSR | S_IRUGO, fsa4480_reg_show, fsa4480_reg_store);

static int fsa4480_probe(struct i2c_client *i2c,
			 const struct i2c_device_id *id)
{
	struct fsa4480_priv *fsa_priv;
	int rc = 0;

	pr_info("fsa4480_probe enter");

	fsa_priv = devm_kzalloc(&i2c->dev, sizeof(*fsa_priv),
				GFP_KERNEL);
	if (!fsa_priv)
		return -ENOMEM;

	fsa_priv->dev = &i2c->dev;
/*
	fsa_priv->usb_psy = power_supply_get_by_name("usb");
	if (!fsa_priv->usb_psy) {
		rc = -EPROBE_DEFER;
		dev_err(fsa_priv->dev,
			"%s: could not get USB psy info: %d\n",
			__func__, rc);
		goto err_data;
	}
	power_supply_put(fsa_priv->usb_psy);
*/
	fsa_priv->regmap = devm_regmap_init_i2c(i2c, &fsa4480_regmap_config);
	if (IS_ERR_OR_NULL(fsa_priv->regmap)) {
		dev_err(fsa_priv->dev, "%s: Failed to initialize regmap: %d\n",
			__func__, rc);
		if (!fsa_priv->regmap) {
			rc = -EINVAL;
			goto err_data;
		}
		rc = PTR_ERR(fsa_priv->regmap);
		goto err_data;
	}

	atomic_set(&(fsa_priv->usbc_mode), -1);

	fsa4480_read_chip_id(fsa_priv->regmap);
	fsa4480_update_reg_defaults(fsa_priv->regmap);

#if 0
	fsa_priv->psy_nb.notifier_call = fsa4480_typec_headset_notifier;
	fsa_priv->psy_nb.priority = 0;

	fsa_priv->edev = extcon_get_edev_by_phandle(fsa_priv->dev, 0);
	if (IS_ERR(fsa_priv->edev)) {
		rc = PTR_ERR(fsa_priv->edev);
		dev_err(fsa_priv->dev, "%s() failed to find extcon device, %d\n", rc);
		goto err_data;
	}

	rc = extcon_register_notifier(fsa_priv->edev,
			EXTCON_JACK_HEADPHONE, &(fsa_priv->psy_nb));
	if (rc) {
		dev_err(fsa_priv->dev, "failed to register extcon HEADPHONE notifier, ret %d\n", rc);
		goto err_data;
	}
#endif
/*
	if (extcon_get_state(hdst->edev, EXTCON_JACK_HEADPHONE)) {
		hdst->typec_attached = true;
		sprd_headset_typec_work(hdst);
	}
*/

	mutex_init(&fsa_priv->notification_lock);
	i2c_set_clientdata(i2c, fsa_priv);

	dev_set_drvdata(&i2c->dev, fsa_priv);
	rc = sysfs_create_file(&i2c->dev.kobj, &fsa4480_reg_attr.attr);
	if (rc < 0)	{
		pr_err("%s failed to create sys file fsa4480_reg_attr", __func__);
	}

	INIT_WORK(&fsa_priv->usbc_analog_work,
		  fsa4480_usbc_analog_work_fn);

	fsa_priv->fsa4480_notifier.rwsem =
		(struct rw_semaphore)__RWSEM_INITIALIZER
		((fsa_priv->fsa4480_notifier).rwsem);
	fsa_priv->fsa4480_notifier.head = NULL;

	pr_info("fsa4480_probe successfully");

	return 0;

err_data:
	devm_kfree(&i2c->dev, fsa_priv);
	return rc;
}

static int fsa4480_remove(struct i2c_client *i2c)
{
	struct fsa4480_priv *fsa_priv =
			(struct fsa4480_priv *)i2c_get_clientdata(i2c);

	if (!fsa_priv)
		return -EINVAL;

	fsa4480_usbc_update_settings(fsa_priv, 0x18, 0x98);
	cancel_work_sync(&fsa_priv->usbc_analog_work);
	pm_relax(fsa_priv->dev);
	/* deregister from PMI */
#if 0
	extcon_unregister_notifier(fsa_priv->edev, EXTCON_JACK_HEADPHONE, &fsa_priv->psy_nb);
#endif
	/*power_supply_put(fsa_priv->usb_psy);*/
	mutex_destroy(&fsa_priv->notification_lock);
	dev_set_drvdata(&i2c->dev, NULL);

	return 0;
}

static const struct of_device_id fsa4480_i2c_dt_match[] = {
	{
		.compatible = "sprd,fsa4480-i2c",
	},
	{}
};

static struct i2c_driver fsa4480_i2c_driver = {
	.driver = {
		.name = FSA4480_I2C_NAME,
		.of_match_table = fsa4480_i2c_dt_match,
	},
	.probe = fsa4480_probe,
	.remove = fsa4480_remove,
};

static int __init fsa4480_init(void)
{
	int rc;

	rc = i2c_add_driver(&fsa4480_i2c_driver);
	if (rc)
		pr_err("fsa4480: Failed to register I2C driver: %d\n", rc);

	return rc;
}
module_init(fsa4480_init);

static void __exit fsa4480_exit(void)
{
	i2c_del_driver(&fsa4480_i2c_driver);
}
module_exit(fsa4480_exit);

MODULE_DESCRIPTION("FSA4480 I2C driver");
MODULE_LICENSE("GPL v2");
