# MAX fork features: all custom sources live in $(PROJ_DIR)/max/.
# This file is included once from the upstream Makefile with a single line.
# Add new fork sources here instead of editing upstream build lists.

SRC_FILES += \
  $(PROJ_DIR)/max/max_ble_guard.c \

INC_FOLDERS += \
  $(PROJ_DIR)/max \
