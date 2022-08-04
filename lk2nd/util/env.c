// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022 Nikita Travkin <nikita@trvn.ru> */

#include <debug.h>
#include <list.h>
#include <string.h>
#include <stdlib.h>

#include <lk2nd/env.h>

#define LK2ND_ENV_NAME_LEN 32
#define LK2ND_ENV_VALUE_LEN 64

/* 
 * NOTE: The array MUST be sorted by name.
 * The devault variables will be searched through with bsearch()
 */
static const struct def_env {
	char *name;
	char *value;
} default_vars[] = {
	{ "menu_timeout", "0" },
};

static int env_strcmp(const void *name, const void *var)
{	
	return strcmp((const char*)name, ((const struct def_env*)var)->name);
}

static char *lk2nd_getenv_default(const char *name)
{
	struct def_env *var = bsearch(name, default_vars, ARRAY_SIZE(default_vars), sizeof(struct def_env), env_strcmp);

	if (var)
		return var->value;

	return NULL;
}

struct env_var {
	struct list_node node;
	char name[LK2ND_ENV_NAME_LEN];
	char value[LK2ND_ENV_VALUE_LEN];
};

struct list_node environment = LIST_INITIAL_VALUE(environment);

static struct env_var *find_var(const char *name)
{
	struct env_var *var;

	list_for_every_entry(&environment, var, struct env_var, node) {
		if (!strcmp(var->name, name))
			return var;
	}

	return NULL;
}

int lk2nd_setenv(const char *name, const char *value)
{
	struct env_var *var = find_var(name);

	if (var) {
		strncpy(var->value, value, LK2ND_ENV_VALUE_LEN-1);
		return 0;
	}

	var = malloc(sizeof(*var));

	strncpy(var->name, name, LK2ND_ENV_NAME_LEN-1);
	strncpy(var->value, value, LK2ND_ENV_VALUE_LEN-1);

	list_add_head(&environment, &var->node);

	return 0;
}

char *lk2nd_getenv(const char *name)
{
	struct env_var *var = find_var(name);

	if (var)
		return var->value;

	return lk2nd_getenv_default(name);
}
