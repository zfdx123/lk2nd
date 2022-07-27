/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LK2ND_GPIO_H
#define LK2ND_GPIO_H

#include <stdint.h>
#include <bits.h>

enum gpio_devs {
	GPIOL_DEVICE_INVALID = 0,
};

/**
 * struct gpio_desc - GPIO pin descriptor.
 * @pin:   The pin number on the controller.
 * @dev:   The device number. See enum gpio_devs.
 * @flags: Internal flags set for this gpio.
 * @err:   This descriptor is invalid.
 *
 * If the descriptor is invalid, the err will be 1
 * and pin will contain the error code.
 */
struct gpio_desc {
	uint32_t pin : 16, dev : 8, flags : 7, err : 1;
};

#define FLAG_ACTIVE_LOW	BIT(0)

/**
 * gpiol_error() - Create error pin descriptor.
 * @err: Error code.
 */
static inline struct gpio_desc gpiol_error(int err)
{
	return (struct gpio_desc){ .pin = err, .err = 1 };
}

/**
 * gpiol_get() - Get gpio number from the DT definition.
 * @dtb:  pointer to the DT.
 * @node: Offset of the node containing the foo-gpios property.
 * @name: Name of the gpio in the DT node (i.e. foo for foo-gpios).
 *        name can be NULL for a simple "gpios" property.
 *
 * Returns: GPIO descriptor that can be used for GPIO operations.
 */
struct gpio_desc gpiol_get(const void *dtb, int node, const char *name);

/**
 * int gpiol_direction_input() - Configure the gpio as input.
 * @desc: GPIO descriptor.
 *
 * Returns: 0 on success or an error code.
 */
int gpiol_direction_input(struct gpio_desc desc);

/**
 * int gpiol_direction_output() - Configure the gpio as output.
 * @desc:  GPIO descriptor.
 * @value: GPIO state to be set immediately on mode change.
 *
 * Returns: 0 on success or an error code.
 */
int gpiol_direction_output(struct gpio_desc desc, int value);

/**
 * int gpiol_get_value() - Get the value of the gpio.
 * @desc:  GPIO descriptor.
 *
 * Returns: GPIO state on success or an error code.
 */
int gpiol_get_value(struct gpio_desc desc);

/**
 * int gpiol_set_value() - Set the value of the gpio.
 * @desc:  GPIO descriptor.
 * @value: GPIO state to be set.
 *
 * Returns: 0 on success or an error code.
 */
void gpiol_set_value(struct gpio_desc desc, int value);

#define PIN_CONFIG_BIAS_DISABLE		0
#define PIN_CONFIG_BIAS_PULL_UP		BIT(0)
#define PIN_CONFIG_BIAS_PULL_DOWN	BIT(1)
#define PIN_CONFIG_INPUT_ENABLE		BIT(2)
#define PIN_CONFIG_OUTPUT_ENABLE	BIT(3)

/**
 * int gpiol_set_config() - Set the GPIO pin configuration.
 * @desc:   GPIO descriptor.
 * @config: Flags to be set for the pin configuration.
 *
 * Returns: 0 on success or an error code.
 */
int gpiol_set_config(struct gpio_desc desc, uint32_t config);

#endif /* LK2ND_GPIO_H */
