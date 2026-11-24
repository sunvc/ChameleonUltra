# BRAVO fork features: all custom sources live in $(PROJ_DIR)/bravo/.
# This file is included once from the upstream Makefile with a single line.
# Add new fork sources here instead of editing upstream build lists.

SRC_FILES += \
  $(PROJ_DIR)/bravo/bravo_ble_guard.c \
  $(PROJ_DIR)/bravo/bravo_polling.c \

INC_FOLDERS += \
  $(PROJ_DIR)/bravo \

# Reader-dependent features exist only on the Ultra build.
ifeq (${CURRENT_DEVICE_TYPE}, ${CHAMELEON_ULTRA})
SRC_FILES += \
  $(PROJ_DIR)/bravo/mf1_offline_clone.c \
  $(PROJ_DIR)/bravo/bravo_offline_clone_job.c
endif
