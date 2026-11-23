#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

# No USB-authorization UI in recovery — keep adb open
WITH_ADB_INSECURE := true

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit from 825X_Pro device
$(call inherit-product, device/alps/825X_Pro/device.mk)

# Inherit common TWRP/Omni stuff.
$(call inherit-product, vendor/omni/config/common.mk)

# Omni enables adb authentication for every non-eng build. `ro.` properties
# are write-once, so the later ro.adb.secure=0 from device.mk is silently
# ignored and recovery adb still asks for authorization — impossible to
# approve with a broken/absent screen. Drop the "=1" at the source.
PRODUCT_SYSTEM_DEFAULT_PROPERTIES := $(filter-out ro.adb.secure=%,$(PRODUCT_SYSTEM_DEFAULT_PROPERTIES))
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += ro.adb.secure=0

PRODUCT_DEVICE := 825X_Pro
PRODUCT_NAME := lineage_825X_Pro
PRODUCT_BRAND := alps
PRODUCT_MODEL := 825X_Pro
PRODUCT_MANUFACTURER := alps

PRODUCT_GMS_CLIENTID_BASE := android-alps

PRODUCT_BUILD_PROP_OVERRIDES += \
    BuildDesc="full_825X_Pro-userdebug 9 PPR1.180610.011 eng.root.20200426.093427 test-keys" \
    BuildFingerprint=alps/full_ac8257_demo/ac8257_demo:9/PPR1.180610.011/root05091632:userdebug/test-keys
