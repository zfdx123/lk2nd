/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LK2ND_ENV_H
#define LK2ND_ENV_H

/**
 * lk2nd_setenv() - Set an environment variable
 * @name:  Envvar name.
 * @value: A value to set.
 *
 * The method will append the environment with a new variable,
 * overwriting the value if the name already exists.
 *
 * Return: 0 or success or an error code.
 */
int lk2nd_setenv(const char *name, const char *value);

/**
 * lk2nd_getenv() - Get an environment variable
 * @name:  Envvar name.
 *
 * Method will search the environment for the given name.
 * A set of default predefined variables may be used if they were
 * never overwritten.
 *
 * User must take care to not write to the returned string pointer.
 *
 * Return: Pointer to the value string or NULL if it wasn't found.
 */
char *lk2nd_getenv(const char *name);

#endif /* LK2ND_ENV_H */
