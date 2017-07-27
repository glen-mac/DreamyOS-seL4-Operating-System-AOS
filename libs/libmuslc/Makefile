# @LICENSE(NICTA)

# Targets
TARGETS := libmuslc.a

# Source files required to build the target
CFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/*/*.c))

ifeq ($(ARCH),x86_64) 
$(error x86_64 not supported)
endif

ifeq ($(ARCH),x86)
MARCH = ia32
else
MARCH = $(ARCH)
endif

ifdef CONFIG_X86_64
	MARCH = x86_64
endif

# ASM files require to build the target
#$(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/internal/$(ARCH)/*.S)) 
ASMFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/thread/$(MARCH)/*.S)) \
            $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/ldso/$(MARCH)/*.S)) \
            $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/setjmp/$(MARCH)/*.S)) \
            $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/signal/$(MARCH)/*.S)) \
	    $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/fenv/$(MARCH)/*.S)) \
	    $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/string/$(MARCH)/*.S)) \
	    $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/math/$(MARCH)/*.S)) 

ifdef CONFIG_X86_64
	MARCH = ia32
endif

# Header files/directories this library provides
HDRFILES := $(wildcard ${SOURCE_DIR}/include/*) \
	    $(wildcard ${SOURCE_DIR}/arch_include/$(MARCH)/arch/*)



NK_CFLAGS += -fcx-limited-range -ffreestanding -fexcess-precision=standard -frounding-math \
		  -U __BIG_ENDIAN  \
	    -I$(SOURCE_DIR)/src -I$(SOURCE_DIR)/src/internal  -Wno-missing-braces -Wno-parentheses -Wno-uninitialized \
		-Wno-unused-variable -Wno-unused-value -D_XOPEN_SOURCE=700 -Wno-unused-local-typedefs

include $(SEL4_COMMON)/common.mk


PREBUILT_FILE=$(SOURCE_DIR)/pre-built/libmuslc-${MARCH}-${PLAT}.a
ifeq ($(CONFIG_LIB_MUSL_C_USE_PREBUILT),y)
  ifneq ($(wildcard $(PREBUILT_FILE)),)

libmuslc.a: $(PREBUILT_FILE)
	$(call cp_file,$<,$@)

# Don't attempt to build the prebuilt files.
$(PREBUILT_FILE):
	false

  endif
endif
