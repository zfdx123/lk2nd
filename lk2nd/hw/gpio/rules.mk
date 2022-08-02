# SPDX-License-Identifier: BSD-3-Clause
LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/gpio.o \
	$(LOCAL_DIR)/tlmm.o \

ifeq ($(TARGET), msm8960)
OBJS += $(LOCAL_DIR)/pm8921.o
else
OBJS += $(LOCAL_DIR)/pm8x41.o
endif


