# SPDX-License-Identifier: BSD-3-Clause
LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/cmdline.o \
	$(LOCAL_DIR)/lkfdt.o \
	$(LOCAL_DIR)/mmu.o \

# Generate the version tag from VCS

LK2ND_VERSION := $(shell $(LK_TOP_DIR)/lk2nd/scripts/do-release.sh -d)
VERSION_FILE := $(BUILDDIR)/$(LOCAL_DIR)/version.c

.PHONY: $(VERSION_FILE)
$(VERSION_FILE):
	@$(MKDIR)
	@echo generating $@ for lk2nd $(LK2ND_VERSION)
	@echo "const char* LK2ND_VERSION = \"$(LK2ND_VERSION)\";" > $@

$(BUILDDIR)/$(LOCAL_DIR)/version.o: $(VERSION_FILE)
	@$(MKDIR)
	@echo compiling $<
	$(NOECHO)$(CC) $(CFLAGS) $(THUMBCFLAGS) --std=c99 $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

OBJS += \
	$(LOCAL_DIR)/version.o \
