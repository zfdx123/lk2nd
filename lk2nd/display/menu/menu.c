// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023 Nikita Travkin <nikita@trvn.ru> */

#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <compiler.h>
#include <debug.h>
#include <platform.h>
#include <dev/fbcon.h>
#include <sys/types.h>
#include <kernel/thread.h>

#include <lk2nd/util/minmax.h>
#include <lk2nd/device/keys.h>
#include <lk2nd/device.h>
#include <lk2nd/version.h>
#include <lk2nd/menu.h>

#define FONT_WIDTH	(5+1)
#define FONT_HEIGHT	12
#define MIN_LINE	40

enum fbcon_colors {
	WHITE = 2,
	SILVER,
	YELLOW,
	ORANGE,
	RED,
	GREEN,
	WHITE_ON_BLUE,
};

static int scale_factor = 0;

static void fbcon_puts(char *str, unsigned type, int y, bool center)
{
	struct fbcon_config *fb = fbcon_display();
	int line_len = fb->width;
	int text_len = strlen(str) * FONT_WIDTH * scale_factor;
	int x = 0;

	if (center)
		x = (line_len - min(text_len, line_len)) / 2;

	while (*str != 0) {
		fbcon_putc_factor_xy(*str++, type, scale_factor, x, y);
		x += FONT_WIDTH * scale_factor;
		if (x >= line_len)
			return;
	}
}

static __PRINTFLIKE(4, 5) void fbcon_printf(unsigned type, int y, bool center, const char *fmt, ...)
{
	char buf[MIN_LINE*3];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, MIN_LINE*3, fmt, ap);
	va_end(ap);

	fbcon_puts(buf, type, y, center);
}

static const uint16_t published_keys[] = {
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_POWER,
	KEY_HOME,
};

/**
 * lk2nd_boot_pressed_key() - Get the pressed key, if any.
 */
static uint16_t lk2nd_boot_pressed_key(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(published_keys); ++i)
		if (lk2nd_keys_pressed(published_keys[i]))
			return published_keys[i];

	return 0;
}

static uint16_t wait_key(void)
{
	uint16_t keycode = 0;

	while (!(keycode = lk2nd_boot_pressed_key()))
		thread_sleep(1);

	while (lk2nd_keys_pressed(keycode))
		thread_sleep(1);

	return keycode;
}

#define xstr(s) str(s)
#define str(s) #s

static void opt_reboot(void)     { reboot_device(0); }
static void opt_recoery(void)    { reboot_device(RECOVERY_MODE); }
static void opt_bootloader(void) { reboot_device(FASTBOOT_MODE); }
static void opt_edl(void)        { reboot_device(EMERGENCY_DLOAD); }
static void opt_shutdown(void)   { shutdown_device(); }

struct {
	char *name;
	unsigned color;
	void (*action)(void);
} menu_options[] = {
	{ "  Reboot  ", GREEN,  opt_reboot },
	{ " Recovery ", ORANGE, opt_recoery },
	{ "Bootloader", ORANGE, opt_bootloader },
	{ "    EDL   ", RED,    opt_edl },
	{ " Shutdown ", RED,    opt_shutdown },
};

/**
 * lk2nd_fastboot_menu() - Display a minimal menu
 */
void lk2nd_fastboot_menu(void)
{
	struct fbcon_config *fb = fbcon_display();
	int y, y_menu, sel = 0, i, old_scale, incr;

	/*
	 * Make sure the specified line lenght fits on the screen.
	 */
	scale_factor = max(1, min(fb->width, fb->height) / (FONT_WIDTH * MIN_LINE));
	old_scale = scale_factor;
	incr = FONT_HEIGHT * scale_factor;

	y = incr * 2;

	fbcon_clear();

	/*
	 * Draw the static part of the menu
	 */

	scale_factor += 1;
	incr = FONT_HEIGHT * scale_factor;
	fbcon_printf(WHITE,  y, true, xstr(BOARD)); y += incr;

	scale_factor = old_scale;
	incr = FONT_HEIGHT * scale_factor;

	fbcon_printf(SILVER, y, true, LK2ND_VERSION); y += incr;
	fbcon_printf(SILVER, y, true, lk2nd_dev.model); y += incr * 2;

	fbcon_printf(RED,    y, true, "Fastboot mode"); y += incr * 2;

	/* Skip lines for the menu */
	y_menu = y;
	y += incr * (ARRAY_SIZE(menu_options) + 1);

	fbcon_printf(SILVER, y, true, "Volume keys to navigate."); y += incr;
	fbcon_printf(SILVER, y, true, "Power key to select."); y += incr;

	/*
	 * Draw the device-specific information at the bottom of the screen
	 */

	scale_factor = max(1, scale_factor-1);
	incr = FONT_HEIGHT * scale_factor;
	y = fb->height - 8 * incr;

	fbcon_printf(WHITE, y, true, "About this device"); y += incr;

	fbcon_printf(SILVER, y, false, " Panel:  %s", lk2nd_dev.panel.name); y += incr;
	if (lk2nd_dev.battery) {
		fbcon_printf(SILVER, y, false, " Battery:  %s", lk2nd_dev.battery); y += incr;
	}
#if WITH_LK2ND_DEVICE_2ND
	fbcon_printf(SILVER, y, false, " Bootloader:  %s", lk2nd_dev.bootloader); y += incr;
#endif

	/*
	 * Loop to render the menu elements
	 */

	scale_factor = old_scale;
	incr = FONT_HEIGHT * scale_factor;
	while (true) {
		y = y_menu;
		fbcon_clear_msg(y/FONT_HEIGHT, (y/FONT_HEIGHT+ARRAY_SIZE(menu_options)*scale_factor));
		for (i = 0; i < (int)ARRAY_SIZE(menu_options); ++i) {
			fbcon_printf(
				i == sel ? menu_options[i].color : SILVER,
				y, true, "%c %s %c",
				i == sel ? '>' : ' ',
				menu_options[i].name,
				i == sel ? '<' : ' '
			);
			y += incr;
		}

		switch (wait_key()) {
		case KEY_POWER:
			menu_options[sel].action();
			break;
		case KEY_VOLUMEUP:
			sel--;
			if (sel < 0) sel = ARRAY_SIZE(menu_options)-1;
			break;
		case KEY_VOLUMEDOWN:
			sel = (sel+1) % ARRAY_SIZE(menu_options);
			break;
		}
	}
}

void display_default_image_on_screen(void);
void display_default_image_on_screen(void)
{
	struct fbcon_config *fb = fbcon_display();
	int y, incr;

	/*
	 * Make sure the specified line lenght fits on the screen.
	 */
	scale_factor = max(1, min(fb->width, fb->height) / (FONT_WIDTH * MIN_LINE));
	incr = FONT_HEIGHT * scale_factor;
	y = fb->height - 3 * incr;

	//fbcon_clear();
	fbcon_clear_msg(y/FONT_HEIGHT, y/FONT_HEIGHT + 3*scale_factor);

	fbcon_printf(WHITE,  y, true, xstr(BOARD)); y += incr;
	fbcon_printf(SILVER, y, true, LK2ND_VERSION); y += incr;
}
