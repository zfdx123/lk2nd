// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

/**
 * list.c - The action list handling code.
 */

#include <stdlib.h>
#include <debug.h>
#include <list.h>

#include <lk2nd/boot.h>

#include "boot.h"

/**
 * lk2nd_boot_add_action() - Register a new boot action.
 * @actions_list:  A list that this action shall be inserted to.
 * @name:          Label for the entry.
 * @priority:      Entry priority.
 * @action:        A function implementing the action.
 * @data:          Data for that function.
 *
 * The new action will be inserted in the order of priority.
 */
void lk2nd_boot_add_action(struct list_node *actions_list, char *name, int priority,
		enum lk2nd_boot_aboot_action (*action)(void *data), void *data)
{
	struct lk2nd_boot_action *lp, *act = malloc(sizeof(*act));

	strncpy(act->name, name, LK2ND_BOOT_MAX_NAME_LEN-1);
	act->priority = priority;
	act->action = action;
	act->data = data;

	lp = list_peek_head_type(actions_list, struct lk2nd_boot_action, node);


	while (lp && lp->priority > priority)
		lp = list_next_type(actions_list, &lp->node, struct lk2nd_boot_action, node);

	if (lp)
		list_add_before(&lp->node, &act->node);
	else
		list_add_tail(actions_list, &act->node);
}

/**
 * lk2nd_boot_pick_action() - Print out the action list.
 * @actions_list:  A list of actions to print.
 */
void lk2nd_boot_print_actions(struct list_node *actions_list)
{
	struct lk2nd_boot_action *entry;

	dprintf(INFO, " | %-32s | Prio |\n", "Name");
	list_for_every_entry(actions_list, entry, struct lk2nd_boot_action, node) {
		dprintf(INFO, " | %-32s | %4d |\n", entry->name, entry->priority);
	}
}

/**
 * lk2nd_boot_pick_action() - Examine the list and run an option.
 * @actions_list:  A list of actions to inspect.
 * @interactive:   The user shall be prompted with the choice.
 */
enum lk2nd_boot_aboot_action lk2nd_boot_pick_action(struct list_node *actions_list, bool interactive)
{
	struct lk2nd_boot_action *best = NULL;

	if (interactive)
		return lk2nd_boot_menu(actions_list);
	
	best = list_peek_head_type(actions_list, struct lk2nd_boot_action, node);
	if (best) {
		dprintf(INFO, "boot: Picked %s (%d)\n", best->name, best->priority);
		return best->action(best->data);
	}

	dprintf(CRITICAL, "boot: No action was performed, requesting fastboot\n");
	return LK2ND_ABOOT_ACTION_FASTBOOT;
}

static enum lk2nd_boot_aboot_action action_list(void *data)
{
	return lk2nd_boot_pick_action((struct list_node*)data, true);
}

/**
 * action_list_register() - Insert a submenu with a given list of actions.
 * @actions_list:  A list that this action shall be inserted to.
 * @name:          Label for the entry.
 * @priority:      Entry priority.
 * @child:         A list of actions the inserted menu contains.
 */
void action_list_register(struct list_node *actions_list, char *name, int priority, struct list_node *child)
{
	lk2nd_boot_add_action(actions_list, name, priority, action_list, child);
}
