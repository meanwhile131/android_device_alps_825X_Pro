#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),825X_Pro)

# Preserve the device path: all-subdir-makefiles changes LOCAL_PATH.
_825X_PATH := $(LOCAL_PATH)

# ---------------------------------------------------------------------------
# Boot kernel: use the stock prebuilt Image.gz-dtb.
#
# The DTB appended by a source build comes from the UJC201/Jancar device tree
# (56 MB framebuffer at 0x61800000, mdp at 0x65100000). This panel's LK/ARM2
# firmware expects the 825X_Pro layout instead (28 MB framebuffer, mdp at
# 0x63500000); booting with the wrong DTB leaves the panel dark with the
# backlight on. The stock kernel is also what the known-good boot_flashable.img
# carries, so copy it to INSTALLED_KERNEL_TARGET for boot/recovery.
#
# Keep TARGET_KERNEL_SOURCE for Soong's generated_kernel_includes (its
# soong.variables was already written from BoardConfig.mk). Clearing it only
# on the make side is what selects kernel.mk's prebuilt-kernel path.
# ---------------------------------------------------------------------------
TARGET_PREBUILT_KERNEL := $(_825X_PATH)/prebuilts/kernel
TARGET_KERNEL_SOURCE :=

# The kernel's built-in first-stage fstab mounts the avm partition at /avm.
# Stock system.img has that directory; without it init aborts early and reboots
# straight to the bootloader (looks like a "boot failure" in fastboot).
_825X_AVM_DIR := $(TARGET_ROOT_OUT)/avm
$(_825X_AVM_DIR):
	$(hide) mkdir -p $@
$(PRODUCT_OUT)/system.img: $(_825X_AVM_DIR)

# The kernel's first-stage fstab mounts the vendor partition at /vendor.
# Stock system.img has /vendor as a real directory; the generic build makes it
# a symlink to /system/vendor, which breaks the sphal linker namespace
# ("/vendor/lib64/..." resolves to /system/vendor/... and is rejected).
_825X_ROOT_VENDOR := $(TARGET_ROOT_OUT)/vendor
$(_825X_ROOT_VENDOR): $(TARGET_ROOT_OUT)/init.environ.rc
	$(hide) rm -rf $@
	$(hide) mkdir -p $@
$(PRODUCT_OUT)/system.img: $(_825X_ROOT_VENDOR)

# ---------------------------------------------------------------------------
# Ship our precompiled SELinux policy in /odm/etc/selinux.
#
# The device's separate vendor partition carries a stock vendor_sepolicy.cil
# that does not combine with our platform CIL (secilc fails on genfscon rules,
# init reboots to bootloader). init checks /odm/etc/selinux/precompiled_sepolicy
# BEFORE /vendor, and there is no odm partition on this device, so placing our
# own matching precompiled policy there makes init load it directly.
# ---------------------------------------------------------------------------
_825X_SRC_SEPOLICY := $(TARGET_OUT_VENDOR)/etc/selinux/precompiled_sepolicy
_825X_ROOT_SETUP  := $(TARGET_ROOT_OUT)/init.environ.rc
_825X_ODM_ETC     := $(TARGET_ROOT_OUT)/odm/etc

# Replace the /odm/etc -> /vendor/odm/etc symlink with a real directory and
# ship the platform's own precompiled policy there. The vendor partition's
# stock precompiled_sepolicy has a different plat_and_mapping sha256, so init
# ignores it, tries to compile platform + stock vendor CIL at boot, secilc
# fails on genfscon rules, and init reboots straight to the bootloader.
_825X_PRECOMPILED_SEPOLICY := $(call intermediates-dir-for,ETC,precompiled_sepolicy)/precompiled_sepolicy
_825X_PRECOMPILED_SHA := $(call intermediates-dir-for,ETC,precompiled_sepolicy.plat_and_mapping.sha256)/precompiled_sepolicy.plat_and_mapping.sha256
$(_825X_ODM_ETC): $(_825X_ROOT_SETUP) $(_825X_PRECOMPILED_SEPOLICY) $(_825X_PRECOMPILED_SHA)
	$(hide) rm -rf $@
	$(hide) mkdir -p $@/selinux
	$(hide) cp $(_825X_PRECOMPILED_SEPOLICY) $@/selinux/precompiled_sepolicy
	$(hide) cp $(_825X_PRECOMPILED_SHA) $@/selinux/precompiled_sepolicy.plat_and_mapping.sha256

$(PRODUCT_OUT)/system.img: $(_825X_ODM_ETC)

include $(call all-subdir-makefiles,$(LOCAL_PATH))

# ---------------------------------------------------------------------------
# Fix /sbin/sh in the recovery ramdisk.
#
# TWRP ships a stale `sh -> recovery` symlink. /sbin/permissive.sh uses
# `#!/sbin/sh`, so invoking it starts another full TWRP instance, which runs
# permissive.sh again -> recursive fork loop (dozens of `sh` processes), the
# framebuffer is cleared on every respawn and the screen stays black.
# Point sh at the real bash binary TWRP already ships instead.
# ---------------------------------------------------------------------------
_825X_RECOVERY_SH := $(TARGET_RECOVERY_ROOT_OUT)/sbin/sh
$(_825X_RECOVERY_SH): $(TARGET_RECOVERY_ROOT_OUT)/sbin/bash
	@echo "----- Fixing recovery /sbin/sh -> bash -----"
	$(hide) rm -f $@
	$(hide) ln -sf bash $@
	# adbd's shell service runs as shell, and these are installed 0750 by the
	# build; make them world-executable so `adb shell` works in TWRP.
	$(hide) chmod 0755 $(TARGET_RECOVERY_ROOT_OUT)/sbin/bash \
		$(TARGET_RECOVERY_ROOT_OUT)/sbin/linker64 \
		$(TARGET_RECOVERY_ROOT_OUT)/sbin/libc.so
	# Allow `adb root` in TWRP so pstore / logd files are readable for debugging.
	$(hide) sed -i 's/^ro.secure=1$$/ro.secure=0/' $(TARGET_RECOVERY_ROOT_OUT)/prop.default

$(PRODUCT_OUT)/ramdisk-recovery.cpio: $(_825X_RECOVERY_SH)
$(PRODUCT_OUT)/recovery.img: $(_825X_RECOVERY_SH)

# ---------------------------------------------------------------------------
# Stock vendor SELinux blobs.
#
# The generated vendor_file_contexts only covers generic AOSP HAL names, so
# without this the build labels the MediaTek/AutoChips binaries with the
# fallback `vendor_file` context. init then cannot exec them (SELinux
# denials), the HALs crash and the device bootloops. Ship the stock SELinux
# files from vendor/alps/825X_Pro and use the stock vendor contexts when
# labeling vendor.img. They are installed by this hook rather than
# PRODUCT_COPY_FILES because the build installs files at the same paths.
# ---------------------------------------------------------------------------
_825X_SELINUX_SRC  := vendor/alps/825X_Pro/proprietary/vendor/etc/selinux
_825X_SELINUX_FILES := \
    vendor_file_contexts \
    vendor_sepolicy.cil \
    vendor_property_contexts \
    vendor_hwservice_contexts \
    vendor_seapp_contexts \
    vndservice_contexts \
    vendor_mac_permissions.xml \
    precompiled_sepolicy \
    precompiled_sepolicy.plat_and_mapping.sha256 \
    plat_pub_versioned.cil \
    plat_sepolicy_vers.txt
_825X_SELINUX_STAMP := $(PRODUCT_OUT)/.vendor_selinux_blobs.stamp

$(_825X_SELINUX_STAMP): \
    $(addprefix $(TARGET_OUT_VENDOR)/etc/selinux/,$(_825X_SELINUX_FILES)) \
    $(addprefix $(_825X_SELINUX_SRC)/,$(_825X_SELINUX_FILES))
	@echo "----- Installing stock vendor SELinux blobs -----"
	$(hide) cp -f $(addprefix $(_825X_SELINUX_SRC)/,$(_825X_SELINUX_FILES)) \
	    $(TARGET_OUT_VENDOR)/etc/selinux/
	$(hide) touch $@

$(PRODUCT_OUT)/vendor.img: $(_825X_SELINUX_STAMP)

# Combined file_contexts: our platform contexts + stock vendor contexts.
# build_image.py labels the vendor filesystem with SELINUX_FC, so make it
# point at the combined blob for vendor.img only (target-specific variable).
_825X_PLAT_FC_TXT := $(call intermediates-dir-for,ETC,file_contexts.bin)/file_contexts.concat.tmp
_825X_FC_TXT      := $(PRODUCT_OUT)/obj/ETC/file_contexts_825x.concat.tmp
_825X_FC_BIN      := $(PRODUCT_OUT)/obj/ETC/file_contexts_825x.bin

$(_825X_FC_TXT): $(_825X_PLAT_FC_TXT) $(_825X_SELINUX_SRC)/vendor_file_contexts
	$(hide) mkdir -p $(dir $@)
	$(hide) cat $^ > $@

$(_825X_FC_BIN): $(_825X_FC_TXT) $(HOST_OUT_EXECUTABLES)/fc_sort $(HOST_OUT_EXECUTABLES)/sefcontext_compile
	$(hide) $(HOST_OUT_EXECUTABLES)/fc_sort $< $@.sorted
	$(hide) $(HOST_OUT_EXECUTABLES)/sefcontext_compile -o $@ $@.sorted

$(PRODUCT_OUT)/vendor.img: SELINUX_FC := $(_825X_FC_BIN)
$(PRODUCT_OUT)/vendor.img: $(_825X_FC_BIN)

# ---------------------------------------------------------------------------
# vbmeta image
#
# LK refuses to boot our rebuilt images with an OEM-signed vbmeta, so ship a
# test-key-signed vbmeta with verification disabled instead. Declaring the
# standard target makes main.mk include it in droidcore and `m vbmetaimage`.
# ---------------------------------------------------------------------------
INSTALLED_VBMETAIMAGE_TARGET := $(PRODUCT_OUT)/vbmeta.img

$(INSTALLED_VBMETAIMAGE_TARGET): $(AVBTOOL) $(BOARD_AVB_KEY_PATH)
	@echo "----- Generating test-key vbmeta.img -----"
	$(hide) python2 $(AVBTOOL) make_vbmeta_image \
		--algorithm $(BOARD_AVB_ALGORITHM) \
		--key $(BOARD_AVB_KEY_PATH) \
		$(BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS) \
		--output $@

endif
