/*
 * Copyright(c) 2013 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact Information:
 * Intel Corporation
 */
 
/*
 * Driver for Cypress CY8C9540A I/O Expander and PWM
 *
 * Izmir-specific.
 * Based on gpio-adp5588.
 */
 
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pwm.h>
#include <linux/slab.h>
 
#define DRV_NAME			"cy8c9540a"
 
/* CY8C9540A settings */
#define NGPIO				40
#define NPWM				8
#define PWM_MAX_PERIOD			0xff
#define DEVID_FAMILY_CY8C9540A		0x40
#define DEVID_FAMILY_MASK		0xf0
#define NPORTS				6
 
/* Register offset  */
#define REG_INPUT_PORT0			0x00
#define REG_OUTPUT_PORT0		0x08
#define REG_INTR_STAT_PORT0		0x10
#define REG_PORT_SELECT			0x18
#define REG_INTR_MASK			0x19
#define REG_SELECT_PWM			0x1a
#define REG_PIN_DIR			0x1c
#define REG_DRIVE_PULLUP		0x1d
#define REG_PWM_SELECT			0x28
#define REG_PWM_CLK			0x29
#define REG_PWM_PERIOD			0x2a
#define REG_PWM_PULSE_W			0x2b
#define REG_ENABLE			0x2d
#define REG_DEVID_STAT			0x2e
 
struct cy8c9540a {
	struct i2c_client *client;
	struct gpio_chip gpio_chip;
	struct pwm_chip pwm_chip;
	struct mutex lock;
	/* protect serialized access to the interrupt controller bus */
	struct mutex irq_lock;
	/* cached output registers */
	u8 outreg_cache[NPORTS];
	/* cached IRQ mask */
	u8 irq_mask_cache[NPORTS];
	/* IRQ mask to be applied */
	u8 irq_mask[NPORTS];
};
 
/* Per-port GPIO offset */
static const u8 cy8c9540a_port_offs[] = {
	0,
	8,
	16,
	20,
	28,
	36,
};
 
/* Galileo-specific data.  */
 
#define GPIO_BASE_ID			16
#define GPIO_IRQBASE			64
#define PWM_BASE_ID			0
// TODO: Support dynamic PWM values
#define PWM_CLK				0x00	/* see resulting PWM_TCLK_NS */
#define PWM_TCLK_NS			31250	/* 32kHz */
#define SOC_GPIO_INT_PIN		13

#define PWM_CLK_32K			0x00
#define PWM_TCLK_NS_32K		31250
#define PWM_CLK_24M			0x01
#define PWM_TCLK_NS_24M		41
#define PWM_CLK_1_5M		0x02
#define PWM_TCLK_NS_1_5M	666
#define PWM_CLK_93_75K		0x03
#define PWM_TCLK_NS_93_75K	10666
#define PWM_CLK_367_6		0x04
#define PWM_TCLK_NS_367_6	2720348
 
#define NCLOCKS 5
static const unsigned int clock_select[][2] = {
	{PWM_CLK_32K, PWM_TCLK_NS_32K},
	{PWM_CLK_24M, PWM_TCLK_NS_24M},
	{PWM_CLK_1_5M, PWM_TCLK_NS_1_5M},
	{PWM_CLK_93_75K, PWM_TCLK_NS_93_75K},
	{PWM_CLK_367_6, PWM_TCLK_NS_367_6}
};

/* PWM-to-GPIO mapping (0 == first Cypress GPIO).  */
#define PWM_UNUSED			-1
static const int pwm2gpio_mapping[] = {
	PWM_UNUSED,
	3,
	PWM_UNUSED,
	2,
	9,
	1,
	8,
	0,
};
 
static inline u8 cypress_get_port(unsigned gpio)
{
	u8 i = 0;
	for (i = 0; i < sizeof(cy8c9540a_port_offs) - 1; i ++) {
		if (! (gpio / cy8c9540a_port_offs[i + 1]))
			break;
	}
	return i;
}
 
static inline u8 cypress_get_offs(unsigned gpio, u8 port)
{
	return gpio - cy8c9540a_port_offs[port];
}
 
static int cy8c9540a_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	s32 ret = 0;
	u8 port = 0;
	u8 in_reg = 0;
	struct cy8c9540a *dev =
	    container_of(chip, struct cy8c9540a, gpio_chip);
	struct i2c_client *client = dev->client;
 
	port = cypress_get_port(gpio);
	in_reg = REG_INPUT_PORT0 + port;
 
	ret = i2c_smbus_read_byte_data(client, in_reg);
	if (ret < 0) {
		dev_err(&client->dev, "can't read input port%u\n", in_reg);
	}
 
	return !!(ret & BIT(cypress_get_offs(gpio, port)));
}
 
static void cy8c9540a_gpio_set_value(struct gpio_chip *chip,
				   unsigned gpio, int val)
{
	s32 ret = 0;
	struct cy8c9540a *dev =
	    container_of(chip, struct cy8c9540a, gpio_chip);
	struct i2c_client *client = dev->client;
	u8 port = cypress_get_port(gpio);
	u8 out_reg = REG_OUTPUT_PORT0 + port;
 
	mutex_lock(&dev->lock);
 
	if (val) {
		dev->outreg_cache[port] |= BIT(cypress_get_offs(gpio, port));
	} else {
		dev->outreg_cache[port] &= ~BIT(cypress_get_offs(gpio, port));
	}
 
	ret = i2c_smbus_write_byte_data(client, out_reg,
					dev->outreg_cache[port]);
	if (ret < 0) {
		dev_err(&client->dev, "can't write output port%u\n", port);
	}
 
	mutex_unlock(&dev->lock);
}
 
static int cy8c9540a_gpio_set_drive(struct gpio_chip *chip, unsigned gpio,
					unsigned mode)
{
	int ret = 0;
	struct cy8c9540a *dev =
	    container_of(chip, struct cy8c9540a, gpio_chip);
	struct i2c_client *client = dev->client;
	u8 port = cypress_get_port(gpio);
	u8 pin = cypress_get_offs(gpio, port);
	u8 offs = 0;
	u8 val = 0;
 
	switch(mode) {
	case GPIOF_DRIVE_PULLUP:
		offs = 0x0;
		break;
	case GPIOF_DRIVE_STRONG:
		offs = 0x4;
		break;
	case GPIOF_DRIVE_HIZ:
		offs = 0x6;
		break;
	default:
		/*
		 * See databook for alternative modes.  This driver won't
		 * support them though.
		 */
		return -EINVAL;
		break;
	}
 
	mutex_lock(&dev->lock);
 
	ret = i2c_smbus_write_byte_data(client, REG_PORT_SELECT, port);
	if (ret < 0) {
		dev_err(&client->dev, "can't select port %u\n", port);
		goto end;
	}
 
	ret = i2c_smbus_read_byte_data(client, REG_DRIVE_PULLUP + offs);
	if (ret < 0) {
		dev_err(&client->dev, "can't read pin direction\n");
		goto end;
	}
 
	val = (u8)(ret | BIT(pin));
 
	ret = i2c_smbus_write_byte_data(client, REG_DRIVE_PULLUP + offs, val);
	if (ret < 0) {
		dev_err(&client->dev, "can't set drive mode port %u\n", port);
		goto end;
	}
 
	ret = 0;
 
end:
	mutex_unlock(&dev->lock);
	return ret;
}
 
static int cy8c9540a_gpio_direction(struct gpio_chip *chip, unsigned gpio,
					int out, int val)
{
	s32 ret = 0;
	u8 pins = 0;
	struct cy8c9540a *dev =
	    container_of(chip, struct cy8c9540a, gpio_chip);
	struct i2c_client *client = dev->client;
	u8 port = cypress_get_port(gpio);
 
	if (out) {
		cy8c9540a_gpio_set_value(chip, gpio, val);
	}
 
	mutex_lock(&dev->lock);
 
	ret = i2c_smbus_write_byte_data(client, REG_PORT_SELECT, port);
	if (ret < 0) {
		dev_err(&client->dev, "can't select port %u\n", port);
		goto end;
	}
 
	ret = i2c_smbus_read_byte_data(client, REG_PIN_DIR);
	if (ret < 0) {
		dev_err(&client->dev, "can't read pin direction\n");
		goto end;
	}
 
	pins = (u8)ret & 0xff;
	if (out) {
		pins &= ~BIT(cypress_get_offs(gpio, port));
	} else {
		pins |= BIT(cypress_get_offs(gpio, port));
	}
 
	ret = i2c_smbus_write_byte_data(client, REG_PIN_DIR, pins);
	if (ret < 0) {
		dev_err(&client->dev, "can't write pin direction\n");
	}
 
end:
	mutex_unlock(&dev->lock);
	return ret;
}
 
static int cy8c9540a_gpio_direction_output(struct gpio_chip *chip,
					unsigned gpio, int val)
{
	return cy8c9540a_gpio_direction(chip, gpio, 1, val);
}
 
static int cy8c9540a_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	return cy8c9540a_gpio_direction(chip, gpio, 0, 0);
}
 
static void cy8c9540a_irq_bus_lock(struct irq_data *d)
{
	struct cy8c9540a *dev = irq_data_get_irq_chip_data(d);
	mutex_lock(&dev->irq_lock);
}
 
static void cy8c9540a_irq_bus_sync_unlock(struct irq_data *d)
{
	struct cy8c9540a *dev = irq_data_get_irq_chip_data(d);
	struct i2c_client *client = dev->client;
	int ret = 0;
	int i = 0;
 
	for (i = 0; i < NPORTS; i++) {
		if (dev->irq_mask_cache[i] ^ dev->irq_mask[i]) {
			dev->irq_mask_cache[i] = dev->irq_mask[i];
			ret = i2c_smbus_write_byte_data(client, REG_PORT_SELECT, i);
			if (ret < 0) {
				dev_err(&client->dev, "can't select port %u\n", i);
				goto end;
			}
 
			ret = i2c_smbus_write_byte_data(client, REG_INTR_MASK, dev->irq_mask[i]);
			if (ret < 0) {
				dev_err(&client->dev, "can't write int mask on port %u\n", i);
				goto end;
			}
 
		}
	}
 
end:
	mutex_unlock(&dev->irq_lock);
}
 
static void cy8c9540a_irq_mask(struct irq_data *d)
{
	struct cy8c9540a *dev = irq_data_get_irq_chip_data(d);
	unsigned gpio = d->irq - GPIO_IRQBASE;
	u8 port = cypress_get_port(gpio);
 
	dev->irq_mask[port] |= BIT(cypress_get_offs(gpio, port));
}
 
static void cy8c9540a_irq_unmask(struct irq_data *d)
{
	struct cy8c9540a *dev = irq_data_get_irq_chip_data(d);
	unsigned gpio = d->irq - GPIO_IRQBASE;
	u8 port = cypress_get_port(gpio);
 
	dev->irq_mask[port] &= ~BIT(cypress_get_offs(gpio, port));
}
 
static int cy8c9540a_gpio_to_irq(struct gpio_chip *chip, unsigned gpio)
{
	return GPIO_IRQBASE + gpio;
}
 
static int cy8c9540a_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct cy8c9540a *dev = irq_data_get_irq_chip_data(d);
	int ret = 0;
 
	if ((type != IRQ_TYPE_EDGE_BOTH)) {
		dev_err(&dev->client->dev, "irq %d: unsupported type %d\n",
			d->irq, type);
		ret = -EINVAL;
		goto end;
	}
 
end:
	return ret; 
}
 
static struct irq_chip cy8c9540a_irq_chip = {
	.name			= "cy8c9540a-irq",
	.irq_mask		= cy8c9540a_irq_mask,
	.irq_unmask		= cy8c9540a_irq_unmask,
	.irq_bus_lock		= cy8c9540a_irq_bus_lock,
	.irq_bus_sync_unlock	= cy8c9540a_irq_bus_sync_unlock,
	.irq_set_type		= cy8c9540a_irq_set_type,
};
 
static irqreturn_t cy8c9540a_irq_handler(int irq, void *devid)
{
	struct cy8c9540a *dev = devid;
	u8 stat[NPORTS], pending = 0;
	unsigned port = 0, gpio = 0, gpio_irq = 0;
	int ret = 0;
 
	ret = i2c_smbus_read_i2c_block_data(dev->client, REG_INTR_STAT_PORT0,
		NPORTS, stat);	/* W1C  */
	if (ret < 0) {
		memset(stat, 0, sizeof(stat));
	}
 
	ret = IRQ_NONE;
 
	for (port = 0; port < NPORTS; port ++) {
		/* Databook doesn't specify if this is a post-mask status
		   register or not.  Consider it 'raw' for safety.  */
		mutex_lock(&dev->irq_lock);
		pending = stat[port] & (~dev->irq_mask[port]);
		mutex_unlock(&dev->irq_lock);
 
		while (pending) {
			ret = IRQ_HANDLED;
			gpio = __ffs(pending);
			pending &= ~BIT(gpio);
			gpio_irq = GPIO_IRQBASE + cy8c9540a_port_offs[port]
				+ gpio;
			handle_nested_irq(gpio_irq);
		}
	}
 
	return ret;
}
 
static int cy8c9540a_irq_setup(struct cy8c9540a *dev)
{
	struct i2c_client *client = dev->client;
	u8 dummy[NPORTS];
	unsigned gpio = 0;
	int ret = 0;
	int i = 0;
 
	mutex_init(&dev->irq_lock);
 
	/* Clear interrupt state */
 
	ret = i2c_smbus_read_i2c_block_data(dev->client, REG_INTR_STAT_PORT0,
		NPORTS, dummy);	/* W1C  */
	if (ret < 0) {
		dev_err(&client->dev, "couldn't clear int status\n");
		goto err;
	}
 
	/* Initialise interrupt mask */
 
	memset(dev->irq_mask_cache, 0xff, sizeof(dev->irq_mask_cache));
	memset(dev->irq_mask, 0xff, sizeof(dev->irq_mask));
	for (i = 0; i < NPORTS; i++) {
		ret = i2c_smbus_write_byte_data(client, REG_PORT_SELECT, i);
		if (ret < 0) {
			dev_err(&client->dev, "can't select port %u\n", i);
			goto err;
		}
 
		ret = i2c_smbus_write_byte_data(client, REG_INTR_MASK, dev->irq_mask[i]);
		if (ret < 0) {
			dev_err(&client->dev, "can't write int mask on port %u\n", i);
			goto err;
		}
	}
 
	/* Allocate external interrupt GPIO pin */
 
	ret = gpio_request(SOC_GPIO_INT_PIN, "cy8c9540a-int");
	if (ret) {
		dev_err(&client->dev, "failed to request gpio%u\n",
			SOC_GPIO_INT_PIN);
		goto err;
	}
 
	/* Allocate IRQ descriptors for Cypress GPIOs and set handler */
 
	ret = irq_alloc_descs(GPIO_IRQBASE, GPIO_IRQBASE, NGPIO, 0);
	if (ret < 0) {
		dev_err(&client->dev, "failed to allocate IRQ numbers\n");
		goto err_free_gpio;
	}
 
	for (gpio = 0; gpio < NGPIO; gpio++) {
		int irq = gpio + GPIO_IRQBASE;
		irq_set_chip_data(irq, dev);
		irq_set_chip_and_handler(irq, &cy8c9540a_irq_chip,
					 handle_edge_irq);
		irq_set_nested_thread(irq, 1);
		irq_set_noprobe(irq);
	}
 
	ret = request_threaded_irq(gpio_to_irq(SOC_GPIO_INT_PIN),
				   NULL,
				   cy8c9540a_irq_handler,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				   dev_name(&client->dev), dev);
	if (ret) {
		dev_err(&client->dev, "failed to request irq %d\n",
			client->irq);
		goto err_free_irq_descs;
	}
 
	return 0;
 
err_free_irq_descs:
	irq_free_descs(GPIO_IRQBASE, NGPIO);
err_free_gpio:
	gpio_free(SOC_GPIO_INT_PIN);
err:
	mutex_destroy(&dev->irq_lock);
	return ret;
}
 
static void cy8c9540a_irq_teardown(struct cy8c9540a *dev)
{
	mutex_destroy(&dev->irq_lock);
	irq_free_descs(GPIO_IRQBASE, NGPIO);
	free_irq(gpio_to_irq(SOC_GPIO_INT_PIN), dev);
	gpio_free(SOC_GPIO_INT_PIN);
}
 
static int cy8c9540a_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			     int duty_ns, int period_ns)
{
	int ret = 0;
	int period = 0, duty = 0;
	int i;
	int pwm_tclk_ns;
	int selected_clock = -1;

	struct cy8c9540a *dev =
	    container_of(chip, struct cy8c9540a, pwm_chip);
	struct i2c_client *client = dev->client;
 
	if (pwm->pwm > NPWM) {
		return -EINVAL;
	}

	/*
	 * We need to iterate over the available clocks
	 * to find a suitable source clock and divider
	 * combination
     */
	for (i = 0; i < NCLOCKS; i++) {
		/*
         * Select a clock candidate
		 */
		pwm_tclk_ns = clock_select[i][1];
		period = period_ns / pwm_tclk_ns;

		/*
		 * Check if the period can be achieved
		 * by this clock.
		 * checked by the PWM framework.
		 */
		if (period <= PWM_MAX_PERIOD && period > 0) {
			selected_clock = i;
			break;
		}
	}
 
	if (selected_clock < 0) {
		dev_err(&client->dev, "invalid period\n");
		return -EINVAL;
	} else {
		period = period_ns / pwm_tclk_ns;
		duty = duty_ns / pwm_tclk_ns;
	}
 
	mutex_lock(&dev->lock);
	ret = i2c_smbus_write_byte_data(client, REG_PWM_SELECT, (u8)pwm->pwm);
	if (ret < 0) {
		dev_err(&client->dev, "can't write to REG_PWM_SELECT\n");
		goto end;
	}
 
	ret = i2c_smbus_write_byte_data(client, REG_PWM_CLK, (u8)clock_select[selected_clock][0]);
	if (ret < 0) {
		dev_err(&client->dev, "can't write to REG_PWM_CLK\n");
		goto end;
	}

	ret = i2c_smbus_write_byte_data(client, REG_PWM_PERIOD, (u8)period);
	if (ret < 0) {
		dev_err(&client->dev, "can't write to REG_PWM_PERIOD\n");
		goto end;
	}
 
	ret = i2c_smbus_write_byte_data(client, REG_PWM_PULSE_W, (u8)duty);
	if (ret < 0) {
		dev_err(&client->dev, "can't write to REG_PWM_PULSE_W\n");
		goto end;
	}
 
end:
	mutex_unlock(&dev->lock);
 
	return ret;
}
 
static int cy8c9540a_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	int ret = 0;
	int gpio = 0;
	int port = 0, pin = 0;
	u8 out_reg = 0;
	u8 val = 0;
	struct cy8c9540a *dev =
	    container_of(chip, struct cy8c9540a, pwm_chip);
	struct i2c_client *client = dev->client;
 
	if (pwm->pwm > NPWM) {
		return -EINVAL;
	}
 
	gpio = pwm2gpio_mapping[pwm->pwm];
	port = cypress_get_port(gpio);
	pin = cypress_get_offs(gpio, port);
	out_reg = REG_OUTPUT_PORT0 + port;
 
	/* Set pin as output driving high */
	ret = cy8c9540a_gpio_direction(&dev->gpio_chip, gpio, 1, 1);
	if (val < 0) {
		dev_err(&client->dev, "can't set pwm%u as output\n", pwm->pwm);
		return ret;
	}
 
	mutex_lock(&dev->lock);
 
	/* Enable PWM */
	val = i2c_smbus_read_byte_data(client, REG_SELECT_PWM);
	if (val < 0) {
		dev_err(&client->dev, "can't read REG_SELECT_PWM\n");
		ret = val;
		goto end;
	}
	val |= BIT((u8)pin);
	ret = i2c_smbus_write_byte_data(client, REG_SELECT_PWM, val);
	if (ret < 0) {
		dev_err(&client->dev, "can't write to SELECT_PWM\n");
		goto end;
	}
 
end:
	mutex_unlock(&dev->lock);
 
	return ret;
}
 
static void cy8c9540a_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	int ret = 0;
	int gpio = 0;
	int port = 0, pin = 0;
	u8 val = 0;
	struct cy8c9540a *dev =
	    container_of(chip, struct cy8c9540a, pwm_chip);
	struct i2c_client *client = dev->client;
 
	if (pwm->pwm > NPWM) {
		return;
	}
 
	gpio = pwm2gpio_mapping[pwm->pwm];
	if (PWM_UNUSED == gpio) {
		dev_err(&client->dev, "pwm%d is unused\n", pwm->pwm);
		return;
	}
 
	port = cypress_get_port(gpio);
	pin = cypress_get_offs(gpio, port);
 
	mutex_lock(&dev->lock);
 
	/* Disable PWM */
	val = i2c_smbus_read_byte_data(client, REG_SELECT_PWM);
	if (val < 0) {
		dev_err(&client->dev, "can't read REG_SELECT_PWM\n");
		goto end;
	}
	val &= ~BIT((u8)pin);
	ret = i2c_smbus_write_byte_data(client, REG_SELECT_PWM, val);
	if (ret < 0) {
		dev_err(&client->dev, "can't write to SELECT_PWM\n");
	}
 
end:
	mutex_unlock(&dev->lock);
 
	return;
}
 
/* 
 * Some PWMs are unavailable on Galileo.  Prevent user from reserving them.
 */
static int cy8c9540a_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	int gpio = 0;
	struct cy8c9540a *dev =
	    container_of(chip, struct cy8c9540a, pwm_chip);
	struct i2c_client *client = dev->client;
 
	if (pwm->pwm > NPWM) {
		return -EINVAL;
	}
 
	gpio = pwm2gpio_mapping[pwm->pwm];
	if (PWM_UNUSED == gpio) {
		dev_err(&client->dev, "pwm%d unavailable\n", pwm->pwm);
		return -EINVAL;
	}
 
	return 0;
}
 
static const struct pwm_ops cy8c9540a_pwm_ops = {
	.request = cy8c9540a_pwm_request,
	.config = cy8c9540a_pwm_config,
	.enable = cy8c9540a_pwm_enable,
	.disable = cy8c9540a_pwm_disable,
};
 
/*
 * cy8c9540a_setup
 *
 * Initialise the device with default setup.
 * No need to roll back if this fails.
 */
static int cy8c9540a_setup(struct cy8c9540a *dev)
{
	int ret = 0;
	struct i2c_client *client = dev->client;
	int i = 0;
	const u8 eeprom_enable_seq[] = {0x43, 0x4D, 0x53, 0x2};
 
	/* Disable PWM, set all GPIOs as input.  */
	for (i = 0; i < NPORTS; i ++) {
		ret = i2c_smbus_write_byte_data(client, REG_PORT_SELECT, i);
		if (ret < 0) {
			dev_err(&client->dev, "can't select port %u\n", i);
			goto end;
		}
 
		ret = i2c_smbus_write_byte_data(client, REG_SELECT_PWM, 0x00);
		if (ret < 0) {
			dev_err(&client->dev, "can't write to SELECT_PWM\n");
			goto end;
		}
 
		ret = i2c_smbus_write_byte_data(client, REG_PIN_DIR, 0xff);
		if (ret < 0) {
			dev_err(&client->dev, "can't write to PIN_DIR\n");
			goto end;
		}
	}
 
	/* Cache the output registers */
	ret = i2c_smbus_read_i2c_block_data(dev->client, REG_OUTPUT_PORT0,
					    sizeof(dev->outreg_cache),
					    dev->outreg_cache);
	if (ret < 0) {
		dev_err(&client->dev, "can't cache output registers\n");
		goto end;
	}
 
	/* Set default PWM clock source.  */
	for (i = 0; i < NPWM; i ++) {
		ret = i2c_smbus_write_byte_data(client, REG_PWM_SELECT, i);
		if (ret < 0) {
			dev_err(&client->dev, "can't select pwm %u\n", i);
			goto end;
		}
 
		ret = i2c_smbus_write_byte_data(client, REG_PWM_CLK, PWM_CLK);
		if (ret < 0) {
			dev_err(&client->dev, "can't write to REG_PWM_CLK\n");
			goto end;
		}
	}
 
	/* Enable the EEPROM */
	ret = i2c_smbus_write_i2c_block_data(client, REG_ENABLE,
					     sizeof(eeprom_enable_seq),
					     eeprom_enable_seq);
	if (ret < 0) {
		dev_err(&client->dev, "can't enable EEPROM\n");
		goto end;
	}
 
end:
	return ret;
}
 
static int cy8c9540a_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct cy8c9540a *dev;
	struct gpio_chip *gc;
	int ret = 0;
	s32 dev_id = 0;
 
	if (!i2c_check_functionality(client->adapter,
					I2C_FUNC_SMBUS_I2C_BLOCK |
					I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "SMBUS Byte/Block unsupported\n");
		return -EIO;
	}
 
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&client->dev, "failed to alloc memory\n");
		return -ENOMEM;
	}
 
	dev->client = client;
 
	gc = &dev->gpio_chip;
	gc->direction_input = cy8c9540a_gpio_direction_input;
	gc->direction_output = cy8c9540a_gpio_direction_output;
	gc->get = cy8c9540a_gpio_get_value;
	gc->set = cy8c9540a_gpio_set_value;
	gc->set_drive = cy8c9540a_gpio_set_drive;
 
	gc->can_sleep = 1;
 
	gc->base = GPIO_BASE_ID;
	gc->ngpio = NGPIO;
	gc->label = client->name;
	gc->owner = THIS_MODULE;
	gc->to_irq = cy8c9540a_gpio_to_irq;
 
	mutex_init(&dev->lock);
 
	/* Whoami */
	dev_id = i2c_smbus_read_byte_data(client, REG_DEVID_STAT);
	if (dev_id < 0) {
		dev_err(&client->dev, "can't read device ID\n");
		ret = dev_id;
		goto err;
	}
	dev_info(&client->dev, "dev_id=0x%x\n", dev_id & 0xff);
	if (DEVID_FAMILY_CY8C9540A != (dev_id & DEVID_FAMILY_MASK)) {
		dev_err(&client->dev, "unknown/unsupported dev_id 0x%x\n",
			dev_id & 0xff);
		ret = -ENODEV;
		goto err;
	}
 
	ret = cy8c9540a_setup(dev);
	if (ret) {
		goto err;
	}
 
	ret = cy8c9540a_irq_setup(dev);
	if (ret) {
		goto err;
	}
	ret = gpiochip_add(&dev->gpio_chip);
	if (ret) {
		dev_err(&client->dev, "gpiochip_add failed %d\n", ret);
		goto err_irq;
	}
 
	dev->pwm_chip.dev = &client->dev;
	dev->pwm_chip.ops = &cy8c9540a_pwm_ops;
	dev->pwm_chip.base = PWM_BASE_ID;
	dev->pwm_chip.npwm = NPWM;
 
	ret = pwmchip_add(&dev->pwm_chip);
	if (ret) {
		dev_err(&client->dev, "pwmchip_add failed %d\n", ret);
		goto err_gpiochip;
	}
 
	i2c_set_clientdata(client, dev);
 
	return 0;
err_gpiochip:
	if(gpiochip_remove(&dev->gpio_chip))
		dev_warn(&client->dev, "gpiochip_remove failed\n");
err_irq:
	cy8c9540a_irq_teardown(dev);
err:
	mutex_destroy(&dev->lock);
	kfree(dev);
 
	return ret;
}
 
static int cy8c9540a_remove(struct i2c_client *client)
{
	struct cy8c9540a *dev = i2c_get_clientdata(client);
	int ret = 0;
	int err = 0;
 
	ret = pwmchip_remove(&dev->pwm_chip);
	if (ret < 0) {
		dev_err(&client->dev, "pwmchip_remove failed %d\n", ret);
		err = ret;
	}
 
	ret = gpiochip_remove(&dev->gpio_chip);
	if (ret) {
		dev_err(&client->dev, "gpiochip_remove failed %d\n", ret);
		err = ret;
	}
 
	cy8c9540a_irq_teardown(dev);
 
	mutex_destroy(&dev->lock);
	kfree(dev);
 
	return err;
}
 
static const struct i2c_device_id cy8c9540a_id[] = {
	{DRV_NAME, 0},
	{}
};
 
MODULE_DEVICE_TABLE(i2c, cy8c9540a_id);
 
static struct i2c_driver cy8c9540a_driver = {
	.driver = {
		   .name = DRV_NAME,
		   },
	.probe = cy8c9540a_probe,
	.remove = cy8c9540a_remove,
	.id_table = cy8c9540a_id,
};
