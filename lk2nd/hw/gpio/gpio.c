// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

#include <debug.h>
#include <stdint.h>
#include <string.h>
#include <libfdt.h>

#include <lk2nd/gpio.h>

#include "supplier.h"

#define LK2ND_GPIOL_PROP_LEN 32

struct gpio_desc gpiol_get(const void *dtb, int node, const char *name)
{
	char prop_name[LK2ND_GPIOL_PROP_LEN + 7] = "gpios";
	const fdt32_t *prop_val;
	struct gpio_desc pin = {0};
	uint32_t flags, config = 0;
	int len, ret;

	if (name) {
		strncpy(prop_name, name, LK2ND_GPIOL_PROP_LEN);
		strcat(prop_name, "-gpios");
	}
	prop_val = fdt_getprop(dtb, node, prop_name, &len);

	if (len < 0) {
		dprintf(CRITICAL, "gpiol_get failed: Unable to get property: %d\n", len);
		return gpiol_error(len);
	} else if (len != 3 * sizeof(*prop_val)) {
		dprintf(CRITICAL, "gpiol_get failed: gpios arrays are not supported. (len=%d)\n", len);
		return gpiol_error(-1);
	}

	pin.pin = fdt32_to_cpu(prop_val[1]);
	pin.dev = fdt32_to_cpu(prop_val[0]) & 0xff;
	flags = fdt32_to_cpu(prop_val[2]);

	/* Translate core-specific flags */
	pin.flags |= (flags & GPIO_ACTIVE_LOW ? FLAG_ACTIVE_LOW : 0);

	/* Translate initial pin configuration flags */
	config |= (flags & GPIO_PULL_UP ? PIN_CONFIG_BIAS_PULL_UP : 0);
	config |= (flags & GPIO_PULL_DOWN ? PIN_CONFIG_BIAS_PULL_DOWN : 0);
	config |= (flags & GPIO_INPUT ? PIN_CONFIG_INPUT_ENABLE : 0);
	config |= (flags & GPIO_OUTPUT ? PIN_CONFIG_OUTPUT_ENABLE : 0);

	ret = gpiol_set_config(pin, config);
	if (ret < 0) {
		dprintf(CRITICAL, "gpiol_get failed: Unable to configure gpio: %d\n", len);
		return gpiol_error(ret);
	}

	return pin;
}

int gpiol_direction_input(struct gpio_desc desc)
{
	switch (desc.dev) {
	case GPIOL_DEVICE_TLMM:
		lk2nd_gpio_tlmm_output_enable(desc.pin, false);
		break;

	case GPIOL_DEVICE_PMIC:
		return lk2nd_gpio_pmic_set_dir(desc.pin, false, false);

	case GPIOL_DEVICE_PMIC_PON:
		dprintf(SPEW, "%s: device %d does not support this action.\n", __func__, desc.dev);
		break;

	default:
		dprintf(CRITICAL, "%s: device %d is not known.\n", __func__, desc.dev);
		return 0;
	}

	return 0;
}

int gpiol_direction_output(struct gpio_desc desc, int value)
{
	switch (desc.dev) {
	case GPIOL_DEVICE_TLMM:
		lk2nd_gpio_tlmm_output_enable(desc.pin, true);
		break;

	case GPIOL_DEVICE_PMIC:
		return lk2nd_gpio_pmic_set_dir(desc.pin, true, value);

	case GPIOL_DEVICE_PMIC_PON:
		dprintf(SPEW, "%s: device %d does not support this action.\n", __func__, desc.dev);
		break;

	default:
		dprintf(CRITICAL, "%s: device %d is not known.\n", __func__, desc.dev);
		return 0;
	}

	gpiol_set_value(desc, value);

	return 0;
}

int gpiol_get_value(struct gpio_desc desc)
{
	int val = 0;

	switch (desc.dev) {
	case GPIOL_DEVICE_TLMM:
		val = lk2nd_gpio_tlmm_get(desc.pin);
		break;

	case GPIOL_DEVICE_PMIC:
		val = lk2nd_gpio_pmic_get(desc.pin);
		break;

	case GPIOL_DEVICE_PMIC_PON:
		val = lk2nd_gpio_pmic_pon_get(desc.pin);
		break;

	default:
		dprintf(CRITICAL, "%s: device %d is not known.\n", __func__, desc.dev);
		return 0;
	}

	return (desc.flags & FLAG_ACTIVE_LOW) ? !val : val;
}

void gpiol_set_value(struct gpio_desc desc, int value)
{
	if (desc.flags & FLAG_ACTIVE_LOW)
		value = !value;

	switch (desc.dev) {
	case GPIOL_DEVICE_TLMM:
		lk2nd_gpio_tlmm_set(desc.pin, !!value);
		return;

	case GPIOL_DEVICE_PMIC:
		lk2nd_gpio_pmic_set(desc.pin, !!value);
		return;

	case GPIOL_DEVICE_PMIC_PON:
		dprintf(INFO, "%s: device %d does not support this action.\n", __func__, desc.dev);
		break;

	default:
		dprintf(CRITICAL, "%s: device %d is not known.\n", __func__, desc.dev);
		return;
	}
}

int gpiol_set_config(struct gpio_desc desc, uint32_t config)
{
	switch (desc.dev) {
	case GPIOL_DEVICE_TLMM:
		return lk2nd_gpio_tlmm_config(desc.pin, config);

	case GPIOL_DEVICE_PMIC:
		return lk2nd_gpio_pmic_config(desc.pin, config);

	case GPIOL_DEVICE_PMIC_PON:
		dprintf(SPEW, "%s: device %d does not support this action.\n", __func__, desc.dev);
		break;

	default:
		dprintf(CRITICAL, "%s: device %d is not known.\n", __func__, desc.dev);
		return 0;
	}

	return 0;
}
