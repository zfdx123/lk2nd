// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

#include <debug.h>
#include <stdint.h>
#include <string.h>
#include <libfdt.h>

#include <lk2nd/gpio.h>

#define LK2ND_GPIOL_PROP_LEN 32

struct gpio_desc gpiol_get(const void *dtb, int node, const char *name)
{
	char prop_name[LK2ND_GPIOL_PROP_LEN + 7] = "gpios";
	const fdt32_t *prop_val;
	struct gpio_desc pin;
	uint32_t flags;
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
	pin.flags = BITS_SHIFT(flags, 30, 24);

	/* First 3 bytes are initial configuration */
	ret = gpiol_set_config(pin, BITS_SHIFT(flags, 23, 0));
	if (ret < 0) {
		dprintf(CRITICAL, "gpiol_get failed: Unable to configure gpio: %d\n", len);
		return gpiol_error(ret);
	}

	return pin;
}

int gpiol_direction_input(struct gpio_desc desc)
{
	switch (desc.dev) {
	default:
		dprintf(CRITICAL, "%s: device %d is not known.\n", __func__, desc.dev);
		return 0;
	}

	return 0;
}

int gpiol_direction_output(struct gpio_desc desc, int value)
{
	switch (desc.dev) {
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
	default:
		dprintf(CRITICAL, "%s: device %d is not known.\n", __func__, desc.dev);
		return;
	}
}

int gpiol_set_config(struct gpio_desc desc, uint32_t config)
{
	switch (desc.dev) {
	default:
		dprintf(CRITICAL, "%s: device %d is not known.\n", __func__, desc.dev);
		return 0;
	}

	return 0;
}
