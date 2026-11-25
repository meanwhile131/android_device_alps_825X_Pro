#!/usr/bin/env python3
"""Append the stock AVB tail (hashtree + vbmeta + footer) to our system image.

The 825X_Pro LK verifies the system partition against the OEM-signed vbmeta
in the vbmeta partition. A plain sparse system.img has no hashtree, vbmeta or
AVB footer, so LK rejects it and drops to fastboot.

Usage:
  make_flashable_system.py <built-sparse-system.img> <stock-system.bin> <output.img>

The output is a full-size (2 GiB) raw image ready for:
  fastboot flash system output.img
"""

import os
import struct
import subprocess
import sys
import tempfile

PART_SIZE = 2147483648  # system partition size
FOOTER = struct.Struct(">4sIIQQQ")


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)

    sparse, avb_src, dst = sys.argv[1:]
    data = open(avb_src, "rb").read()

    if len(data) < 100 * 1024 * 1024:
        # Pre-extracted tail blob: hashtree + vbmeta, no footer.
        vbmeta_size = 1728
        hashtree = data[:-vbmeta_size]
        vbmeta = data[-vbmeta_size:]
        # Stock system content size; keep in sync with BOARD_SYSTEMIMAGE_PARTITION_SIZE.
        orig_size = 2113515520
        vbmeta_off = orig_size + len(hashtree)
    else:
        # Full stock system image.
        magic, ver, res, orig_size, vbmeta_off, vbmeta_size = FOOTER.unpack(
            data[len(data) - 64 : len(data) - 64 + FOOTER.size]
        )
        if magic != b"AVBf":
            sys.exit("ERROR: stock system image has no AVB footer")
        vbmeta = data[vbmeta_off : vbmeta_off + vbmeta_size]
        hashtree = data[orig_size:vbmeta_off]
    if vbmeta[:4] != b"AVB0":
        sys.exit("ERROR: no vbmeta blob in AVB source")

    # Expand our sparse system.img to a raw ext4 image.
    fd, raw_path = tempfile.mkstemp(prefix="system_raw_")
    os.close(fd)
    try:
        subprocess.check_call(
            ["out/host/linux-x86/bin/simg2img", sparse, raw_path]
        )
        raw = open(raw_path, "rb").read()
        if len(raw) > orig_size:
            sys.exit(
                "ERROR: system image (%d bytes) exceeds stock content size (%d)"
                % (len(raw), orig_size)
            )

        img = bytearray(PART_SIZE)
        img[: len(raw)] = raw
        img[orig_size:vbmeta_off] = hashtree
        img[vbmeta_off : vbmeta_off + vbmeta_size] = vbmeta
        footer = FOOTER.pack(
            b"AVBf", 1, 0, orig_size, vbmeta_off, vbmeta_size
        ) + b"\0" * (64 - FOOTER.size)
        img[PART_SIZE - 64 :] = footer

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
