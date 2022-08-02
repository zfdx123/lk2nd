// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

#include <stdint.h>
#include <bits.h>
#include <debug.h>
#include <dev/pm8921.h>

#include <lk2nd/gpio.h>

#include "supplier.h"

#define PMIC_GPIO_NUM_PIN(num)   BITS_SHIFT(num, 7, 0)
#define PMIC_GPIO_NUM_SID(num)   BITS_SHIFT(num, 15, 8)

int lk2nd_gpio_pmic_config(uint32_t num, int flags)
{
	struct pm8921_gpio pm_gpio = {
		.function  = 0,
		.vin_sel   = 2,
	};

	if (flags & PIN_CONFIG_BIAS_PULL_UP)
		pm_gpio.pull = PM_GPIO_PULL_UP_1_5;
	else if (flags & PIN_CONFIG_BIAS_PULL_DOWN)
		pm_gpio.pull = PM_GPIO_PULL_DN;
	else
		pm_gpio.pull = PM_GPIO_PULL_NO;

	if (flags & PIN_CONFIG_OUTPUT_ENABLE)
		pm_gpio.direction = PM_GPIO_DIR_OUT;
	else
		pm_gpio.direction = PM_GPIO_DIR_IN;

	return pm8921_gpio_config(PMIC_GPIO_NUM_PIN(num), &pm_gpio);
}

int lk2nd_gpio_pmic_set_dir(uint32_t num, bool oe, bool on)
{
	return 0;
}

void lk2nd_gpio_pmic_set(uint32_t num, bool on) { }

int lk2nd_gpio_pmic_get(uint32_t num)
{
	uint8_t status;

	pm8921_gpio_get(PMIC_GPIO_NUM_PIN(num), &status);

	return status;
}

int lk2nd_gpio_pmic_pon_get(uint32_t num)
{
	uint8_t status = 0;
	
	switch (PMIC_GPIO_NUM_PIN(num)) {
		case GPIO_PMIC_PWRKEY:
			pm8921_pwrkey_status(&status);
		return status;
	}

	dprintf(CRITICAL, "gpio_pm8921: PON key %d is unknown.", PMIC_GPIO_NUM_PIN(num));

	return 0;
}

