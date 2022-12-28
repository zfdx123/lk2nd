/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LK2ND_BOOT_BOOT_H
#define LK2ND_BOOT_BOOT_H

#include <list.h>
#include <string.h>

#include <lk2nd/boot.h>

#define LK2ND_BOOT_MAX_NAME_LEN 32

/**
 * struct lk2nd_boot_action - Boot action.
 * @name: Entry name.
 * @priority: Priority of the entry.
 *            Negative values will never be auto-picked.
 * @action:   Function that will attempt the boot.
 * @data:     Action-specific data passed to the action.
 */
struct lk2nd_boot_action {
	struct list_node node;
	char name[LK2ND_BOOT_MAX_NAME_LEN];
	int priority;
	enum lk2nd_boot_aboot_action (*action)(void *data);
	void *data;
};

/* util.c */
void lk2nd_boot_dump_devices(void);
void print_file_tree(char *root, char *prefix);
bool file_extension_is(char *name, char *extension);

/* list.h */
void lk2nd_boot_add_action(struct list_node *actions_list, char *name, int priority,
		enum lk2nd_boot_aboot_action (*action)(void *data), void *data);
void lk2nd_boot_print_actions(struct list_node *actions_list);
enum lk2nd_boot_aboot_action lk2nd_boot_pick_action(struct list_node *actions_list, bool interactive);
void action_list_register(struct list_node *actions_list, char *name, int priority, struct list_node *child);

/* menu.c */
uint16_t lk2nd_boot_pressed_key(void);
enum lk2nd_boot_aboot_action lk2nd_boot_menu(struct list_node *actions_list);

/* aboot.c */
void action_aboot_register(struct list_node *actions_list);

/* abootimg.c */
void action_abootimg_register(struct list_node *actions_list, char *root);

#endif /* LK2ND_BOOT_BOOT_H */
