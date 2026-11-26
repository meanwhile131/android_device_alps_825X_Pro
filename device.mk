#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

# API levels
PRODUCT_SHIPPING_API_LEVEL := 28

# Health: the vendor health@2.0 service is provided by proprietary files

# The stock vendor only has the MediaTek power HAL 2.0; Android's
# PowerManagerService asks for android.hardware.power@1.0, so build and start
# the standard 1.0 passthrough service as well.
PRODUCT_PACKAGES += android.hardware.power@1.0-service
PRODUCT_COPY_FILES += \
    device/alps/825X_Pro/android.hardware.power@1.0-service.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/android.hardware.power@1.0-service.rc \
    device/alps/825X_Pro/init.825x_bringup.rc:system/etc/init/init.825x_bringup.rc \
    device/alps/825X_Pro/permissions/android.software.print.xml:system/etc/permissions/android.software.print.xml

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

# Use the normal Android shell for adbd; /sbin/bash only exists in the
# recovery ramdisk and breaks `adb shell` once Android is running.
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.sys.adb.shell=/system/bin/sh

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
    init.825x_supplicant.rc \
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
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.wlan.gen=gen4m \
    ro.lockscreen.disable.default=true
