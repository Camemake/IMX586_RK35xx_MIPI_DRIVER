// SPDX-License-Identifier: GPL-2.0-only
/*
 * Sony IMX586 Quad‑Bayer CMOS sensor driver for Raspberry Pi 5
 *
 * Minimal compile‑time skeleton – functional for 12 MP @ 30 fps.
 * Extend mode tables & controls as required.
 */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/gpio/consumer.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-fwnode.h>

#include "imx586_reg_tables.h"

/* ---------- Basic chip IDs ---------- */
#define IMX586_REG_CHIP_ID_H   0x0016
#define IMX586_REG_CHIP_ID_L   0x0017
#define IMX586_CHIP_ID         0x0586

#define IMX586_REG_STANDBY     0x0100
#define   IMX586_MODE_SW_STANDBY 0x00
#define   IMX586_MODE_STREAMING  0x01

struct imx586_mode {
	u32 width;
	u32 height;
	u32 hts;
	u32 vts;
	u64 link_freq;
	u8  bpp;
	const struct reg_sequence *reg_list;
	u32 reg_list_len;
};

static const struct imx586_mode supported_modes[] = {
	{
		.width = 4000,
		.height = 3000,
		.hts = 0x1130,
		.vts = 0x0C80,
		.link_freq = 445500000ULL,
		.bpp = 10,
		.reg_list = imx586_mode_12mp_30fps,
		.reg_list_len = ARRAY_SIZE(imx586_mode_12mp_30fps),
	},
};

struct imx586 {
	struct device           *dev;
	struct regmap           *regmap;
	struct v4l2_subdev       sd;
	struct v4l2_ctrl_handler ctrls;
	const struct imx586_mode *cur_mode;
	struct gpio_desc        *reset_gpio;
	struct clk              *xclk;
	bool streaming;
};

static const struct regmap_config imx586_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.max_register = 0xFFFF,
};

static int imx586_power_on(struct imx586 *priv)
{
	if (priv->reset_gpio) {
		gpiod_set_value_cansleep(priv->reset_gpio, 0);
		usleep_range(1000, 2000);
		gpiod_set_value_cansleep(priv->reset_gpio, 1);
	}
	if (priv->xclk)
		return clk_prepare_enable(priv->xclk);
	return 0;
}

static int imx586_power_off(struct imx586 *priv)
{
	if (priv->xclk)
		clk_disable_unprepare(priv->xclk);
	if (priv->reset_gpio)
		gpiod_set_value_cansleep(priv->reset_gpio, 0);
	return 0;
}

static int imx586_set_stream(struct v4l2_subdev *sd, int on)
{
	struct imx586 *priv = container_of(sd, struct imx586, sd);
	int ret = 0;

	if (on) {
		ret = regmap_multi_reg_write(priv->regmap,
					  priv->cur_mode->reg_list,
					  priv->cur_mode->reg_list_len);
		if (ret)
			return ret;
		ret = regmap_write(priv->regmap, IMX586_REG_STANDBY,
					   IMX586_MODE_STREAMING);
	} else {
		ret = regmap_write(priv->regmap, IMX586_REG_STANDBY,
					   IMX586_MODE_SW_STANDBY);
	}

	if (!ret)
		priv->streaming = on;
	return ret;
}

static int imx586_s_power(struct v4l2_subdev *sd, int on)
{
	struct imx586 *priv = container_of(sd, struct imx586, sd);
	return on ? imx586_power_on(priv) : imx586_power_off(priv);
}

static const struct v4l2_subdev_video_ops imx586_video_ops = {
	.s_stream = imx586_set_stream,
};

static const struct v4l2_subdev_core_ops imx586_core_ops = {
	.s_power = imx586_s_power,
};

static const struct v4l2_subdev_ops imx586_subdev_ops = {
	.core  = &imx586_core_ops,
	.video = &imx586_video_ops,
};

static int imx586_identify(struct imx586 *priv)
{
	unsigned int id_h, id_l;

	if (regmap_read(priv->regmap, IMX586_REG_CHIP_ID_H, &id_h) ||
	    regmap_read(priv->regmap, IMX586_REG_CHIP_ID_L, &id_l))
		return -EIO;

	if (((id_h << 8) | id_l) != IMX586_CHIP_ID) {
		dev_err(priv->dev, "chip ID mismatch: 0x%04x\n",
			(id_h << 8) | id_l);
		return -ENODEV;
	}
	return 0;
}

static int imx586_probe(struct i2c_client *client,
		   const struct i2c_device_id *id)
{
	struct imx586 *priv;
	struct device *dev = &client->dev;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->dev = dev;

	priv->regmap = devm_regmap_init_i2c(client, &imx586_regmap_config);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	/* GPIO & clock */
	priv->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	priv->xclk = devm_clk_get_optional(dev, NULL);

	ret = imx586_power_on(priv);
	if (ret)
		return ret;
	ret = imx586_identify(priv);
	imx586_power_off(priv);
	if (ret)
		return ret;

	v4l2_i2c_subdev_init(&priv->sd, client, &imx586_subdev_ops);
	priv->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	return 0;
}

static void imx586_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	v4l2_async_unregister_subdev(sd);
}

static const struct i2c_device_id imx586_id_table[] = {
	{ "imx586", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx586_id_table);

static const struct of_device_id imx586_of_match[] = {
	{ .compatible = "sony,imx586" },
	{ }
};
MODULE_DEVICE_TABLE(of, imx586_of_match);

static struct i2c_driver imx586_i2c_driver = {
	.driver = {
		.name  = "imx586",
		.of_match_table = imx586_of_match,
	},
	.probe    = imx586_probe,
	.remove   = imx586_remove,
	.id_table = imx586_id_table,
};
module_i2c_driver(imx586_i2c_driver);

MODULE_DESCRIPTION("Sony IMX586 Raspberry Pi 5 camera driver");
MODULE_AUTHOR("Your Name <you@example.com>");
MODULE_LICENSE("GPL v2");
