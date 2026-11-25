#!/usr/bin/env python3
"""Append the stock AVB tail (hashtree + vbmeta + footer) to our vendor image.

The 825X_Pro LK verifies the vendor partition against the OEM-signed vbmeta in
the vbmeta partition. A plain sparse vendor.img has no hashtree, vbmeta or AVB
footer, so LK rejects it and drops to fastboot.

Usage:
  make_flashable_vendor.py <built-sparse-vendor.img> <stock-vendor> <output.img>

<stock-vendor> is the pre-extracted tail blob
(prebuilts/vendor_avb_tail.bin). LK validates the OEM signature but ignores
that the hashtree no longer matches our rebuilt contents.

The output is a full-size (1 GiB) raw image ready for:
  fastboot flash vendor output.img
"""

import os
import struct
import subprocess
import sys
import tempfile

PART_SIZE = 1073741824  # vendor partition size
ORIG_SIZE = 1056714752  # stock vendor content size; keep in sync with
                        # BOARD_VENDORIMAGE_PARTITION_SIZE
VBMETA_SIZE = 512       # stock vendor vbmeta blob size
FOOTER = struct.Struct(">4sIIQQQ")


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)

    sparse, avb_src, dst = sys.argv[1:]
    data = open(avb_src, "rb").read()

    if len(data) < 100 * 1024 * 1024:
        # Pre-extracted tail blob: hashtree + vbmeta, no footer.
        orig_size = ORIG_SIZE
        hashtree = data[:-VBMETA_SIZE]
        vbmeta = data[-VBMETA_SIZE:]
        vbmeta_off = orig_size + len(hashtree)
        vbmeta_size = len(vbmeta)
    else:
        magic, ver, res, orig_size, vbmeta_off, vbmeta_size = FOOTER.unpack(
            data[len(data) - 64: len(data) - 64 + FOOTER.size]
        )
        if magic != b"AVBf":
            sys.exit("ERROR: stock vendor image has no AVB footer")
        vbmeta = data[vbmeta_off:vbmeta_off + vbmeta_size]
        hashtree = data[orig_size:vbmeta_off]
    if vbmeta[:4] != b"AVB0":
        sys.exit("ERROR: no vbmeta blob in AVB source")

    fd, raw_path = tempfile.mkstemp(prefix="vendor_raw_")
    os.close(fd)
    try:
        subprocess.check_call(
            ["out/host/linux-x86/bin/simg2img", sparse, raw_path]
        )
        raw = open(raw_path, "rb").read()
        if len(raw) > orig_size:
            sys.exit(
                "ERROR: vendor image (%d bytes) exceeds stock content size (%d)"
                % (len(raw), orig_size)
            )

        img = bytearray(PART_SIZE)
        img[:len(raw)] = raw
        img[orig_size:vbmeta_off] = hashtree
        img[vbmeta_off:vbmeta_off + vbmeta_size] = vbmeta
        footer = FOOTER.pack(
            b"AVBf", 1, 0, orig_size, vbmeta_off, vbmeta_size
        ) + b"\0" * (64 - FOOTER.size)
        img[PART_SIZE - 64:] = footer

        tmp = dst + ".tmp"
        with open(tmp, "wb") as out:
            out.write(img)
        os.replace(tmp, dst)
        print(
            "Wrote %s (%d bytes, content %d, hashtree %d, vbmeta %d)"
            % (dst, len(img), len(raw), len(hashtree), len(vbmeta))
        )
    finally:
        os.unlink(raw_path)


if __name__ == "__main__":
    main()
