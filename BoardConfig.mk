#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/alps/825X_Pro

# Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 := 
TARGET_CPU_VARIANT := generic
TARGET_CPU_VARIANT_RUNTIME := generic

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv7-a-neon
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := generic
TARGET_2ND_CPU_VARIANT_RUNTIME := generic

# APEX
OVERRIDE_TARGET_FLATTEN_APEX := true

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := 825X_Pro
TARGET_NO_BOOTLOADER := true

# Display
# The live mtkfb framebuffer reports 1024x768 (fb0/modes), not the panel's
# 720x1280 touch resolution in the DTBO. Match the framebuffer so TWRP picks
# the landscape_mdpi theme.
TARGET_SCREEN_DENSITY := 240
TARGET_SCREEN_WIDTH := 1024
TARGET_SCREEN_HEIGHT := 768

# The panel is physically rotated 90 relative to the framebuffer. TWRP 90
# makes the UI upright; touch coordinates must be transposed to match.
TW_ROTATION := 90

# Use TWRP's built-in transpose instead of a patch to minuitwrp. The
# /sbin/touch_abs init service adjusts the evdev abs range so TWRP's generic
# scaling produces the right coordinates for this panel.
RECOVERY_TOUCHSCREEN_SWAP_XY := true

# mtkfb treats FBIOBLANK as early-suspend and powers the LCD off. That also
# suspends mtk-tpd, whose resume workqueue fails to cancel ("cancel
# resume_workqueue failed") and leaves the touchscreen dead. Disable both
# the blank timer and the blanking itself.
TW_NO_SCREEN_TIMEOUT := true
TW_NO_SCREEN_BLANK := true

# Kernel
BOARD_BOOT_HEADER_VERSION := 1
BOARD_KERNEL_BASE := 0x40078000
BOARD_KERNEL_CMDLINE := bootopt=64S3,32N2,64N2 buildvariant=userdebug
BOARD_KERNEL_PAGESIZE := 2048
BOARD_MKBOOTIMG_ARGS += --header_version $(BOARD_BOOT_HEADER_VERSION)
# Load addresses must match stock: the decompressed kernel is ~27 MB and would
# otherwise overwrite a ramdisk loaded at the default 0x41078000.
BOARD_MKBOOTIMG_ARGS += --kernel_offset 0x00008000
BOARD_MKBOOTIMG_ARGS += --ramdisk_offset 0x11A88000
BOARD_MKBOOTIMG_ARGS += --second_offset 0x00E88000
BOARD_MKBOOTIMG_ARGS += --tags_offset 0x07808000
BOARD_KERNEL_IMAGE_NAME := Image
BOARD_KERNEL_SEPARATED_DTBO := true
TARGET_KERNEL_CONFIG := 825X_Pro_defconfig
TARGET_KERNEL_SOURCE := kernel/alps/825X_Pro

# Kernel - prebuilt
TARGET_FORCE_PREBUILT_KERNEL := true
ifeq ($(TARGET_FORCE_PREBUILT_KERNEL),true)
TARGET_PREBUILT_KERNEL := $(DEVICE_PATH)/prebuilts/kernel
# The recovery DTBO is the 43511-byte image at the start of the dtbo partition;
# embedding the full 8 MB partition image makes the bootloader reject recovery.
BOARD_PREBUILT_DTBOIMAGE := $(DEVICE_PATH)/prebuilts/recovery_dtbo.img
BOARD_KERNEL_SEPARATED_DTBO := 
endif

# Partitions
BOARD_BUILD_SYSTEM_ROOT_IMAGE := true
BOARD_FLASH_BLOCK_SIZE := 131072 # (BOARD_KERNEL_PAGESIZE * 64)
BOARD_BOOTIMAGE_PARTITION_SIZE := 10485760
BOARD_DTBOIMG_PARTITION_SIZE := 8388608
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 33554432

# Platform
TARGET_BOARD_PLATFORM := ac8257

# Properties
TARGET_SYSTEM_PROP += $(DEVICE_PATH)/system.prop
TARGET_VENDOR_PROP += $(DEVICE_PATH)/vendor.prop

# Filesystem tools (required for PRODUCT_SHIPPING_API_LEVEL >= 27)
TARGET_USES_MKE2FS := true

# adbd execs /sbin/sh with a lone "-" for interactive shells. The recovery
# ramdisk inherited a stale `sh -> recovery` symlink from an earlier build, so
# TWRP tried to open a file literally named "-" ("Problem opening - ...").
# Install the real mksh into recovery as /sbin/sh instead.
TWRP_REQUIRED_MODULES += sh

# Recovery
# TWRP is checked out at bootable/recovery (see the local manifest).
RECOVERY_VARIANT := twrp

# mtkfb exposes a 1024x2304 virtual framebuffer (3 x 768). Double buffering
# plus an explicit FBIOPAN_DISPLAY is required to make the panel scan out the
# buffer TWRP actually draws into; without it the screen stays on whatever
# buffer the bootloader left active.
TW_FBIOPAN := true

TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/rootdir/etc/fstab.ac8257
BOARD_INCLUDE_RECOVERY_DTBO := true
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true

# Security patch level
VENDOR_SECURITY_PATCH := 2019-10-05

# Recovery needs adb without an authorization dialog (no volume keys)
WITH_ADB_INSECURE := true

# Verified Boot
BOARD_AVB_ENABLE := false
BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS += --flags 3
BOARD_AVB_RECOVERY_KEY_PATH := external/avb/test/data/testkey_rsa4096.pem
BOARD_AVB_RECOVERY_ALGORITHM := SHA256_RSA4096
BOARD_AVB_RECOVERY_ROLLBACK_INDEX := 1
BOARD_AVB_RECOVERY_ROLLBACK_INDEX_LOCATION := 1

# VINTF
DEVICE_MANIFEST_FILE += $(DEVICE_PATH)/manifest.xml

# Inherit the proprietary files
include vendor/alps/825X_Pro/BoardConfigVendor.mk
