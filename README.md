# Android device tree for alps 825X_Pro (825X_Pro)

```
#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#
```

## Kernel

`boot.img` is built with the stock kernel in `prebuilts/kernel` (the GPL
`Image.gz-dtb` from the working boot image). The exact vendor GPL sources
(Linux 4.9.117 for AC8257) are synced to `kernel/alps/825X_Pro` by
`.repo/local_manifests/ac8257_demo.xml`, from the `autochips-gpl` branch of
`LibreHU/android_kernel_autochips_ac8257_4.9`; they stay in the tree for
Soong's `generated_kernel_includes` and as the source for the prebuilt.

Do not switch `boot.img` back to a source-built kernel as-is. The DTB generated
from that tree is for the UJC201/Jancar board: it reserves a 56 MB framebuffer
at 0x61800000 and places mdp at 0x65100000. This panel's LK/ARM2 firmware
expects the 825X_Pro layout (28 MB framebuffer, mdp at 0x63500000); with the
wrong layout the panel stays dark while the backlight comes on. The stock
kernel is byte-for-byte the one in the known-good `boot_flashable.img`.

`prebuilts/recovery_dtbo.img` is still used for the recovery DTBO: it is the
43511-byte slice from the stock `dtbo` partition. The bootloader rejects the
full 8 MB partition image that a source DTBO build would produce.

`m kernel` installs the prebuilt kernel to `out/target/product/825X_Pro/kernel`,
which is what `mkbootimg` packs into `boot.img` and `recovery.img`. A kernel
source rebuild is not part of the normal `m droid` flow.

The headers-only stand-in under `kernel_headers/` is no longer needed; Soong's
`generated_kernel_includes` now uses the real source tree.

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

## Verified Boot

LK rejects a vbmeta signed with the AOSP test key unless verification is
disabled, so `m vbmetaimage` / `m droid` builds
`out/target/product/825X_Pro/vbmeta.img` with `SHA256_RSA2048` and flags 3
(hashtree disabled, verification disabled). Flash it alongside the other
images:

```
fastboot flash vbmeta out/target/product/825X_Pro/vbmeta.img
```

With verification disabled the built images need no AVB tail, so `system.img`,
`vendor.img` and `recovery.img` can be flashed as produced by the build.

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
* Disconnecting the ignition wire makes the MCU report power state 3. Stock
  MainUI answers that with `Mcu.SendByeBye()`; without it the MCU hard-resets
  the unit a few seconds later. `mcu_keys` sends the same handshake after a
  500 ms debounce, marks `/sys/boot/state` as `suspend_start`/`resume_end` like
  stock's Notifier, and injects a power key so Android sleeps with the panel.
  Set `persist.mcu_keys.acc_handshake=0` to disable it.
* Full stock quick boot also needs framework patches (`PowerManagerService`,
  `Notifier`, `PhoneWindowManager`, `AlarmManagerService`, `JobServiceContext`)
  and the `QuickBootManager` app with `/vendor/etc/qb_list.xml` to stop
  non-whitelisted apps. Those are not part of LineageOS, so this port does not
  reach stock's 0.00 A deep suspend.

Use `persist.mcu_keys.power=<linux keycode>` to remap or disable it (0), and
`persist.mcu_keys.<name>` for the other buttons.

## USB mode

`init.825x_usb.rc` (installed to `/system/etc/init/`) defaults the dual-role
port to USB **device** mode ("This device" in Settings), which is what makes
adb work. The conflicting `data_role host` writes were removed from
`init.ac8257.rc` and `init.ac8257.usb.rc`.
