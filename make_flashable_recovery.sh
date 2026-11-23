#!/bin/bash
# Add the OEM-signed AVB vbmeta to a freshly built recovery image.
#
# 825X_Pro's LK only boots recovery images carrying an OEM-signed vbmeta:
#   - test-key signed vbmeta  -> rejected, drops to fastboot
#   - no vbmeta at all        -> hangs at the boot logo
#
# LK validates the OEM signature but ignores the fact that the hash descriptor
# no longer matches our rebuilt image (this is how Magisk patches this device).
#
# Usage:
#   make_flashable_recovery.sh <recovery.img> <vbmeta.bin|stock_recovery.bin> <output.img>
#
# The second argument may be either the 1344-byte vbmeta blob shipped in
# prebuilts/ or a full stock recovery image (the blob is extracted from it).
#
# This script is invoked automatically by Android.mk when building
# 'recoveryimage'; manual invocation is only needed for ad-hoc images.

set -e

python3 - "$@" <<'PY'
import os, struct, sys

if len(sys.argv) != 4:
    sys.stderr.write(
        "usage: make_flashable_recovery.sh <recovery.img> "
        "<vbmeta.bin|stock_recovery.bin> <output.img>\n")
    sys.exit(2)

src, vbmeta_src, dst = sys.argv[1:]
PART_SIZE = 33554432          # recovery partition size (0x2000000)

content = open(src, 'rb').read()
data = open(vbmeta_src, 'rb').read()

if len(data) == 1344 and data[:4] == b'AVB0':
    vbmeta = data
else:
    # Assume a full stock recovery image.
    vbmeta = data[0x11d0000:0x11d0000 + 1344]
    if len(vbmeta) != 1344 or vbmeta[:4] != b'AVB0':
        raise SystemExit("ERROR: cannot find OEM vbmeta in %s" % vbmeta_src)

img = bytearray(PART_SIZE)
img[:len(content)] = content
img[len(content):len(content) + len(vbmeta)] = vbmeta

# AVB footer (big-endian fields) lives in the last 64 bytes of the partition.
footer = struct.pack('>4sIIQQQ', b'AVBf', 1, 0,
                     len(content), len(content), len(vbmeta)) + b'\0' * 28
img[PART_SIZE - 64:PART_SIZE] = footer

if os.path.abspath(src) == os.path.abspath(dst):
    # In-place update: write to a temporary file and rename atomically so a
    # failed run never leaves a half-patched image behind.
    tmp = dst + '.oemvbmeta.tmp'
    with open(tmp, 'wb') as f:
        f.write(img)
    os.replace(tmp, dst)
else:
    with open(dst, 'wb') as f:
        f.write(img)

print('Wrote %s (%d bytes, content %d, vbmeta %d)' %
      (dst, len(img), len(content), len(vbmeta)))
PY
