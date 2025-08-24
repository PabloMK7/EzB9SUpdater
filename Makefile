# TARGET #


TARGET := 3DS
LIBRARY := 0

ifeq ($(TARGET),$(filter $(TARGET),3DS WIIU))
    ifeq ($(strip $(DEVKITPRO)),)
        $(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
    endif
endif

# COMMON CONFIGURATION #

NAME := EzB9SUpdater

BUILD_DIR := build
OUTPUT_DIR := output
SOURCE_DIRS := source source/json source/allocator source/bootntr
INCLUDE_DIRS := $(SOURCE_DIRS) include

EXTRA_OUTPUT_FILES :=

LIBRARY_DIRS := $(PORTLIBS) $(CTRULIB) $(DEVKITPRO)/libcwav $(DEVKITPRO)/libncsnd
LIBRARIES := citro3d ctru png z m curl mbedtls mbedx509 mbedcrypto cwav ncsnd

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_MICRO := 1

ifndef EZB9S_CONFIG_LINK
$(error You must pass EZB9S_CONFIG_LINK on the make command line, e.g. 'make EZB9S_CONFIG_LINK="https://example.com"')
endif

BUILD_FLAGS := -march=armv6k -mtune=mpcore -mfloat-abi=hard
BUILD_FLAGS_CC := -g -Wall -Wno-strict-aliasing -O3 -mword-relocations \
					-fomit-frame-pointer -ffast-math $(ARCH) $(INCLUDE) -D__3DS__ $(BUILD_FLAGS) \
					-DAPP_VERSION_MAJOR=${VERSION_MAJOR} \
					-DAPP_VERSION_MINOR=${VERSION_MINOR} \
					-DAPP_VERSION_MICRO=${VERSION_MICRO} \
                    -DEZB9S_CONFIG_LINK=\"${EZB9S_CONFIG_LINK}\"

BUILD_FLAGS_CXX := $(COMMON_FLAGS) -std=gnu++11
RUN_FLAGS :=





# 3DS/Wii U CONFIGURATION #

ifeq ($(TARGET),$(filter $(TARGET),3DS WIIU))
	TITLE := EzB9SUpdater
	DESCRIPTION := Easy Updater For B9S
    AUTHOR := PabloMK7
endif

# 3DS CONFIGURATION #

ifeq ($(TARGET),3DS)
    LIBRARY_DIRS += $(DEVKITPRO)/libctru $(DEVKITPRO)/portlibs/3ds/
    LIBRARIES += citro3d ctru png z m curl mbedtls mbedx509 mbedcrypto

    PRODUCT_CODE := CTR-P-EZB9
    UNIQUE_ID := 0xECB95

    CATEGORY := Application
    USE_ON_SD := true

    MEMORY_TYPE := Application
    SYSTEM_MODE := 64MB
    SYSTEM_MODE_EXT := Legacy
    CPU_SPEED := 268MHz
    ENABLE_L2_CACHE := true

    ICON_FLAGS := --flags visible,recordusage

	ROMFS_DIR := romfs

    BANNER_AUDIO := resources/audio.cwav
    
    BANNER_IMAGE := resources/banner.png
    
	ICON := resources/icon.png

endif

# INTERNAL #

include buildtools/make_base

cleanupdater:
	@rm -f $(BUILD_DIR)/3ds-arm/source/updater.d $(BUILD_DIR)/3ds-arm/source/updater.o

re : clean all
.PHONY: re