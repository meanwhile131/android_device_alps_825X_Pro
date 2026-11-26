# Headers-only stand-in for a kernel source tree.
#
# The Soong lineage_generator module "generated_kernel_includes" runs
#
#   make -C $(TARGET_KERNEL_SOURCE) O=<sbox-out-dir> ARCH=arm64 headers_install
#
# to populate the UAPI include dirs exported by "generated_kernel_headers"
# (used by libcryptfs_hw and libtinycompress).  This device ships a prebuilt
# kernel binary and has no kernel source checkout, so TARGET_PREBUILT_KERNEL
# normally makes BoardConfigKernel.mk clear TARGET_KERNEL_SOURCE -- which
# turned the command into `make -C O=<sbox-out-dir>` and failed the build.
#
# Point TARGET_KERNEL_SOURCE here instead.  Only headers_install is needed;
# no kernel is ever built from this tree.
#
# The board file keeps TARGET_PREBUILT_KERNEL set only in Android.mk (after
# soong.variables has been written but before the make-side kernel.mk is
# included), so the real kernel binary still comes from prebuilts/kernel.

.PHONY: headers_install

headers_install:
	@mkdir -p $(O)/usr/include $(O)/usr/techpack/audio/include
	@cp -R include/uapi/. $(O)/usr/include/
	@cp -R techpack/audio/include/. $(O)/usr/techpack/audio/include/
