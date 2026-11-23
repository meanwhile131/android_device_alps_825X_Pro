/*
 * 825X_Pro: adjust the touch input device's reported ABS ranges before TWRP
 * starts, so TWRP's own RECOVERY_TOUCHSCREEN_SWAP_XY mapper produces the
 * correct coordinate transform without any changes to TWRP source.
 *
 * The driver reports raw X in 0..1024 and raw Y in 0..768, but the panel's
 * usable Y span is only about 0..607 (the rest is under the bezel). TWRP's
 * generic mapper scales each axis by the evdev abs range:
 *
 *   ui_x = raw_y * (fb_width  - 1) / max_y
 *   ui_y = raw_x * (fb_height - 1) / max_x
 *
 * With rotation 90, fb_width=768 and fb_height=1024. Setting max_y=607
 * therefore gives ui_x = raw_y * 767/607 ~ raw_y * 1.264, which stretches
 * the usable touch area over the full UI width.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>

/* Provided by bionic libc; avoids a libcutils dependency so the helper can
 * be built fully static and run before /system is mounted in recovery. */
extern int __system_property_get(const char *name, char *value);

#define DEV_INPUT "/dev/input"
#define PROP_VALUE_MAX 92

static int prop_str(const char *name, char *out, const char *def)
{
    int len = __system_property_get(name, out);
    if (len <= 0) {
        strncpy(out, def, PROP_VALUE_MAX - 1);
        out[PROP_VALUE_MAX - 1] = '\0';
        return 0;
    }
    return len;
}

static int prop_int(const char *name, int def)
{
    char value[PROP_VALUE_MAX];
    if (prop_str(name, value, "") > 0)
        return atoi(value);
    return def;
}

static int set_abs(int fd, int axis, int minimum, int maximum)
{
    struct input_absinfo abs;

    if (ioctl(fd, EVIOCGABS(axis), &abs) < 0) {
        fprintf(stderr, "touch_abs: EVIOCGABS(%d) failed: %s\n", axis, strerror(errno));
        return -1;
    }

    abs.minimum = minimum;
    abs.maximum = maximum;

    if (ioctl(fd, EVIOCSABS(axis), &abs) < 0) {
        fprintf(stderr, "touch_abs: EVIOCSABS(%d) failed: %s\n", axis, strerror(errno));
        return -1;
    }

    return 0;
}

int main(void)
{
    char wanted[PROP_VALUE_MAX];
    int max_x, max_y;
    int tries;

    prop_str("persist.twrp.touch.name", wanted, "mtk-tpd");
    max_x = prop_int("persist.twrp.touch.raw_xmax", 1024);
    max_y = prop_int("persist.twrp.touch.raw_ymax", 607);

    if (max_x <= 0 || max_y <= 0) {
        fprintf(stderr, "touch_abs: invalid abs range %d/%d\n", max_x, max_y);
        return 1;
    }

    /* ueventd may still be creating /dev/input/event* when we start. */
    for (tries = 0; tries < 40; tries++) {
        DIR *dir = opendir(DEV_INPUT);
        struct dirent *de;

        if (dir == NULL) {
            usleep(100000);
            continue;
        }

        while ((de = readdir(dir)) != NULL) {
            char path[PATH_MAX];
            char name[128];
            int fd;

            if (strncmp(de->d_name, "event", 5) != 0)
                continue;

            snprintf(path, sizeof(path), DEV_INPUT "/%s", de->d_name);
            fd = open(path, O_RDWR);
            if (fd < 0)
                continue;

            if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0)
                name[0] = '\0';

            if (strncmp(name, wanted, sizeof(name)) == 0) {
                int rc = 0;

                rc |= set_abs(fd, ABS_MT_POSITION_X, 0, max_x);
                rc |= set_abs(fd, ABS_MT_POSITION_Y, 0, max_y);
                rc |= set_abs(fd, ABS_X, 0, max_x);
                rc |= set_abs(fd, ABS_Y, 0, max_y);

                printf("touch_abs: %s (%s): x 0..%d, y 0..%d%s\n",
                       path, name, max_x, max_y, rc ? " (FAILED)" : "");
                close(fd);
                closedir(dir);
                return rc;
            }

            close(fd);
        }

        closedir(dir);
        usleep(100000);
    }

    fprintf(stderr, "touch_abs: device '%s' not found\n", wanted);
    return 1;
}
