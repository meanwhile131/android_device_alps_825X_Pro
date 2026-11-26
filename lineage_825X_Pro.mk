#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

# No USB-authorization UI in recovery — keep adb open
WITH_ADB_INSECURE := true

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
# LineageOS full phone config: complete app set plus telephony (SIM, SMS, APNs)
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

# Inherit from 825X_Pro device
$(call inherit-product, device/alps/825X_Pro/device.mk)

# Use the standard LineageOS launcher (Trebuchet) instead of the crashing
# "Home Sample" app pulled in by core_base.mk.  The AOSP CarLauncher needs
# android.car at runtime and crashes on this non-automotive build, so it is
# not shipped.
PRODUCT_PACKAGES += TrebuchetQuickStep
PRODUCT_PACKAGES := $(filter-out Home,$(PRODUCT_PACKAGES))

# TWRP is selected in BoardConfig.mk (RECOVERY_VARIANT := twrp); the product
# does not need vendor/omni/config/common.mk. Keep the client id property that
# the omni config used to derive from PRODUCT_GMS_CLIENTID_BASE.
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.com.google.clientidbase=android-alps \
    ro.adb.secure=0

PRODUCT_DEVICE := 825X_Pro
PRODUCT_NAME := lineage_825X_Pro
PRODUCT_BRAND := alps
PRODUCT_MODEL := 825X_Pro
PRODUCT_MANUFACTURER := alps

PRODUCT_GMS_CLIENTID_BASE := android-alps

PRODUCT_BUILD_PROP_OVERRIDES += \
    BuildDesc="full_825X_Pro-userdebug 9 PPR1.180610.011 eng.root.20200426.093427 test-keys" \
    BuildFingerprint=alps/full_ac8257_demo/ac8257_demo:9/PPR1.180610.011/root05091632:userdebug/test-keys
