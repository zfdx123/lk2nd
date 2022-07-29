// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

#include <bits.h>
#include <debug.h>
#include <reg.h>
#include <platform/iomap.h>
#include <stdint.h>

#include <lk2nd/gpio.h>

#include "supplier.h"

enum tlmm_pull {
	TLMM_NO_PULL,
	TLMM_PULL_DOWN,
	TLMM_KEEPER,
	TLMM_PULL_UP,
};

enum tlmm_drv_str {
	TLMM_DRV_2MA,
	TLMM_DRV_4M,
	TLMM_DRV_6MA,
	TLMM_DRV_8MA,
	TLMM_DRV_10MA,
	TLMM_DRV_12MA,
	TLMM_DRV_14MA,
	TLMM_DRV_16MA,
};

#define TLMM_IN		BIT(0)
#define TLMM_OUT	BIT(1)

#define TLMM_CFG(pull, func, drv_str, oe, hihys) \
	((pull) | (func) << 2 | (drv_str) << 6 | (oe) << 9 | (hihys) << 10)

int lk2nd_gpio_tlmm_config(uint32_t num, int flags)
{
	uint32_t val, pull, oe = 0;

	if (flags & PIN_CONFIG_BIAS_PULL_UP)
		pull = TLMM_PULL_UP;
	else if (flags & PIN_CONFIG_BIAS_PULL_DOWN)
		pull = TLMM_PULL_DOWN;
	else
		pull = TLMM_NO_PULL;

	oe = !!(flags & PIN_CONFIG_OUTPUT_ENABLE);

	val = TLMM_CFG(pull, 0, TLMM_DRV_2MA, oe, 0);

	writel(val, GPIO_CONFIG_ADDR(num));

	return 0;
}

void lk2nd_gpio_tlmm_output_enable(uint32_t num, bool oe)
{
	uint32_t val = readl(GPIO_CONFIG_ADDR(num));

	writel((val & ~(1 << 9)) | (oe << 9), GPIO_CONFIG_ADDR(num));
}

void lk2nd_gpio_tlmm_set(uint32_t num, bool on)
{
	writel(on << 1, GPIO_IN_OUT_ADDR(num));
}

int lk2nd_gpio_tlmm_get(uint32_t num)
{
	return readl(GPIO_IN_OUT_ADDR(num)) & TLMM_IN;
}
