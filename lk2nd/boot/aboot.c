// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

/**
 * aboot.c - Actions provided by aboot.
 *
 * This file provides action entries for
 * emmc boot partition, recovery and fastboot.
 */

#include <debug.h>
#include <reboot.h>

#include <lk2nd/boot.h>

#include "boot.h"


static enum lk2nd_boot_aboot_action action_aboot(void *data)
{
	return (enum lk2nd_boot_aboot_action)data;
}

static enum lk2nd_boot_aboot_action action_aboot_poweroff(void *data)
{
	shutdown_device();
	/* We should not reach this */
	return LK2ND_ABOOT_ACTION_NONE;
}

void action_aboot_register()
{
	lk2nd_boot_add_action("partition: boot", 10, action_aboot,
			(void*)LK2ND_ABOOT_ACTION_BOOT);
	lk2nd_boot_add_action("partition: recovery", -10, action_aboot,
			(void*)LK2ND_ABOOT_ACTION_RECOVERY);
	lk2nd_boot_add_action("Fastboot", -20, action_aboot,
			(void*)LK2ND_ABOOT_ACTION_FASTBOOT);
	lk2nd_boot_add_action("Power off", -30, action_aboot_poweroff, NULL);
}
