// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

#include <stdlib.h>
#include <debug.h>
#include <list.h>
#include <lib/bio.h>
#include <lib/fs.h>

#include <lk2nd/boot.h>

#include "boot.h"

/* Top level list */
struct list_node actions_list = LIST_INITIAL_VALUE(actions_list);

/**
 * lk2nd_boot_do_action() - Boot the OS.
 *
 * This is a top-level boot application method that surveys all
 * possible boot targets, possibly shows the selection menu and
 * performs the boot of the selected option. It can also instruct
 * aboot to some action if needed.
 *
 * Return: The method never returns if the boot action succeds.
 * If the selected boot action returns, it's action code will be
 * returned to correct the aboot behavior. If no action fires,
 * LK2ND_ABOOT_ACTION_FASTBOOT is returned.
 */
enum lk2nd_boot_aboot_action lk2nd_boot_do_action(void)
{
	struct bdev_struct *bdevs = bio_get_bdevs();
	char mountpoint[16];
	bdev_t *bdev;
	int ret;

	dprintf(INFO, "boot: Trying to boot...\n");

	list_for_every_entry(&bdevs->list, bdev, bdev_t, node) {

		/* Skip top level block devices. */
		if (!bdev->is_subdev)
			continue;

		snprintf(mountpoint, sizeof(mountpoint), "/%s", bdev->name);
		ret = fs_mount(mountpoint, "ext2", bdev->name);
		if (ret < 0)
			continue;

		dprintf(INFO, "Scanning %s ...\n", bdev->name);

		dprintf(INFO, "%s\n", mountpoint);
		print_file_tree(mountpoint, " ");

		action_abootimg_register(&actions_list, mountpoint);
	}

	/* aboot actions go last in the list. */
	action_aboot_register(&actions_list);

	dprintf(INFO, "boot: Available entries:\n");
	lk2nd_boot_print_actions(&actions_list);

	return lk2nd_boot_pick_action(&actions_list, lk2nd_boot_pressed_key());
}

/**
 * lk2nd_boot_init() - Prepare lk2nd boot.
 *
 * This method is called early in aboot to do any preparations
 * or dump some debug information to the log.
 */
void lk2nd_boot_init(void)
{
	dprintf(INFO, "boot: Init\n");
	lk2nd_boot_dump_devices();
}
