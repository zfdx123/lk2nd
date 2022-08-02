/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef DT_BINDINGS_GPIO_H
#define DT_BINDINGS_GPIO_H

/*
 * Macro to set a phandle like 'GIO\x00'.
 * Implementation will derive driver ID from this
 */
#define GPIO_DEV_PHANDLE(dev) (0x47494f00 | ((dev) & 0xff))

#define GPIOL_DEVICE_INVALID	0
#define GPIOL_DEVICE_TLMM	1
#define GPIOL_DEVICE_PMIC	2
#define GPIOL_DEVICE_PMIC_PON	3

/* GPIO configuration flags */
#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define GPIO_ACTIVE_HIGH	0
#define GPIO_ACTIVE_LOW		BIT(0)

#define GPIO_BIAS_DISABLE	0
#define GPIO_PULL_UP		BIT(4)
#define GPIO_PULL_DOWN		BIT(5)
#define GPIO_INPUT		BIT(6)
#define GPIO_OUTPUT		BIT(7)

/* device-specific definitions */

#define GPIO_PMIC_NUM(num, sid) (((num) & 0xff) | ((sid) & 0xff) << 8)

#define GPIO_PMIC_PWRKEY	1
#define GPIO_PMIC_RESIN		2

#endif /* DT_BINDINGS_GPIO_H */
