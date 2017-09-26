# Targets
TARGETS := game.bin

# Source files required to build the target
CFILES   := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/crt/*.c))

# Libraries required to build the target
LIBS := muslc sel4 sos
#export DEBUG=1
include $(SEL4_COMMON)/common.mk