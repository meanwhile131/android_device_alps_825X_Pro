# Android device tree for alps 825X_Pro (825X_Pro)

```
#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#
```

## Custom vendor partition

The vendor partition is built from the proprietary blobs in
`vendor/alps/825X_Pro` and debloated: the native libs of stock priv-app APKs
that are not shipped in LineageOS (Chrome, Google Maps, YouTube,
EasyConnected, ES_sign, SRV_Upgrade) and the factory test images/sounds are
commented out in `proprietary-files.txt` and excluded from
`825X_Pro-vendor.mk`. All HALs, firmware and config blobs are kept, including
the stock kernel modules, MediaTek HAL implementations, feature permissions and
SELinux files under `vendor/alps/825X_Pro/proprietary/` that the generic build
does not generate.

The stock vendor SELinux files live in
`vendor/alps/825X_Pro/proprietary/vendor/etc/selinux/` and are installed by
`Android.mk`. `vendor.img` is labeled with a combination of our platform
contexts and those stock vendor contexts, otherwise the MediaTek/AutoChips
binaries get the generic `vendor_file` label, init cannot exec them and the
device bootloops.

`make_flashable_vendor.py` appends the stock AVB hashtree + vbmeta + footer
from `prebuilts/vendor_avb_tail.bin` to the built sparse `vendor.img`. This is
required because LK rejects a vendor image without the OEM-signed vbmeta. It
runs as part of `m vendorimage` / `m droid`, or standalone via:

```
m flashable_vendorimage
```

Flash the generated image (raw or sparse, fastboot handles both):

```
fastboot flash vendor out/target/product/825X_Pro/vendor_flashable.img
```

Keep `BOARD_VENDORIMAGE_PARTITION_SIZE` at the stock content size: the stock
hashtree/vbmeta offsets depend on it even though our content is smaller.

## Front-panel keys, backlight and power

`mcu_keys` drains the stock MCU key queue and publishes the buttons through
uinput. It also reproduces two bits of stock MainUI behaviour that Android
does not handle on its own:

* The panel backlight is switched by the MCU over UART, not only by Android's
  light HAL. `mcu_keys` checks `Mcu.BklisOn()` on each key and calls
  `Mcu.BklTurn()` until the panel is reported on, so the display is not left
  dark after Android blanks it on sleep. It does not turn the backlight off —
  that is left to Android's normal display power handling.
* The VOL knob press is reported by this panel as `PKEY_MUTE` (`0x010`) and is
  published as `KEY_MUTE`, matching stock MainUI's mute action. The dedicated
  power codes from the stock key definitions are still published as
  `KEY_POWER`.

Use `persist.mcu_keys.power=<linux keycode>` to remap or disable it (0), and
`persist.mcu_keys.<name>` for the other buttons.

## USB mode

`init.825x_usb.rc` (installed to `/system/etc/init/`) defaults the dual-role
port to USB **device** mode ("This device" in Settings), which is what makes
adb work. The conflicting `data_role host` writes were removed from
`init.ac8257.rc` and `init.ac8257.usb.rc`.
