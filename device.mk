#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

# API levels
PRODUCT_SHIPPING_API_LEVEL := 28

# Health: the vendor health@2.0 service is provided by proprietary files

# Overlays
PRODUCT_ENFORCE_RRO_TARGETS := *
PRODUCT_PACKAGE_OVERLAYS += device/alps/825X_Pro/overlay
# settingsprovider uses private APIs; keep this overlay static instead of
# letting PRODUCT_ENFORCE_RRO_TARGETS generate an RRO that fails sdk_check.
PRODUCT_ENFORCE_RRO_EXCLUDED_OVERLAYS += \
    device/alps/825X_Pro/overlay/frameworks/base/packages/SettingsProvider \
    device/alps/825X_Pro/overlay/packages/apps/Trebuchet

# Front-panel hardware buttons are read from the external MCU by the stock
# libts59xhw.so; mcu_keys polls it and republishes the keys via uinput.
PRODUCT_PACKAGES += \
    mcu_keys

# Recovery adb must be always-allowed (no confirmation UI in recovery)
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.adb.secure=0
# Enable adb by default so a connected PC is detected after a fresh boot.
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.sys.usb.config=adb
# Stock vendor priv-apps request privileged permissions that aren't in the
# LineageOS privapp-permissions whitelist; enforcing that check kills
# system_server in a boot loop.
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.control_privapp_permissions=disable

# adbd execs /sbin/sh with a lone "-" for interactive shells. Our recovery
# ramdisk has a stale `sh -> recovery` symlink, which makes the recovery
# binary try to open a file literally named "-" and print
# "Problem opening - for reading!". Point adbd at the real bash binary that
# TWRP ships in /sbin instead.
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.sys.adb.shell=/sbin/bash

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
    init.825x_usb.rc \
    init.825x_touch.rc \
    init.825x_keys.rc \
    touch_abs \
    touch_flip \

# Soong namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(LOCAL_PATH)

# Inherit the proprietary files
$(call inherit-product, vendor/alps/825X_Pro/825X_Pro-vendor.mk)

# Stock vendor properties. TARGET_VENDOR_PROP is obsolete in this build
# system; with BOARD_PROPERTY_OVERRIDES_SPLIT_ENABLED=true these land in
# vendor/build.prop. ro.vendor.wlan.gen selects the Wi-Fi kernel module.
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.wlan.gen=gen4m
