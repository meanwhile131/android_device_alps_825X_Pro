#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

# API levels
PRODUCT_SHIPPING_API_LEVEL := 28

# Health: the vendor health@2.0 service is provided by proprietary files

# Overlays
PRODUCT_ENFORCE_RRO_TARGETS := *

# Recovery adb must be always-allowed (no confirmation UI in recovery)
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.adb.secure=0

# Product characteristics
PRODUCT_CHARACTERISTICS := default

# Rootdir
PRODUCT_PACKAGES += \
    fstab.ac8257 \
    factory_init.connectivity.rc \
    factory_init.project.rc \
    factory_init.rc \
    init.ac8257.rc \
    init.ac8257.usb.rc \
    init.aee.rc \
    init.ago.rc \
    init.connectivity.rc \
    init.modem.rc \
    init.project.rc \
    meta_init.connectivity.rc \
    meta_init.modem.rc \
    meta_init.project.rc \
    meta_init.rc \
    multi_init.rc \
    init.recovery.ac8257.rc \

# Soong namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(LOCAL_PATH)

# Inherit the proprietary files
$(call inherit-product, vendor/alps/825X_Pro/825X_Pro-vendor.mk)
