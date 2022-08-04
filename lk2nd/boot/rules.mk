# SPDX-License-Identifier: BSD-3-Clause
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULES += \
	lib/fs \
	lib/bio \
	lib/partition \
	lk2nd/boot/menu \

OBJS += \
	$(LOCAL_DIR)/boot.o \
	$(LOCAL_DIR)/util.o \
	$(LOCAL_DIR)/aboot.o \
	$(LOCAL_DIR)/abootimg.o \
