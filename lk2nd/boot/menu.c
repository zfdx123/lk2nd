// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

/**
 * menu.c - The UI implementation for the action list.
 */

#include <stdlib.h>
#include <config.h>
#include <debug.h>
#include <list.h>
#include <dev/fbcon.h>
#include <platform/timer.h>

#include <lk2nd/device/keys.h>
#include <lk2nd/boot.h>

#include "boot.h"

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

static uint32_t wait_key(void)
{
	unsigned int i;
	uint32_t keycode = 0;


	while (!keycode) {
		for (i = 0; i < ARRAY_SIZE(published_keys); ++i)
			if (lk2nd_keys_pressed(published_keys[i]))
				keycode = published_keys[i];

		mdelay(1);
	}

	while (lk2nd_keys_pressed(keycode))
		mdelay(1);

	return keycode;
}

/**
 * lk2nd_boot_menu() - Prompt user with the list of actions.
 */
enum lk2nd_boot_aboot_action lk2nd_boot_menu(struct list_node *actions_list)
{
	struct lk2nd_boot_action *entry, *selected;
	unsigned int scale = 2;
	char msg[128];

	selected = list_peek_head_type(actions_list, struct lk2nd_boot_action, node);

	dprintf(INFO, "boot_menu: Entry\n");

	while (true) {
		fbcon_clear();
		display_fbcon_menu_message("boot menu\n\n", FBCON_RED_MSG, scale, 0);

		display_fbcon_menu_message("Press POWER to select\n\n", FBCON_SUBTITLE_MSG, scale, 0);

		list_for_every_entry(actions_list, entry, struct lk2nd_boot_action, node) {
			snprintf(msg, sizeof(msg), "%s %s\n", (entry==selected ? "->" : "* "), entry->name);
			display_fbcon_menu_message(msg, (entry==selected ? FBCON_COMMON_MSG : FBCON_SUBTITLE_MSG), scale, 0);
		}

		switch (wait_key()) {
		case KEY_POWER:
			return selected->action(selected->data);

		case KEY_VOLUMEUP:
			selected = list_prev_wrap_type(actions_list, &selected->node, struct lk2nd_boot_action, node);
			break;

		case KEY_VOLUMEDOWN:
			selected = list_next_wrap_type(actions_list, &selected->node, struct lk2nd_boot_action, node);
			break;
		}
	}
}


