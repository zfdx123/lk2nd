/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef GPIO_SUPPLIER_H
#define GPIO_SUPPLIER_H

#include <stdint.h>

/* tlmm.c */
int lk2nd_gpio_tlmm_config(uint32_t num, int flags);
void lk2nd_gpio_tlmm_output_enable(uint32_t num, bool oe);
void lk2nd_gpio_tlmm_set(uint32_t num, bool on);
int lk2nd_gpio_tlmm_get(uint32_t num);

#endif /* GPIO_SUPPLIER_H */
