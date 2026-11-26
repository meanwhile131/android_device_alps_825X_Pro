/* Minimal userspace ABI for the Qualcomm QSEECOM device.
 *
 * The real header comes from the kernel that produced prebuilts/kernel; that
 * kernel source is not part of this tree.  libcryptfs_hw only needs the
 * structures/ioctl numbers below to compile, so the header is provided here
 * for the generated_kernel_headers Soong module.  On this device the
 * /dev/qseecom fallback path is unused (the vendor blob is absent), so the
 * ioctl numbers never reach a real kernel.
 */
#ifndef _UAPI_LINUX_QSEECOM_H
#define _UAPI_LINUX_QSEECOM_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define QSEECOM_HASH_SIZE 32

enum qseecom_key_management_usage_type {
	QSEECOM_KEY_MGMT_LOCK = 0x01,
	QSEECOM_KEY_MGMT_USAGE = 0x02,
	QSEECOM_KEY_MGMT_UNLOCK = 0x03,
	QSEECOM_KEY_MGMT_ID_KEY = 0x04,
	QSEECOM_KEY_MGMT_MAX = 0x05
};

struct qseecom_create_key_req {
	__u8 hash32[QSEECOM_HASH_SIZE];
	enum qseecom_key_management_usage_type usage;
};

struct qseecom_wipe_key_req {
	enum qseecom_key_management_usage_type usage;
	__u32 wipe_key_flag;
};

struct qseecom_update_key_userinfo_req {
	__u8 current_hash32[QSEECOM_HASH_SIZE];
	__u8 new_hash32[QSEECOM_HASH_SIZE];
	enum qseecom_key_management_usage_type usage;
};

struct qseecom_ice_data_t {
	__u32 flag;
};

#define QSEECOM_IOC_MAGIC 0x71

#define QSEECOM_IOCTL_CREATE_KEY_REQ \
	_IOWR(QSEECOM_IOC_MAGIC, 1, struct qseecom_create_key_req)
#define QSEECOM_IOCTL_WIPE_KEY_REQ \
	_IOWR(QSEECOM_IOC_MAGIC, 2, struct qseecom_wipe_key_req)
#define QSEECOM_IOCTL_UPDATE_KEY_USER_INFO_REQ \
	_IOWR(QSEECOM_IOC_MAGIC, 3, struct qseecom_update_key_userinfo_req)
#define QSEECOM_IOCTL_SET_ICE_INFO \
	_IOW(QSEECOM_IOC_MAGIC, 29, struct qseecom_ice_data_t)

#endif /* _UAPI_LINUX_QSEECOM_H */
