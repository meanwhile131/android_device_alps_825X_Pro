/*
 * 825X_Pro: mirror the touchscreen X axis for Android.
 *
 * The panel's reported X axis runs opposite to the UI. There is no driver
 * sysfs knob for this on the stock kernel, so grab the physical mtk-tpd
 * evdev node early in boot and republish the events through uinput with
 * ABS_X / ABS_MT_POSITION_X mirrored around the reported maximum.
 */

#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEV_INPUT "/dev/input"
#define TOUCH_NAME "mtk-tpd"

/* The panel's usable Y span ends somewhere before the reported max 768;
 * scale raw Y onto the full 768-high framebuffer. Tunable at runtime via
 * persist.touch.raw_ymax (and raw_xmax / xflip), so calibration doesn't need
 * a rebuild. */
#define SCREEN_Y_MAX 767
extern int __system_property_get(const char *name, char *value);

static int prop_int(const char *name, int def) {
    char buf[92];
    int n = __system_property_get(name, buf);
    if (n <= 0) return def;
    buf[n] = '\0';
    return atoi(buf);
}

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define NBITS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define KEY_BITS NBITS(KEY_MAX)
#define ABS_BITS NBITS(ABS_MAX)

static volatile sig_atomic_t running = 1;

static void on_signal(int sig) {
    (void)sig;
    running = 0;
}

static int open_touch(void) {
    DIR *dir = opendir(DEV_INPUT);
    if (!dir) return -1;

    struct dirent *de;
    int fd = -1;
    while ((de = readdir(dir)) != NULL) {
        char path[PATH_MAX], name[128];
        if (strncmp(de->d_name, "event", 5) != 0) continue;
        snprintf(path, sizeof(path), DEV_INPUT "/%s", de->d_name);
        int f = open(path, O_RDWR);
        if (f < 0) continue;
        if (ioctl(f, EVIOCGNAME(sizeof(name)), name) < 0) name[0] = '\0';
        if (strncmp(name, TOUCH_NAME, sizeof(name)) == 0) {
            fd = f;
            break;
        }
        close(f);
    }
    closedir(dir);
    return fd;
}

static int setup_uinput(int src, int max_x) {
    int ufd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (ufd < 0) {
        fprintf(stderr, "touch_flip: cannot open /dev/uinput: %s\n", strerror(errno));
        return -1;
    }

    if (ioctl(ufd, UI_SET_EVBIT, EV_SYN) < 0 ||
        ioctl(ufd, UI_SET_EVBIT, EV_KEY) < 0 ||
        ioctl(ufd, UI_SET_EVBIT, EV_ABS) < 0) {
        perror("touch_flip: UI_SET_EVBIT");
        goto fail;
    }

    unsigned long keybits[KEY_BITS] = {0};
    if (ioctl(src, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) < 0) {
        perror("touch_flip: EVIOCGBIT(EV_KEY)");
        goto fail;
    }
    for (int i = 0; i < KEY_MAX; i++) {
        if (keybits[i / BITS_PER_LONG] & (1UL << (i % BITS_PER_LONG)))
            if (ioctl(ufd, UI_SET_KEYBIT, i) < 0) { perror("UI_SET_KEYBIT"); goto fail; }
    }

    unsigned long absbits[ABS_BITS] = {0};
    if (ioctl(src, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) < 0) {
        perror("touch_flip: EVIOCGBIT(EV_ABS)");
        goto fail;
    }
    for (int code = 0; code < ABS_MAX; code++) {
        if (!(absbits[code / BITS_PER_LONG] & (1UL << (code % BITS_PER_LONG)))) continue;
        struct uinput_abs_setup setup = {0};
        setup.code = code;
        if (ioctl(src, EVIOCGABS(code), &setup.absinfo) < 0) {
            perror("touch_flip: EVIOCGABS");
            goto fail;
        }
        if (ioctl(ufd, UI_ABS_SETUP, &setup) < 0) {
            perror("touch_flip: UI_ABS_SETUP");
            goto fail;
        }
    }

    ioctl(ufd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "825X_Pro Touch (X flipped)");
    usetup.id.bustype = BUS_HOST;
    usetup.id.vendor = 0x8257;
    usetup.id.product = 0x8257;
    usetup.id.version = 1;
    if (ioctl(ufd, UI_DEV_SETUP, &usetup) < 0) {
        perror("touch_flip: UI_DEV_SETUP");
        goto fail;
    }
    if (ioctl(ufd, UI_DEV_CREATE) < 0) {
        perror("touch_flip: UI_DEV_CREATE");
        goto fail;
    }

    (void)max_x;
    return ufd;

fail:
    close(ufd);
    return -1;
}

int main(void) {
    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);

    int src = -1;
    for (int tries = 0; tries < 100 && running; tries++) {
        src = open_touch();
        if (src >= 0) break;
        usleep(100000);
    }
    if (src < 0) {
        fprintf(stderr, "touch_flip: %s not found\n", TOUCH_NAME);
        return 1;
    }

    struct input_absinfo xinfo;
    if (ioctl(src, EVIOCGABS(ABS_MT_POSITION_X), &xinfo) < 0) {
        perror("touch_flip: EVIOCGABS(X)");
        return 1;
    }
    int max_x = prop_int("persist.touch.raw_xmax", xinfo.maximum);
    int raw_y_max = prop_int("persist.touch.raw_ymax", 607);
    int xflip = prop_int("persist.touch.xflip", 1);
    fprintf(stderr, "touch_flip: xmax=%d ymax=%d xflip=%d\n", max_x, raw_y_max, xflip);

    if (ioctl(src, EVIOCGRAB, 1) < 0) {
        perror("touch_flip: EVIOCGRAB");
        return 1;
    }

    int ufd = setup_uinput(src, max_x);
    if (ufd < 0) {
        ioctl(src, EVIOCGRAB, 0);
        return 1;
    }

    fprintf(stderr, "touch_flip: mirroring %s X 0..%d\n", TOUCH_NAME, max_x);

    struct input_event ev;
    while (running) {
        ssize_t n = read(src, &ev, sizeof(ev));
        if (n == (ssize_t)sizeof(ev)) {
            if (ev.type == EV_ABS &&
                (ev.code == ABS_X || ev.code == ABS_MT_POSITION_X)) {
                if (xflip) ev.value = max_x - ev.value;
            } else if (ev.type == EV_ABS &&
                       (ev.code == ABS_Y || ev.code == ABS_MT_POSITION_Y)) {
                ev.value = ev.value * SCREEN_Y_MAX / raw_y_max;
                if (ev.value > SCREEN_Y_MAX) ev.value = SCREEN_Y_MAX;
            }
            if (write(ufd, &ev, sizeof(ev)) != (ssize_t)sizeof(ev)) {
                perror("touch_flip: write uinput");
                break;
            }
        } else if (n < 0 && errno != EINTR) {
            perror("touch_flip: read");
            break;
        }
    }

    ioctl(ufd, UI_DEV_DESTROY);
    close(ufd);
    ioctl(src, EVIOCGRAB, 0);
    close(src);
    return 0;
}
