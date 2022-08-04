// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

#include <stdlib.h>
#include <config.h>
#include <debug.h>
#include <list.h>
#include <dev/fbcon.h>
#include <platform/timer.h>

#include <lk2nd/device/keys.h>
#include <lk2nd/env.h>
#include <lk2nd/boot.h>

#include "menu.h"
#include "../boot.h"

static void display_fbcon_menu_message(char *str, unsigned type,
	unsigned scale_factor, int y_start)
{
	while(*str != 0) {
		fbcon_putc_factor(*str++, type, scale_factor, y_start);
		y_start = 0;
	}
}

static const uint16_t published_keys[] = {
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_POWER,
	KEY_HOME,
};

static uint32_t wait_key(int timeout)
{
	unsigned int i;
	uint32_t keycode = 0;

	
	while (!keycode && timeout) {
		for (i = 0; i < ARRAY_SIZE(published_keys); ++i)
			if (lk2nd_keys_pressed(published_keys[i]))
				keycode = published_keys[i];

		if (timeout >= 0) {
			mdelay(1);
			timeout--;
		}
	}

	while (lk2nd_keys_pressed(keycode))
		mdelay(1);

	return keycode;
}

/**
 * lk2nd_boot_menu() - Prompt the user to pick a boot entry.
 * @actions:  A list of all possible actions to show to the user.
 * @selected: The action to be selected initially.
 * @timeout:  A timeout in seconds.
 *
 * When timeout passes with no action, the pre-selected action
 * will be returned. If timeout is < 0, the menu will stay forever.
 *
 * Returns: Pointer to the selected action.
 */
struct lk2nd_boot_action *lk2nd_boot_menu(struct list_node *actions, struct lk2nd_boot_action *selected, int timeout)
{
	struct lk2nd_boot_action *entry;
	unsigned int scale = 2;
	char msg[128];

	dprintf(INFO, "boot_menu: Entry (timeout=%d)\n", timeout);

	while (timeout) {
		fbcon_clear();
		display_fbcon_menu_message("boot menu\n\n", FBCON_RED_MSG, scale, 0);

		if (timeout >= 0) {
			snprintf(msg, sizeof(msg), "Boot in %d sec...\n\n", timeout);
			display_fbcon_menu_message(msg, FBCON_SUBTITLE_MSG, scale, 0);
		}
		else {
			display_fbcon_menu_message("Press POWER to select\n\n", FBCON_SUBTITLE_MSG, scale, 0);
		}

		list_for_every_entry(actions, entry, struct lk2nd_boot_action, node) {
			snprintf(msg, sizeof(msg), "%s %s\n", (entry==selected ? "->" : "* "), entry->name);
			display_fbcon_menu_message(msg, (entry==selected ? FBCON_COMMON_MSG : FBCON_SUBTITLE_MSG), scale, 0);
		}

		switch (wait_key(timeout >= 0 ? 1000 : -1)) {
		case KEY_POWER:
			timeout = -1;
			return selected;

		case KEY_VOLUMEUP:
			timeout = -1;
			selected = list_prev_wrap_type(actions, &selected->node, struct lk2nd_boot_action, node);
			break;

		case KEY_VOLUMEDOWN:
			timeout = -1;
			selected = list_next_wrap_type(actions, &selected->node, struct lk2nd_boot_action, node);
			break;
		case 0:
			timeout--;
		}
	}

	return selected;
}
