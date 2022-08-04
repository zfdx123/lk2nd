/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LK2ND_BOOT_MENU_H
#define LK2ND_BOOT_MENU_H

struct lk2nd_boot_action *lk2nd_boot_menu(struct list_node *actions, struct lk2nd_boot_action *selected, int timeout);

#endif /* LK2ND_BOOT_MENU_H */
