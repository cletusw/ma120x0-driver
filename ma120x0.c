#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <linux/kernel.h>

//------------------------------------------------------------------eh_clear---
// flip to clear error registers
#define ma_eh_clear__a 45
#define ma_eh_clear__len 1
#define ma_eh_clear__mask 0x04
#define ma_eh_clear__shift 0x02
#define ma_eh_clear__reset 0x00
//-----------------------------------------------------------------error_acc---
// accumulated errors,  at and after triggering
#define ma_error_acc__a 109
#define ma_error_acc__len 8
#define ma_error_acc__mask 0xff
#define ma_error_acc__shift 0x00
#define ma_error_acc__reset 0x00
//---------------------------------------------------------------------error---
// current error flag monitor reg - for app. ctrl.
#define ma_error__a 124
#define ma_error__len 8
#define ma_error__mask 0xff
#define ma_error__shift 0x00
#define ma_error__reset 0x00

#define SOC_ENUM_ERR(xname, xenum)\
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),\
	.access = SNDRV_CTL_ELEM_ACCESS_READ,\
	.info = snd_soc_info_enum_double,\
	.get = snd_soc_get_enum_double, .put = snd_soc_put_enum_double,\
	.private_value = (unsigned long)&(xenum) }


/* Private data for the MA120X0 */
struct ma120x0_priv {
	struct regmap *regmap;
	struct snd_soc_component *component;
	struct gpio_desc *enable_gpio;
	struct gpio_desc *mute_gpio;
};

static struct ma120x0_priv *priv_data;


//Alsa Controls
static const char * const err_flycap_text[] = {"Ok", "Error"};
static const char * const err_overcurr_text[] = {"Ok", "Error"};
static const char * const err_pllerr_text[] = {"Ok", "Error"};
static const char * const err_pvddunder_text[] = {"Ok", "Error"};
static const char * const err_overtempw_text[] = {"Ok", "Error"};
static const char * const err_overtempe_text[] = {"Ok", "Error"};
static const char * const err_pinlowimp_text[] = {"Ok", "Error"};
static const char * const err_dcprot_text[] = {"Ok", "Error"};

static const struct soc_enum err_flycap_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 0, 3, err_flycap_text);
static const struct soc_enum err_overcurr_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 1, 3, err_overcurr_text);
static const struct soc_enum err_pllerr_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 2, 3, err_pllerr_text);
static const struct soc_enum err_pvddunder_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 3, 3, err_pvddunder_text);
static const struct soc_enum err_overtempw_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 4, 3, err_overtempw_text);
static const struct soc_enum err_overtempe_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 5, 3, err_overtempe_text);
static const struct soc_enum err_pinlowimp_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 6, 3, err_pinlowimp_text);
static const struct soc_enum err_dcprot_ctrl =
	SOC_ENUM_SINGLE(ma_error__a, 7, 3, err_dcprot_text);

static const struct snd_kcontrol_new ma120x0_snd_controls[] = {
	//Enum Error Monitor (read-only)
	SOC_ENUM_ERR("I.Err flycap", err_flycap_ctrl),
	SOC_ENUM_ERR("J.Err overcurr", err_overcurr_ctrl),
	SOC_ENUM_ERR("K.Err pllerr", err_pllerr_ctrl),
	SOC_ENUM_ERR("L.Err pvddunder", err_pvddunder_ctrl),
	SOC_ENUM_ERR("M.Err overtempw", err_overtempw_ctrl),
	SOC_ENUM_ERR("N.Err overtempe", err_overtempe_ctrl),
	SOC_ENUM_ERR("O.Err pinlowimp", err_pinlowimp_ctrl),
	SOC_ENUM_ERR("P.Err dcprot", err_dcprot_ctrl),
};


//Codec Driver
static int ma120x0_clear_err(struct snd_soc_component *component)
{
	int ret = 0;

	struct ma120x0_priv *ma120x0;

	ma120x0 = snd_soc_component_get_drvdata(component);

	ret = snd_soc_component_update_bits(component,
		ma_eh_clear__a, ma_eh_clear__mask, 0x00);
	if (ret < 0)
		return ret;

	ret = snd_soc_component_update_bits(component,
		ma_eh_clear__a, ma_eh_clear__mask, 0x04);
	if (ret < 0)
		return ret;

	ret = snd_soc_component_update_bits(component,
		ma_eh_clear__a, ma_eh_clear__mask, 0x00);
	if (ret < 0)
		return ret;

	return 0;
}

static int ma120x0_probe(struct snd_soc_component *component)
{
	struct ma120x0_priv *ma120x0;

	int ret = 0;

	pr_info("ma120x0: ma120x0_probe starting");

	ma120x0 = snd_soc_component_get_drvdata(component);

	//Reset error
	ma120x0_clear_err(component);
	if (ret < 0) {
		pr_info("ma120x0: ma120x0_probe completed with return code %d\n", ret);
		return ret;
	}

	//Check for errors
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x00, 0);
	if (ret < 0) {
		pr_info("ma120x0: ma120x0_probe completed with return code %d\n", ret);
		return ret;
	}
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x01, 0);
	if (ret < 0) {
		pr_info("ma120x0: ma120x0_probe completed with return code %d\n", ret);
		return ret;
	}
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x02, 0);
	if (ret < 0) {
		pr_info("ma120x0: ma120x0_probe completed with return code %d\n", ret);
		return ret;
	}
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x08, 0);
	if (ret < 0) {
		pr_info("ma120x0: ma120x0_probe completed with return code %d\n", ret);
		return ret;
	}
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x10, 0);
	if (ret < 0) {
		pr_info("ma120x0: ma120x0_probe completed with return code %d\n", ret);
		return ret;
	}
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x20, 0);
	if (ret < 0) {
		pr_info("ma120x0: ma120x0_probe completed with return code %d\n", ret);
		return ret;
	}
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x40, 0);
	if (ret < 0) {
		pr_info("ma120x0: ma120x0_probe completed with return code %d\n", ret);
		return ret;
	}
	ret = snd_soc_component_test_bits(component, ma_error_acc__a, 0x80, 0);
	if (ret < 0) {
		pr_info("ma120x0: ma120x0_probe completed with return code %d\n", ret);
		return ret;
	}

	pr_info("ma120x0: ma120x0_probe completed with return code %d\n", ret);

	return 0;
}

static int drv_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *control, int event)
{
	struct snd_soc_component *c = snd_soc_dapm_to_component(w->dapm);
	struct ma120x0_priv *priv_data = snd_soc_component_get_drvdata(c);
	bool enable;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		enable = true;
		break;
	case SND_SOC_DAPM_PRE_PMD:
		enable = false;
		break;
	default:
		WARN(1, "Unexpected event");
		return -EINVAL;
	}

	pr_info("ma120x0: drv_event, enable = %d\n", enable);

	if (enable) {
		// enable_gpio is active low
		gpiod_set_value_cansleep(priv_data->enable_gpio, 0);
		msleep(50);

		// mute_gpio is active (muted) low
		gpiod_set_value_cansleep(priv_data->mute_gpio, 1);
	} else {
		// mute_gpio is active (muted) low
		gpiod_set_value_cansleep(priv_data->mute_gpio, 0);
		msleep(50);

		// enable_gpio is active low
		gpiod_set_value_cansleep(priv_data->enable_gpio, 1);
	}

	return 0;
}

static const struct snd_soc_dapm_widget ma120x0_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("IN_A"),
	SND_SOC_DAPM_INPUT("IN_B"),

	SND_SOC_DAPM_OUT_DRV_E("DRV", SND_SOC_NOPM, 0, 0, NULL, 0, drv_event,
			       (SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	SND_SOC_DAPM_OUTPUT("OUT_A"),
	SND_SOC_DAPM_OUTPUT("OUT_B"),
};

static const struct snd_soc_dapm_route ma120x0_dapm_routes[] = {
	{ "DRV", NULL, "IN_A" },
	{ "DRV", NULL, "IN_B" },
	{ "OUT_A", NULL, "DRV" },
	{ "OUT_B", NULL, "DRV" },
};

static const struct snd_soc_component_driver ma120x0_component_driver = {
	.probe = ma120x0_probe,
	.dapm_widgets = ma120x0_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ma120x0_dapm_widgets),
	.dapm_routes = ma120x0_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(ma120x0_dapm_routes),
	.controls = ma120x0_snd_controls,
	.num_controls = ARRAY_SIZE(ma120x0_snd_controls),
	.idle_bias_on = 1,
	.use_pmdown_time = 1,
	.endianness = 1,
	.non_legacy_dai_naming = 1,
};


//I2C Driver
static const struct reg_default ma120x0_reg_defaults[] = {
	{	ma_error__a,	0x00	},
};

static bool ma120x0_reg_volatile(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case ma_error__a:
		return true;
	default:
		return false;
	}
}

static struct regmap_config ma120x0_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = 255,
	.volatile_reg = ma120x0_reg_volatile,

	.cache_type = REGCACHE_RBTREE,
	.reg_defaults = ma120x0_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(ma120x0_reg_defaults),
};

static int ma120x0_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *id)
{
	int ret;

	/* To tail ("wait on") the logs: */
	/* dmesg -w */
	/* (optionally use `-l info` for just info level logs) */
	pr_info("ma120x0: ma120x0_i2c_probe starting");

	priv_data = devm_kzalloc(&i2c->dev, sizeof(*priv_data), GFP_KERNEL);
	if (!priv_data)
		return -ENOMEM;
	i2c_set_clientdata(i2c, priv_data);

	priv_data->regmap = devm_regmap_init_i2c(i2c, &ma120x0_regmap_config);
	if (IS_ERR(priv_data->regmap)) {
		ret = PTR_ERR(priv_data->regmap);
		return ret;
	}

	//Startup sequence

	//Make sure the device is muted
	priv_data->mute_gpio = devm_gpiod_get_optional(&i2c->dev, "mute_gp",
		GPIOD_OUT_LOW);
	if (IS_ERR(priv_data->mute_gpio)) {
		ret = PTR_ERR(priv_data->mute_gpio);
		dev_err(&i2c->dev, "Failed to get mute gpio line: %d\n", ret);
		return ret;
	}
	msleep(50);

	//Enable ma120x0
	priv_data->enable_gpio = devm_gpiod_get_optional(&i2c->dev,
		"enable_gp", GPIOD_OUT_LOW);
	if (IS_ERR(priv_data->enable_gpio)) {
		ret = PTR_ERR(priv_data->enable_gpio);
		dev_err(&i2c->dev,
		"Failed to get ma120x0 enable gpio line: %d\n", ret);
		return ret;
	}
	msleep(50);

	ret = devm_snd_soc_register_component(&i2c->dev,
		&ma120x0_component_driver, NULL, 0);

	pr_info("ma120x0: ma120x0_i2c_probe completed with return code %d\n", ret);

	return ret;
}

static int ma120x0_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_component(&i2c->dev);
	i2c_set_clientdata(i2c, NULL);

	gpiod_set_value_cansleep(priv_data->mute_gpio, 0);
	msleep(30);
	gpiod_set_value_cansleep(priv_data->enable_gpio, 1);
	msleep(200);

	kfree(priv_data);

	return 0;
}

static void ma120x0_i2c_shutdown(struct i2c_client *i2c)
{
	snd_soc_unregister_component(&i2c->dev);
	i2c_set_clientdata(i2c, NULL);

	gpiod_set_value_cansleep(priv_data->mute_gpio, 0);
	msleep(30);
	gpiod_set_value_cansleep(priv_data->enable_gpio, 1);
	msleep(200);

	kfree(priv_data);
}

static const struct of_device_id ma120x0_of_match[] = {
	{ .compatible = "merus,ma120x0", },
	{},
};

MODULE_DEVICE_TABLE(of, ma120x0_of_match);

static const struct i2c_device_id ma120x0_i2c_id[] = {
	{ "ma120x0", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ma120x0_i2c_id);

static struct i2c_driver ma120x0_i2c_driver = {
	.driver = {
		.name = "ma120x0",
		.owner = THIS_MODULE,
		.of_match_table = ma120x0_of_match,
	},
	.probe = ma120x0_i2c_probe,
	.remove = ma120x0_i2c_remove,
	.shutdown = ma120x0_i2c_shutdown,
	.id_table = ma120x0_i2c_id,
};

module_i2c_driver(ma120x0_i2c_driver);

MODULE_DESCRIPTION("Partial ASoC driver for ma120x0 based on the ma120x0p one");
MODULE_AUTHOR("Clayton Watts <cletusw@gmail.com>");
MODULE_LICENSE("GPL v2");
