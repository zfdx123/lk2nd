// SPDX-License-Identifier: GPL-2.0+
// © 2019 Mis012

#include <stdio.h>
#include <stdlib.h>
#include <lib/fs.h>
#include <string.h>
#include <err.h>

#include "config.h"
#include "fs_util.h"

#define ENTRIES_DIR "/boot/lk2nd/entries"
#define GLOBAL_CONFIG_FILE "/boot/lk2nd/lk2nd.conf"

struct env_struct {
	char *env;
	char *value;
};

static struct env_struct envs[] = {
	{"$DEVICE", "samsung-a3u-eur"},
	{"$SOC", "msm8916"},
};

int compute_new_len(char *src, struct env_struct *envs, int num_envs) {
	int new_len = strlen(src);
	char *c;

	for(int i = 0; i < num_envs; i++) {
		char *cursor = src;
		while(c = strstr(cursor, envs[i].env)) {
			new_len = new_len - strlen(envs[i].env) + strlen(envs[i].value);
			cursor = c + strlen(envs[i].env);
		}
	}

	return new_len;
}

void strreplace(char *dest, char *src, char *placeholder, char *value)
{
	char *start = src;
	char *cursor = dest;
	char *c;
	while (c = strstr(start, placeholder)) {
		strncpy(cursor, start, c-start);
		cursor += c-start;
		strcpy(cursor, value);
		cursor += strlen(value);
		c += strlen(placeholder);
		start = c;
	}
	strcpy(cursor, start);
}

void strreplace_bulk(char *dest, char *src, int max_len, struct env_struct *envs, int num_envs)
{
	char *temp = malloc(max_len + 1);

	strcpy(temp, src);

	for(int i = 0; i < num_envs; i++) {
		strreplace(dest, temp, envs[i].env, envs[i].value);
		strcpy(temp, dest);
	}

	free(temp);
}

int config_parse_option(char **_dest, const char *option, const char *buffer) {
	char *temp = strstr(buffer, option);
	if(!temp)
		return -1;
	
	temp += strlen(option);
	while (*temp == ' ')
		temp++;

	char *newline = strchr(temp, '\n');
	if(newline)
		*newline = '\0';

	int new_len = compute_new_len(temp, envs, ARRAY_SIZE(envs));
	char *dest = malloc(new_len + 1);
	if(!dest)
		return ERR_NO_MEMORY;

	strreplace_bulk(dest, temp, new_len + 1, envs, ARRAY_SIZE(envs));
	*_dest = dest;

	//restore the buffer
	*newline = '\n';

	return 0;
}

int parse_boot_entry_file(struct boot_entry *entry, struct dirent ent) {
	int ret;
	filehandle *entry_file_handle = NULL;
	unsigned char *buf;

	char *path = malloc(strlen(ent.name) + strlen(ENTRIES_DIR"/") + 1);
	if(!path)
		return ERR_NO_MEMORY;
	strcpy(path, ENTRIES_DIR"/");
	strcat(path, ent.name);

	off_t entry_file_size = fs_get_file_size(path);
	buf = malloc(entry_file_size + 1);
	if(!buf)
		return ERR_NO_MEMORY;

	ret = fs_open_file(path, &entry_file_handle);
	printf("fs_open_file ret: %d\n", ret);
	if(ret < 0) {
		return ret;
	}

	ret = fs_read_file(entry_file_handle, buf, 0, entry_file_size);
	printf("fs_read_file ret: %d\n", ret);
	if(ret < 0) {
		free(buf);
		return ret;
	}

	ret = fs_close_file(entry_file_handle);
	printf("fs_close_file ret: %d\n", ret);
	if(ret) {
		free(buf);
		return ret;
	}

	buf[entry_file_size] = '\0';
	
	ret = config_parse_option(&entry->title, "title", (const char *)buf);
	if(ret < 0) {
		printf("SYNTAX ERROR: entry \"%s\" - no option 'title'\n", path);
		free(buf);
		return ret;
	}

	ret = config_parse_option(&entry->linux, "linux", (const char *)buf);
	if(ret < 0) {
		printf("SYNTAX ERROR: entry \"%s\" - no option 'linux'\n", path);
		free(buf);
		return ret;
	}

	ret = config_parse_option(&entry->initrd, "initrd", (const char *)buf);
	if(ret < 0) {
		printf("SYNTAX ERROR: entry \"%s\" - no option 'initrd'\n", path);
		free(buf);
		return ret;
	}

	ret = config_parse_option(&entry->dtb, "dtb", (const char *)buf);
	if(ret < 0) {
		printf("SYNTAX ERROR: entry \"%s\" - no option 'dtb'\n", path);
		free(buf);
		return ret;
	}

	ret = config_parse_option(&entry->options, "options", (const char *)buf);
	if(ret < 0) {
		printf("SYNTAX ERROR: entry \"%s\" - no option 'options'\n", path);
		free(buf);
		return ret;
	}

	free(buf);

	entry->error = false;

	return 0;
}

int parse_boot_entries(const char *block_device, struct boot_entry **_entry_list) {
	int entry_count;

	int ret;

	struct boot_entry *entry_list;

	ret = fs_mount("/boot", "ext2", block_device); //system
	printf("fs_mount ret: %d\n", ret);
	if(ret) {
		return ret;
	}

	ret = entry_count = dir_count_entries(ENTRIES_DIR);
	if (ret < 0) {
		entry_count = 0;
		fs_unmount("/boot");
		return ret;
	}

	if(entry_count == 0) {
		fs_unmount("/boot");
		printf("NOTICE: no boot entries found\n");
		return 0;
	}


	entry_list = malloc(entry_count * sizeof(struct boot_entry));
	if(!entry_list) {
		entry_count = 0;
		fs_unmount("/boot");
		return ERR_NO_MEMORY;
	}

	struct dirhandle *dirhandle;
	struct dirent ent;

	ret = fs_open_dir(ENTRIES_DIR, &dirhandle);
	if(ret < 0) {
		printf("fs_open_dir ret: %d\n", ret);
		entry_count = 0;
		fs_unmount("/boot");
		return ret;
	}

	int i = 0;
	while(fs_read_dir(dirhandle, &ent) >= 0) {
		struct boot_entry *entry = entry_list + i;
		ret = parse_boot_entry_file(entry, ent);
		if(ret < 0) {
			printf("error passing entry: %s\n", ent.name);
			entry->error = true;
			entry->title = "SYNTAX ERROR";
		}
		entry->block_device = block_device;
		i++;
	}
	fs_close_dir(dirhandle);

	fs_unmount("/boot");
	
	*_entry_list = entry_list;
	
	return entry_count;
}

int parse_global_config(const char *block_device, struct global_config *global_config) {
	int ret;
	filehandle *global_config_file_handle = NULL;
	unsigned char *buf;

	ret = fs_mount("/boot", "ext2", block_device); //system
	printf("fs_mount ret: %d\n", ret);
	if(ret) {
		return ret;
	}

	off_t entry_file_size = fs_get_file_size(GLOBAL_CONFIG_FILE);
	if(!entry_file_size) {
		fs_unmount("/boot");
		printf("NOTICE: can't stat lk2nd.conf, does it exist?\n");
		return 0;
	}

	buf = malloc(entry_file_size + 1);
	if(!buf) {
		fs_unmount("/boot");
		return ERR_NO_MEMORY;
	}

	ret = fs_open_file(GLOBAL_CONFIG_FILE, &global_config_file_handle);
	printf("fs_open_file ret: %d\n", ret);
	if(ret < 0) {
		fs_unmount("/boot");
		return ret;
	}

	ret = fs_read_file(global_config_file_handle, buf, 0, entry_file_size);
	printf("fs_read_file ret: %d\n", ret);
	if(ret < 0) {
		free(buf);
		fs_unmount("/boot");
		return ret;
	}

	ret = fs_close_file(global_config_file_handle);
	printf("fs_close_file ret: %d\n", ret);
	if(ret) {
		free(buf);
		fs_unmount("/boot");
		return ret;
	}

	ret = config_parse_option(&global_config->default_entry_title, "default", (const char *)buf);
	if(ret < 0) {
		printf("WARNING: lk2nd.conf: - no option 'default'\n");

		global_config->default_entry_title = NULL;
	}

	ret = config_parse_option(&global_config->charger_entry_title, "charger", (const char *)buf);
	if(ret < 0) {
		printf("NOTICE: lk2nd.conf: - no option 'charger'\n");

		global_config->charger_entry_title = NULL;
	}

	char *timeout = NULL;
	ret = config_parse_option(&timeout, "timeout", (const char *)buf);
	if(ret < 0) {
		printf("NOTICE: lk2nd.conf - no option 'timeout', will use 0\n");

		timeout = NULL;
	}

	if(timeout)
		global_config->timeout = atoi(timeout);
	else
		global_config->timeout = 0;

	fs_unmount("/boot");

	return 0;
}

// as an exception, this function is not for parsing, but for using the parsed information

struct boot_entry *get_entry_by_title(struct boot_entry * entry_list, int num_of_boot_entries, char *title) {
	int i;
	for (i = 0; i < num_of_boot_entries; i++) {
		if((entry_list + i)->error)
			continue;
		if(!strcmp(title, (entry_list + i)->title))
			return entry_list + i;
	}

	printf("WARNING: no boot entry with title `%s`\n", title);

	return NULL;
}
