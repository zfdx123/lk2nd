#ifndef __REBOOT_BOOT_ENTRY_H
#define __REBOOT_BOOT_ENTRY_H

struct global_config {
	char *default_entry_title;
	char *charger_entry_title;
	struct boot_entry *default_entry;
	int timeout;
};

struct boot_entry {
	char *title;
	char *linux;
	char *initrd;
	char *dtb;
	char *options;
	char *block_device;
	bool error;
};

int parse_boot_entries(const char *block_device, struct boot_entry **entry_list);

int parse_global_config(const char *block_device, struct global_config *global_config);

struct boot_entry *get_entry_by_title(struct boot_entry * entry_list, int num_of_boot_entries, char *title);

#endif
