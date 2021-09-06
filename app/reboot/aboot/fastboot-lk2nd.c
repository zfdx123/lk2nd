#include <debug.h>
#include <platform.h>
#include <platform/iomap.h>
#include <pm8x41_regulator.h>
#include "fastboot.h"

#include <lib/bio.h>
#include <lib/fs.h>
#include "../fs_util.h"
#include "../config.h"

extern struct boot_entry *usb_entry_list;
extern int usb_entry_count;

extern void reset_menu();

void load_entries_from_usb()
{
	int entry_count;

	int ret;

	ret = entry_count = parse_boot_entries("ramdisk", &usb_entry_list);
	if (ret < 0) {
		printf("falied to parse boot entries from usb: %d\n", ret);
		return;
	}

	usb_entry_count = entry_count;
}

static void cmd_oem_boot(const char *arg, void *data, unsigned sz)
{
	static bool first_time = 1;
	unsigned int size;
	void *img_data;

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

	load_entries_from_usb();

	reset_menu();

	fastboot_okay("");
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
}
