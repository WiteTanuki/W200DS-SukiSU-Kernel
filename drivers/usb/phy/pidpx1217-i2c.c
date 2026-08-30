#include <linux/module.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of_device.h>
#include <linux/types.h>

#define PI2DPX1217_VENDOR_ID_OFFSET 0x00
#define PI2DPX1217_DEVICE_ID_OFFSET 0x01
#define PI2DPX1217_MODE_OFFSET 0x03
#define PI2DPX1217_HPD_OFFSET 0x04
#define PI2DPX1217_EQ_FG_RX2_OFFSET 0x05
#define PI2DPX1217_EQ_FG_TX2_OFFSET 0x06
#define PI2DPX1217_EQ_FG_TX1_OFFSET 0x07
#define PI2DPX1217_EQ_FG_RX1_OFFSET 0x08
#define PI2DPX1217_AUX_FLIP_CONTROL_OFFSET 0x09
#define PI2DPX1217_AUX_SWITCH_OFFSET 0x12
#define PI2DPX1217_EQ_FG_VALUE 0x7C
#define PI2DPX1217_HPD_enable 0x06
#define PI2DPX1217_HPD_disable 0x04
#define USB_CABLE_POSITIVE_DIRECTION 0
#define USB_CABLE_NEGATIVE_DIRECTION 1
#define PI2DPX1217_4LANE_DP_MODE 1
#define PI2DPX1217_USB3_MODE 2
#define PI2DPX1217_2LANE_DP_USB3_MODE 3
#define PI2DPX1217_4LANE_DP_MODE_POSITIVE_VALUE 0X20
#define PI2DPX1217_USB3_MODE_POSITIVE_VALUE 0X40
#define PI2DPX1217_2LANE_DP_USB3_MODE_POSITIVE_VALUE 0X60
#define PI2DPX1217_4LANE_DP_MODE_NEGATIVE_VALUE 0X30
#define PI2DPX1217_USB3_MODE_NEGATIVE_VALUE 0X50
#define PI2DPX1217_2LANE_DP_USB3_MODE_NEGATIVE_VALUE 0X70
#define PI2DPX1217_REG3_DEFAULT_VALUE 0XC0
#define PI2DPX1217_REG4_DEFAULT_VALUE 0X04
#define PI2DPX1217_EQ_FG_DEFAULT_VALUE 0X08


static int PIDPX1217_VALID  = 0;



struct redriver_pidpx1217 {
	struct i2c_client *client;
	struct device  *dev_t;
	struct mutex  mutex;
	struct class  *device_class;
};
static struct redriver_pidpx1217 *info;

// pi2dpx1217 i2c operator interfaces
static int pi2dpx1217_write_byte( struct i2c_client *client, u8 offset, u8 val)
{
	struct redriver_pidpx1217 *info = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&info->mutex);
	ret = i2c_smbus_write_byte_data(client, offset, val);
	mutex_unlock(&info->mutex);
	if (ret < 0)
		pr_err("%s: (0x%x) error, ret(%d)\n", __func__, offset, ret);

	return ret;
}

static int pi2dpx1217_read_byte( struct i2c_client *client, u8 offset, u8 *val)
{
	struct redriver_pidpx1217 *info = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&info->mutex);
	ret = i2c_smbus_read_byte_data(client, offset);
	mutex_unlock(&info->mutex);
	if (ret < 0) {
		pr_err("%s: (0x%x) error, ret(%d)\n", __func__, offset, ret);
		return ret;
	}

	ret &= 0xff;
	*val = ret;
	return 0;
}

#define PIDPX1217_REG_ATTR(field, reg_address) \
static ssize_t field##_show(struct device *dev, struct device_attribute *attr, char *buf) \
{ \
	struct redriver_pidpx1217 *info = dev_get_drvdata(dev); \
	u8 value; \
	int ret; \
 \
	ret = pi2dpx1217_read_byte(info->client, reg_address, &value); \
	if (ret < 0) { \
		pr_err("%s: update reg fail %d!\n", __func__, ret); \
		return ret; \
	} \
	return snprintf(buf, PAGE_SIZE, "0x%x\n", value); \
} \
static ssize_t field##_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size) \
{ \
	struct redriver_pidpx1217 *info = dev_get_drvdata(dev); \
	u8 value; \
	int ret; \
 \
	ret = kstrtou8(buf, 16, &value); \
	if (ret != 0) \
		return -EINVAL; \
	ret = pi2dpx1217_write_byte(info->client, reg_address, value); \
	if (ret < 0) { \
		pr_err("%s: update reg fail %d!\n", __func__, ret); \
		return ret; \
	} \
	return size; \
} \
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field##_show, field##_store);

PIDPX1217_REG_ATTR(pidpx1217_reg3, PI2DPX1217_MODE_OFFSET)
PIDPX1217_REG_ATTR(pidpx1217_reg4, PI2DPX1217_HPD_OFFSET)
PIDPX1217_REG_ATTR(pidpx1217_reg5, PI2DPX1217_EQ_FG_RX2_OFFSET)
PIDPX1217_REG_ATTR(pidpx1217_reg6, PI2DPX1217_EQ_FG_TX2_OFFSET)
PIDPX1217_REG_ATTR(pidpx1217_reg7, PI2DPX1217_EQ_FG_TX1_OFFSET)
PIDPX1217_REG_ATTR(pidpx1217_reg8, PI2DPX1217_EQ_FG_RX1_OFFSET)
PIDPX1217_REG_ATTR(pidpx1217_reg9, PI2DPX1217_AUX_FLIP_CONTROL_OFFSET)
PIDPX1217_REG_ATTR(pidpx1217_reg12, PI2DPX1217_AUX_SWITCH_OFFSET)


static struct device_attribute *pi2dpx1217_reg_attributes[] = {
	&dev_attr_pidpx1217_reg3,
	&dev_attr_pidpx1217_reg4,
	&dev_attr_pidpx1217_reg5,
	&dev_attr_pidpx1217_reg6,
	&dev_attr_pidpx1217_reg7,
	&dev_attr_pidpx1217_reg8,
	&dev_attr_pidpx1217_reg9,
	&dev_attr_pidpx1217_reg12,
	/*end*/
	NULL
};
int pi2dpx1217_redriver_is_valid(void)
{
	if (PIDPX1217_VALID == 0)
		return 0;
	else
		return 1;
}
EXPORT_SYMBOL(pi2dpx1217_redriver_is_valid);

int pi2dpx1217_restore_default_value(void)
{
	int ret = 0;
	
	if (!info) {
		pr_err("%s: pi2dpx1217 not probe\n", __func__);
		return -ENODEV;
	}
	pr_info("%s: pi2dpx1217 restore default value \n", __func__);
	ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_MODE_OFFSET, PI2DPX1217_REG3_DEFAULT_VALUE);
	ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_HPD_OFFSET, PI2DPX1217_REG4_DEFAULT_VALUE);
	ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_RX2_OFFSET, PI2DPX1217_EQ_FG_DEFAULT_VALUE);
	ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX2_OFFSET, PI2DPX1217_EQ_FG_DEFAULT_VALUE);
	ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_DEFAULT_VALUE);
	ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_DEFAULT_VALUE);
	if (ret < 0 ) {
		pr_err("%s: pi2dpx1217 restore default value fail %d \n", __func__, ret);
		goto err;
	}
	return 0;
err:
	return ret;
}
EXPORT_SYMBOL(pi2dpx1217_restore_default_value);

int pi2dpx1217_set_operating_mode (int operation_mode, int direction)
{
	int ret = 0;
	
	if (!info) {
		pr_err("%s: pi2dpx1217 not probe\n", __func__);
		return -ENODEV;
	}
	//positive reg setting
	pr_info("%s: pi2dpx1217 start set operation mode= %d direction =%d\n", __func__, operation_mode, direction);
	if (operation_mode == PI2DPX1217_4LANE_DP_MODE) { 
		if (direction == USB_CABLE_POSITIVE_DIRECTION){
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_MODE_OFFSET, PI2DPX1217_4LANE_DP_MODE_POSITIVE_VALUE);
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_HPD_OFFSET, PI2DPX1217_HPD_enable);
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_RX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
		} else {
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_MODE_OFFSET, PI2DPX1217_4LANE_DP_MODE_NEGATIVE_VALUE);
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_HPD_OFFSET, PI2DPX1217_HPD_enable);
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_RX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
			ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
		}
	} else if (operation_mode == PI2DPX1217_USB3_MODE) {
			if (direction == USB_CABLE_POSITIVE_DIRECTION){
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_MODE_OFFSET, PI2DPX1217_USB3_MODE_POSITIVE_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_RX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
			} else {
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_MODE_OFFSET, PI2DPX1217_USB3_MODE_NEGATIVE_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_RX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
			}
	} else {
			if (direction == USB_CABLE_POSITIVE_DIRECTION){
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_MODE_OFFSET, PI2DPX1217_2LANE_DP_USB3_MODE_POSITIVE_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_HPD_OFFSET, PI2DPX1217_HPD_enable);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_RX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
			} else {
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_MODE_OFFSET, PI2DPX1217_2LANE_DP_USB3_MODE_NEGATIVE_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_HPD_OFFSET, PI2DPX1217_HPD_enable);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_RX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX2_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
				ret = pi2dpx1217_write_byte(info->client, PI2DPX1217_EQ_FG_TX1_OFFSET, PI2DPX1217_EQ_FG_VALUE);
			}
	}
	
	if (ret < 0 ) {
		pr_err("%s: pi2dpx1217 set operate mode fail %d \n", __func__, ret);
		goto err;
	}
	return 0;
err:
	return ret;
}
EXPORT_SYMBOL(pi2dpx1217_set_operating_mode);

/*
static int pisdpx1217_initialization(struct redriver_pidpx1217 *info) 
{
	int ret = 0;
	
	pr_info("initialize all register success.\n ");
	
	return ret;
}
*/
static int pi2dpx1217_create_device(struct redriver_pidpx1217 *info)
{
	struct device_attribute **attrs = pi2dpx1217_reg_attributes;
	struct device_attribute *attr;
	int err;

	pr_info("%s:\n", __func__);
	info->device_class = class_create(THIS_MODULE, "pidpx1217");
	if (IS_ERR(info->device_class))
		return PTR_ERR(info->device_class);

	info->dev_t = device_create(info->device_class, NULL, 0, NULL, "pidpx1217");
	if (IS_ERR(info->dev_t))
		return PTR_ERR(info->dev_t);

	dev_set_drvdata(info->dev_t, info);

	while ((attr = *attrs++)) {
		err = device_create_file(info->dev_t, attr);
		if (err) {
			device_destroy(info->device_class, 0);
			return err;
		}
	}
	return 0;
}

static int pi2dpx1217_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	u8 value;

	pr_info("pi2dpx1217 redriver start probe.\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "device doesn't suport I2C\n");
		return -ENODEV;
	}
	info = devm_kzalloc(&client->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	info->client = client;
	i2c_set_clientdata(client, info);
	mutex_init(&info->mutex);
	/*
	//initialize device register
	
	ret = pisdpx1217_initialization(info);
	if (ret < 0)
		return ret;
	*/
	/*read pidpx1217 vnedor id && device id */
	ret = pi2dpx1217_read_byte(info->client, PI2DPX1217_VENDOR_ID_OFFSET, &value);
	if (ret < 0) {
		pr_err("%s: PIDPX1217 reg0 fail= %d\n", __func__, ret);
		goto err_reg;
	}
	pr_info("%s: read reg0 success %d \n", __func__, value);
	ret = pi2dpx1217_read_byte(info->client, PI2DPX1217_DEVICE_ID_OFFSET, &value);
	if (ret < 0) {
		pr_err("%s: read reg1 fail = %d \n", __func__, ret);
		goto err_reg;
	}
	pr_info("%s: read reg1 success %d \n", __func__, value);
	ret = pi2dpx1217_create_device(info);
	if (ret) {
		pr_err("%s: create device failed %d! \n", __func__, ret);
		goto err_device_create;
	}
	//pi2dpx1217_set_operating_mode
	//pi2dpx1217_set_operating_mode(1);
	PIDPX1217_VALID = 1;
	pr_info("pi2dpx1217 redriver ending probe.\n");
	return 0;
err_reg:
err_device_create:
	mutex_destroy(&info->mutex);
	i2c_set_clientdata(client, NULL);
	/*kfree(info);
	info = NULL;*/
	return ret;
}

static const struct of_device_id pi2dpx1217_of_match[] = {
	{
		.compatible = "pi2dpx1217 redriver",
	},
	{ }
};
MODULE_DEVICE_TABLE(of, pi2dpx1217_of_match);
  
static const struct i2c_device_id pi2dpx1217_i2c_ids[] = {
	{"pi2dpx1217 redriver", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, pi2dpx1217_i2c_ids);
  
static struct i2c_driver pidpredriver_i2c = {
	.probe = pi2dpx1217_i2c_probe,
	.driver = {
		.name = "pi2dpx1217",
		.of_match_table = of_match_ptr(pi2dpx1217_of_match),
	},
	.id_table = pi2dpx1217_i2c_ids,
};
  
module_i2c_driver(pidpredriver_i2c);
  
MODULE_DESCRIPTION("usb30 phy piddpx1217 redrive driver");
MODULE_LICENSE("GPL v2");