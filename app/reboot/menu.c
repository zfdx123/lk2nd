// SPDX-License-Identifier: GPL-2.0+
// © 2019 Mis012

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pm8x41.h>
#include <pm8x41_hw.h>
#include <kernel/thread.h>
#include <dev/fbcon.h>
#include <target.h>

#include "aboot/aboot.h"

#include "boot.h"
#include "config.h"

static int num_of_boot_entries;
static int total_num_of_boot_entries;
static struct boot_entry *entry_list;

int usb_entry_count = 0;
struct boot_entry *usb_entry_list = NULL;

struct hardcoded_entry {
	char *title;
	void (*function)(void);
};

//FUGLY
#define TIMEOUT_TEXT "press volume down for boot menu"
#define TIMEOUT_TEXT_SCALE 2

extern uint32_t target_volume_down(); //used in non-FUGLY code as well; commented out there, will use this

extern struct global_config global_config;
extern bool FUGLY_boot_to_default_entry;

static void FUGLY_default_boot_function() 
{
	int i;
	int num_iters = global_config.timeout * 1000 / 100; // times 1000 - sec to msec; divided by 100 - see "lower cpu stress"

	if(global_config.timeout > 0) {
		fbcon_draw_text(20, 20, TIMEOUT_TEXT, TIMEOUT_TEXT_SCALE, 0xFF0000);

		for (i = 0; i < num_iters; i++) {
			if (target_volume_down()) {
				fbcon_draw_text(20, 20, TIMEOUT_TEXT, TIMEOUT_TEXT_SCALE, 0x000000);
				return; //continue to boot menu
			}
			thread_sleep(100); //lower cpu stress
		}
	} else {
		thread_sleep(100); //somehow this is needed to avoid freeze
	}

	boot_to_entry(global_config.default_entry);
	dprintf(CRITICAL, "ERROR: Booting default entry failed. Forcibly bringing up menu.\n");
}
// end of FUGLY
void boot_from_mmc(void);
void boot_recovery_from_mmc(void);
#define HARDCODED_ENTRY_COUNT 3
struct hardcoded_entry hardcoded_entry_list[HARDCODED_ENTRY_COUNT] = {
	{.title = "legacy boot from 'boot' partition", .function = boot_from_mmc},
	{.title = "legacy boot from 'recovery' partition", .function = boot_recovery_from_mmc},
	{.title = "power off", .function = shutdown_device}
};

#define BOOT_ENTRY_SCALE 2
#define ACTUAL_FONT_WIDTH (FONT_WIDTH * BOOT_ENTRY_SCALE)
#define ACTUAL_FONT_HEIGHT (FONT_HEIGHT * BOOT_ENTRY_SCALE)
#define ENTRY_HEIGHT (ACTUAL_FONT_HEIGHT + 4)

static int menu_lines;
static int scroll_offset = 0;
static int selected_option = 0;

static void draw_menu(void)
{
	total_num_of_boot_entries = num_of_boot_entries + usb_entry_count;

	uint32_t max_width = fbcon_get_width() - 1;
	uint32_t max_height = fbcon_get_height() - 1;

	uint32_t frame_width = max_width * 0.90;
	uint32_t frame_height = max_height * 0.90;

	uint32_t margin_x = (max_width - frame_width) / 2;
	uint32_t margin_y = (max_height - frame_height) / 2;

	uint32_t highlight_color;
	uint32_t font_color;

	menu_lines = (frame_height / ENTRY_HEIGHT) - HARDCODED_ENTRY_COUNT - 1;
	if (menu_lines > total_num_of_boot_entries)
		menu_lines = total_num_of_boot_entries;

	fbcon_draw_text(margin_x + 1, (max_height * 0.10) / 4 - ACTUAL_FONT_HEIGHT/2, "re-boot | lk2nd version 0.42", BOOT_ENTRY_SCALE, 0xFFFFFF);

	fbcon_draw_rectangle(margin_x, margin_y, frame_width, frame_height, 0xFFFFFF);

	int i;
	for (i = 0; i < menu_lines; i++) {
		struct boot_entry *current_entry_list;
		int entry_pos;
		uint32_t entry_color;

		if(scroll_offset + i < usb_entry_count) {
			current_entry_list = usb_entry_list;
			entry_pos = scroll_offset + i;
			entry_color = 0x0000FF;
		}
		else {
			current_entry_list = entry_list;
			entry_pos = scroll_offset + (i - usb_entry_count);
			entry_color = 0xFF0000;
		}

		if(scroll_offset + i == selected_option)
			highlight_color = entry_color;
		else
			highlight_color = 0x000000;

		if((current_entry_list + entry_pos)->error)
			font_color = 0xFF0000;
		else
			font_color = 0xFFFFFF;

		fbcon_draw_filled_rectangle(margin_x + 8, (margin_y + 8) + i * ENTRY_HEIGHT - 1, frame_width - (2 * 8), 2 + ACTUAL_FONT_HEIGHT + 2, highlight_color);
		fbcon_draw_text(margin_x + 10, (margin_y + 10) + i * ENTRY_HEIGHT, (current_entry_list + entry_pos)->title, BOOT_ENTRY_SCALE, font_color);
	}

	uint32_t separator_y = (margin_y + 8) + menu_lines * ENTRY_HEIGHT + 2;

	fbcon_draw_horizontal_line(margin_x + 8, (margin_x + 8) + (frame_width - (2 * 8)), separator_y, 0xFFFFFF);

	for (i = 0; i < HARDCODED_ENTRY_COUNT; i++) {

		if((i + total_num_of_boot_entries) == selected_option)
			highlight_color = 0xFF0000;
		else
			highlight_color = 0x000000;

		font_color = 0xFFFFFF;

		fbcon_draw_filled_rectangle(margin_x + 8, (separator_y + 3) + i * ENTRY_HEIGHT, frame_width - (2 * 8), 2 + ACTUAL_FONT_HEIGHT + 2, highlight_color);
		fbcon_draw_text(margin_x + 10, (separator_y + 5) + i * ENTRY_HEIGHT, hardcoded_entry_list[i].title, BOOT_ENTRY_SCALE, font_color);
	}

	fbcon_flush();

}

#define KEY_DETECT_FREQUENCY		50

extern int target_volume_up();
//extern uint32_t target_volume_down(); //declared up top because FUGLY

static bool handle_keys(void) {
	uint32_t volume_up_pressed = target_volume_up();
	uint32_t volume_down_pressed = target_volume_down();
	uint32_t power_button_pressed = pm8x41_get_pwrkey_is_pressed();

	total_num_of_boot_entries = num_of_boot_entries + usb_entry_count;

	//FUGLY
	if(FUGLY_boot_to_default_entry) {
		FUGLY_boot_to_default_entry = 0; //in case we interrupt the autoboot
		FUGLY_default_boot_function();
	}
	//end of FUGLY

	if(volume_up_pressed) {
		if(selected_option > 0) {
			selected_option--;

			// if we moved out of the scroll window, slide the scroll window
			if(selected_option - scroll_offset == -1)
				scroll_offset--;
		}
		
		return 1;
	}
	
	if (volume_down_pressed) {
		if(selected_option < (total_num_of_boot_entries + HARDCODED_ENTRY_COUNT - 1)) {
			selected_option++;

			// if we moved out of the scroll window, slide the scroll window
			// unless we should be moving onto the hardcoded options
			if(selected_option < total_num_of_boot_entries && 
			   selected_option - scroll_offset == menu_lines)
				scroll_offset++;
		}

		return 1;
	}

	if(power_button_pressed) {
		if (selected_option < usb_entry_count) {
			struct boot_entry *entry = usb_entry_list + selected_option;
			boot_to_entry(entry);
		}
		else if(selected_option < total_num_of_boot_entries) {
			struct boot_entry *entry = entry_list + selected_option - usb_entry_count;
			boot_to_entry(entry);
		}
		else {
			hardcoded_entry_list[selected_option - total_num_of_boot_entries].function();
		}
	}

	return 0;
}

int menu_thread(struct {struct boot_entry *entry_list; int num_of_boot_entries;} *arg) {
	entry_list = arg->entry_list;
	num_of_boot_entries = arg->num_of_boot_entries;

	fbcon_clear();
	if(!FUGLY_boot_to_default_entry)
		draw_menu();

	while(1) {
		if(handle_keys()) {
			draw_menu();
			thread_sleep(100); //improve precision
		}

		thread_sleep(KEY_DETECT_FREQUENCY);
	}
}

void reset_menu()
{
	fbcon_clear();
	selected_option = 0;
	draw_menu();
}

// hardcoded functions

extern unsigned int boot_into_recovery;

void boot_from_mmc(void) {
	boot_into_recovery = 0;
	boot_linux_from_mmc();
}

void boot_recovery_from_mmc(void) {
	boot_into_recovery = 1;
	boot_linux_from_mmc();
}
