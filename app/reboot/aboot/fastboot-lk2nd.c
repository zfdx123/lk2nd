#include <debug.h>
#include <platform.h>
#include <platform/iomap.h>
#include <pm8x41_regulator.h>
#include "fastboot.h"

#include <string.h>
#include <lib/bio.h>
#include <lib/fs.h>
#include "../fs_util.h"
#include "../boot.h"
#include "../config.h"

extern struct boot_entry *usb_entry_list;
extern int usb_entry_count;

extern void reset_menu();

void load_entries_from_usb()
{
	static bool first_time = 1;

	unsigned int size;
	void *img_data;

	int entry_count;

	int ret;

	if(!first_time) { // FIXME: should probably allow for running this multiple times
		fastboot_okay("");
		return;
	}

	first_time = 0;

	size = fastboot_get_staged_size();
	printf("~~~~~~~~~~size: %u\n", size);

	// 50MB, if your kernel + initramfs + dtb are bigger than that, this will get overwritten
	img_data = VA((addr_t)(DDR_START + 0x3200000));
	fastboot_get_staged(img_data);

	ret = create_membdev("ramdisk", img_data, size);
	printf("create_membdev ret: %d\n", ret);

	ret = entry_count = parse_boot_entries("ramdisk", &usb_entry_list);
	if (ret < 0) {
		printf("falied to parse boot entries from usb: %d\n", ret);
		return;
	}

	usb_entry_count = entry_count;

	reset_menu();
}

static void cmd_load_entries(const char *arg, void *data, unsigned sz)
{
	load_entries_from_usb();

	fastboot_okay("");
}

void loop_print_entries()
{
	for(int i = 0; i < usb_entry_count; i++)
	{
		struct boot_entry *entry = usb_entry_list + i;

		fastboot_info(entry->title);
	}
}

static void cmd_oem_boot(const char *arg, void *data, unsigned sz)
{
	struct boot_entry *entry_to_boot;

	load_entries_from_usb();

	if(strlen(arg)) {
		entry_to_boot = get_entry_by_title(usb_entry_list, usb_entry_count, (char *)arg);
	} else if (usb_entry_count == 1) {
		// don't force the user to pick if there is only one option
		entry_to_boot = &usb_entry_list[0];
	} else {
		entry_to_boot = NULL;
	}

	if(entry_to_boot) {
		fastboot_info("booting...");
		fastboot_okay("");
		boot_to_entry(entry_to_boot);
	}
	else {
		fastboot_info("usage: `fastboot oem boot [entry title]`");
		fastboot_info("[entry title] is optional when there is exactly one entry");
		fastboot_info("list of entries present in this image:");

		loop_print_entries();

		fastboot_info("--- end of enumeration");

		if(strlen(arg)) {
			dprintf(CRITICAL, "booting from usb failed (entry not found - `%s`, total amount of entries loaded: `%d`)\n", arg, usb_entry_count);
			fastboot_fail("specified entry not found");
		}
		else {
			dprintf(CRITICAL, "booting from usb failed - no entries or more than one entry detected (total amount of entries loaded: `%d`)\n", usb_entry_count);
			fastboot_fail("the amount of entries present is not exactly one");
		}
	}
	
}


static void cmd_oem_dump_regulators(const char *arg, void *data, unsigned sz)
{
	char response[MAX_RSP_SIZE];
	struct spmi_regulator *vreg;
	for (vreg = target_get_regulators(); vreg->name; ++vreg) {
		snprintf(response, sizeof(response), "%s: enabled: %d, voltage: %d mV",
			 vreg->name, regulator_is_enabled(vreg),
			 regulator_get_voltage(vreg));
		fastboot_info(response);
	}
	fastboot_okay("");
}

#if WITH_DEBUG_LOG_BUF
static void cmd_oem_lk_log(const char *arg, void *data, unsigned sz)
{
	fastboot_stage(lk_log_getbuf(), lk_log_getsize());
}
#endif

void fastboot_lk2nd_register_commands(void) {
	fastboot_register("oem boot", cmd_oem_boot);
	fastboot_register("oem dump-regulators", cmd_oem_dump_regulators);
#if WITH_DEBUG_LOG_BUF
	fastboot_register("oem lk_log", cmd_oem_lk_log);
#endif
	fastboot_register("oem load_entries", cmd_load_entries);
}
