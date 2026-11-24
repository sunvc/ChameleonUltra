# MAX fork features: all custom sources live in $(PROJ_DIR)/max/.
# This file is included once from the upstream Makefile with a single line.
# Add new fork sources here instead of editing upstream build lists.

SRC_FILES += \
  $(PROJ_DIR)/max/max_ble_guard.c \

INC_FOLDERS += \
  $(PROJ_DIR)/max \

# Reader-dependent features exist only on the Ultra build.
ifeq (${CURRENT_DEVICE_TYPE}, ${CHAMELEON_ULTRA})
SRC_FILES += \
  $(PROJ_DIR)/max/mf1_offline_clone.c \
  $(PROJ_DIR)/max/max_offline_clone_job.c
endif
