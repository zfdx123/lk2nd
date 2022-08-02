// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

#include <stdint.h>
#include <bits.h>
#include <debug.h>
#include <pm8x41.h>
#include <pm8x41_hw.h>

#include <lk2nd/gpio.h>

#include "supplier.h"

#define PMIC_GPIO_NUM_PIN(num)   BITS_SHIFT(num, 7, 0)
#define PMIC_GPIO_NUM_SID(num)   BITS_SHIFT(num, 15, 8)

int lk2nd_gpio_pmic_config(uint32_t num, int flags)
{
	struct pm8x41_gpio pm_gpio = {
		.function  = 0,
		.vin_sel   = 2,
	};

	if (flags & PIN_CONFIG_BIAS_PULL_UP)
		pm_gpio.pull = PM_GPIO_PULL_UP_1_5;
	else if (flags & PIN_CONFIG_BIAS_PULL_DOWN)
		pm_gpio.pull = PM_GPIO_PULLDOWN_10;
	else
		pm_gpio.pull = PM_GPIO_NO_PULL;

	if (flags & PIN_CONFIG_OUTPUT_ENABLE)
		pm_gpio.direction = PM_GPIO_DIR_OUT;
	else
		pm_gpio.direction = PM_GPIO_DIR_IN;

	return pm8x41_gpio_config_sid(PMIC_GPIO_NUM_SID(num), PMIC_GPIO_NUM_PIN(num), &pm_gpio);
}

#define OUT_LOW		0
#define OUT_HIGH	1
#define DIGITAL_INPUT	BITS_SHIFT(0, 6, 4)
#define DIGITAL_OUTPUT	BITS_SHIFT(1, 6, 4)

int lk2nd_gpio_pmic_set_dir(uint32_t num, bool oe, bool on)
{
	uint32_t gpio_base = GPIO_N_PERIPHERAL_BASE(PMIC_GPIO_NUM_PIN(num));
	uint8_t val = 0;

	gpio_base &= 0x0ffff;
	gpio_base |= (PMIC_GPIO_NUM_SID(num) << 16);

	if (oe)
		val = DIGITAL_OUTPUT | (on ? OUT_HIGH : OUT_LOW);

	REG_WRITE(gpio_base + GPIO_MODE_CTL, val);

	return 0;
}

void lk2nd_gpio_pmic_set(uint32_t num, bool on)
{
	pm8x41_gpio_set_sid(PMIC_GPIO_NUM_SID(num), PMIC_GPIO_NUM_PIN(num), on);
}

int lk2nd_gpio_pmic_get(uint32_t num)
{
	uint8_t status;

	pm8x41_gpio_get_sid(PMIC_GPIO_NUM_SID(num), PMIC_GPIO_NUM_PIN(num), &status);

	return status;
}

int lk2nd_gpio_pmic_pon_get(uint32_t num)
{
	switch (PMIC_GPIO_NUM_PIN(num)) {
		case GPIO_PMIC_PWRKEY:
			return pm8x41_get_pwrkey_is_pressed();

		case GPIO_PMIC_RESIN:
			return pm8x41_resin_status();
	}

	dprintf(CRITICAL, "gpio_pm8x41: PON key %d is unknown.", PMIC_GPIO_NUM_PIN(num));

	return 0;
}
