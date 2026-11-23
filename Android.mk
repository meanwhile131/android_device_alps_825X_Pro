#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),825X_Pro)

# Preserve the device path: all-subdir-makefiles changes LOCAL_PATH.
_825X_PATH := $(LOCAL_PATH)

include $(call all-subdir-makefiles,$(LOCAL_PATH))

# ---------------------------------------------------------------------------
# Flashable recovery image (OEM vbmeta)
#
# This device's LK only boots a recovery that carries an OEM-signed AVB
# vbmeta:
#   * test-key signed vbmeta -> rejected, drops to fastboot
#   * no vbmeta at all       -> hangs at the boot logo
#
# The stock recovery's vbmeta is shipped as a prebuilt and re-used verbatim.
# LK validates the OEM signature but (as with Magisk-patched images) ignores
# that the hash descriptor no longer matches the rebuilt image.
#
# Hook the patch into the normal recovery build so that
#   m recoveryimage
# leaves out/target/product/825X_Pro/recovery.img directly flashable with
#   fastboot flash recovery out/target/product/825X_Pro/recovery.img
# ---------------------------------------------------------------------------
_825X_VBMETA_BLOB  := $(_825X_PATH)/prebuilts/recovery_vbmeta.bin
_825X_VBMETA_TOOL  := $(_825X_PATH)/make_flashable_recovery.sh
_825X_VBMETA_STAMP := $(PRODUCT_OUT)/.recovery_oem_vbmeta.stamp

$(_825X_VBMETA_STAMP): $(PRODUCT_OUT)/recovery.img $(_825X_VBMETA_BLOB) $(_825X_VBMETA_TOOL)
	@echo "----- Adding OEM vbmeta to recovery.img -----"
	$(hide) $(_825X_VBMETA_TOOL) $(PRODUCT_OUT)/recovery.img $(_825X_VBMETA_BLOB) $(PRODUCT_OUT)/recovery.img
	$(hide) touch $@

.PHONY: recoveryimage
recoveryimage: $(_825X_VBMETA_STAMP)

endif
