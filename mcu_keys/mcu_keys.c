/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * 825X_Pro: front-panel hardware buttons.
 *
 * The buttons on this head unit are not wired to the MTK keypad matrix.
 * They are read by the external MCU on UART4 (/dev/ttyS3) and reported to
 * Android by the vendor library libts59xhw.so (the same library the stock
 * MainUI uses through com.yyw.ts70xhw.Mcu).
 *
 * Stock MainUI polls Mcu.mcutask() and Mcu.GetPkey()/GetRkey()/GetEkey()/
 * GetSkey() and turns those vendor key codes into in-app actions.  It is not
 * installed on LineageOS, so nothing ever drains the MCU key queue and the
 * buttons appear dead even though the MCU is receiving them.  Like stock,
 * the library and this helper live on /system so vendor.img can stay stock.
 *
 * This daemon does the polling and re-publishes each key as a standard Linux
 * input event through uinput, so Android's normal key handling applies
 * (Generic.kl maps them to Android keycodes).  The mapping can be changed at
 * runtime with persist.mcu_keys.<name> properties (decimal Linux keycode,
 * 0 disables the key).
 *
 * One bit of stock MainUI behaviour is reproduced here as well: the backlight
 * is driven by the MCU, not by Android's light HAL. Stock calls BklTurn()
 * whenever a key arrives while BklisOn() says the panel is dark. Without that
 * the LCD stays off after Android blanks it on sleep.
 */

#define LOG_TAG "mcu_keys"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <log/log.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

extern int __system_property_get(const char *name, char *value);

/* From libts59xhw.so (32-bit).  These are plain C ABI functions; the stock
 * JNI wrapper just forwards to them.  Loaded at runtime so the daemon can be
 * built independently of the proprietary blob. */
typedef int (*mcu_fn)(void);
static mcu_fn mcu_init;
static mcu_fn mcu_next;
static mcu_fn mcu_task;
static mcu_fn mcu_get_pkey;
static mcu_fn mcu_get_rkey;
static mcu_fn mcu_get_ekey;
static mcu_fn mcu_get_skey;
/* Used to restore the stock MCU backlight state (see above). */
static mcu_fn mcu_bkl_is_on;
static mcu_fn mcu_bkl_turn;
/* Stock MainUI power-state handshake (see DealTask()/CheckPoweroffState()). */
static mcu_fn mcu_get_pow_state;
static mcu_fn mcu_send_bye_bye;

/* Vendor key codes from KeyDef.java in the stock MainUI. */
struct key_map {
    const char *name;
    int mcu_code;
    int keycode;
};

static struct key_map key_map[] = {
    /* front panel, panel-key codes */
    { "menu",   0x008, KEY_MENU },        /* PKEY_MENU / MENU */
    { "phone",  0x017, KEY_PHONE },        /* PKEY_BT / PHONE */
    { "phone",  0x01d, KEY_PHONE },        /* PKEY_TELCALLON */
    { "phone",  0x01e, KEY_PHONE },        /* PKEY_TELCALLOF */
    { "back",   0x016, KEY_BACK },        /* PKEY_RETURN / BACK */
    { "navi",   0x00b, KEY_HOMEPAGE },    /* PKEY_NAVI */
    { "music",  0x01c, KEY_MEDIA },       /* PKEY_MEDIA / MUSIC */
    { "mode",   0x00a, KEY_CONFIG },      /* PKEY_MODE */
    { "band",   0x02b, KEY_NEXTSONG },    /* PKEY_RADIO_BAND */
    { "sel",    0x007, KEY_ENTER },       /* PKEY_SET / SEL */
    { "volup",  0x013, KEY_VOLUMEUP },    /* PKEY_VOLUP */
    { "voldn",  0x014, KEY_VOLUMEDOWN },  /* PKEY_VOLDN */

    /* Stock MainUI treats the VOL knob press as PKEY_MUTE, not power. */
    { "mute",   0x010, KEY_MUTE },        /* PKEY_MUTE / VOL knob press */

    /* Power keys from the stock key definitions (other hardware/remote). */
    { "power",  0x046, KEY_POWER },       /* PKEY_POWER */
    { "power",  0x13a, KEY_POWER },       /* RKEY_POWER */
    { "power",  0x301, KEY_POWER },       /* SKEY_POWEWR_1 */

    /* remote/rotary-key codes used by the same panel */
    { "menu",   0x103, KEY_MENU },        /* RKEY_MENU */
    { "phone",  0x151, KEY_PHONE },        /* RKEY_BT */
    { "phone",  0x111, KEY_PHONE },        /* RKEY_TELCALLON */
    { "phone",  0x112, KEY_PHONE },        /* RKEY_TELCALLOF */
    { "back",   0x110, KEY_BACK },        /* RKEY_RETURN */
    { "navi",   0x106, KEY_HOMEPAGE },    /* RKEY_NAVI */
    { "mode",   0x105, KEY_CONFIG },      /* RKEY_MODE */
    { "band",   0x107, KEY_NEXTSONG },    /* RKEY_BAND */
    { "sel",    0x137, KEY_ENTER },       /* RKEY_MEDIA_SEL */
    { "volup",  0x10e, KEY_VOLUMEUP },    /* RKEY_VOLINC */
    { "voldn",  0x10f, KEY_VOLUMEDOWN },  /* RKEY_VOLDEC */

    /* encoder keys from the two rotary knobs */
    { "voldn",  0x201, KEY_VOLUMEDOWN },  /* EKEY_VOLDN */
    { "volup",  0x202, KEY_VOLUMEUP },    /* EKEY_VOLUP */
    { "tundn",  0x203, KEY_PREVIOUSSONG },/* EKEY_TUNDEC */
    { "tunup",  0x204, KEY_NEXTSONG },    /* EKEY_TUNINC */
};

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define NBITS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define KEY_BITS NBITS(KEY_MAX)
/* 10 * 50 ms: long enough to reject ignition glitches. */
#define ACC_OFF_TICKS 10

static volatile sig_atomic_t running = 1;

static void on_signal(int sig) {
    (void)sig;
    running = 0;
}

static int prop_int(const char *name, int def) {
    char value[92];
    int n = __system_property_get(name, value);
    if (n <= 0) return def;
    value[n] = '\0';
    int v = atoi(value);
    return v; /* 0 disables the key */
}

static void apply_properties(void) {
    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
        char name[96];
        snprintf(name, sizeof(name), "persist.mcu_keys.%s", key_map[i].name);
        int value = prop_int(name, -1);
        if (value != -1) {
            key_map[i].keycode = value;
            ALOGI("key 0x%03x (%s) -> %d (property)", key_map[i].mcu_code,
                  key_map[i].name, value);
        }
    }
}

static int create_uinput(void) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        ALOGE("cannot open /dev/uinput: %s", strerror(errno));
        return -1;
    }
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0 ||
        ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0) {
        ALOGE("UI_SET_EVBIT failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    unsigned long bits[KEY_BITS] = {0};
    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
        int code = key_map[i].keycode;
        if (code <= 0 || code >= KEY_MAX) continue;
        if (bits[code / BITS_PER_LONG] & (1UL << (code % BITS_PER_LONG))) continue;
        bits[code / BITS_PER_LONG] |= 1UL << (code % BITS_PER_LONG);
        if (ioctl(fd, UI_SET_KEYBIT, code) < 0) {
            ALOGE("UI_SET_KEYBIT(%d) failed: %s", code, strerror(errno));
        }
    }

    struct uinput_setup setup;
    memset(&setup, 0, sizeof(setup));
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "825X_Pro MCU Keys");
    setup.id.bustype = BUS_HOST;
    setup.id.vendor = 0x8257;
    setup.id.product = 0x8257;
    setup.id.version = 1;
    if (ioctl(fd, UI_DEV_SETUP, &setup) < 0) {
        ALOGE("UI_DEV_SETUP failed: %s", strerror(errno));
        close(fd);
        return -1;
    }
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        ALOGE("UI_DEV_CREATE failed: %s", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static void send_key(int fd, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = EV_KEY;
    ev.code = code;
    ev.value = value;
    if (write(fd, &ev, sizeof(ev)) != (ssize_t)sizeof(ev)) {
        ALOGE("write EV_KEY failed: %s", strerror(errno));
    }
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    if (write(fd, &ev, sizeof(ev)) != (ssize_t)sizeof(ev)) {
        ALOGE("write EV_SYN failed: %s", strerror(errno));
    }
}

/* The panel backlight is switched by the MCU over UART, not by Android's
 * light HAL. Stock MainUI calls BklTurn() when BklisOn() says the panel is
 * dark. BklTurn() cycles a 3-state value and BklisOn() reports "on" for two
 * of them, so keep toggling until the panel is reported on again. */
static void wake_backlight(void) {
    for (int tries = 0; tries < 3; tries++) {
        if (mcu_bkl_is_on() != 0) return;
        ALOGI("backlight off -> BklTurn() (%d)", tries);
        mcu_bkl_turn();
    }
    ALOGW("could not restore MCU backlight");
}

/* Stock Notifier.setSuspendState() uses this to mark quickboot transitions
 * for the bootloader. The node records the last state; it does not suspend. */
static void write_boot_state(const char *state) {
    int fd = open("/sys/boot/state", O_WRONLY);
    if (fd < 0) {
        ALOGE("open /sys/boot/state: %s", strerror(errno));
        return;
    }
    size_t len = strlen(state);
    if (write(fd, state, len) != (ssize_t)len)
        ALOGE("write /sys/boot/state: %s", strerror(errno));
    close(fd);
}

static void handle_key(int fd, int mcu_code, const char *kind) {
    if (mcu_code == 0 || mcu_code == 0x51) return; /* 0x51 = held/repeat sentinel */
    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
        if (key_map[i].mcu_code != mcu_code) continue;
        int code = key_map[i].keycode;
        if (code <= 0 || code >= KEY_MAX) return;
        ALOGI("%s key 0x%03x -> Linux key %d (%s)", kind, mcu_code, code,
              key_map[i].name);
        wake_backlight();
        send_key(fd, code, 1);
        usleep(60000);
        send_key(fd, code, 0);
        return;
    }
    ALOGW("unmapped %s key 0x%03x", kind, mcu_code);
}

static int load_mcu_library(void) {
    char path[192];
    int n = __system_property_get("persist.mcu_keys.library", path);
    if (n <= 0) {
        strlcpy(path, "/system/lib/libts59xhw.so", sizeof(path));
    } else {
        path[n] = '\0';
    }
    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        ALOGE("dlopen %s failed: %s", path, dlerror());
        return -1;
    }
    ALOGI("loaded %s", path);
    mcu_init = (mcu_fn)dlsym(handle, "Init");
    mcu_next = (mcu_fn)dlsym(handle, "mcunext");
    mcu_task = (mcu_fn)dlsym(handle, "mcutask");
    mcu_get_pkey = (mcu_fn)dlsym(handle, "GetPkey");
    mcu_get_rkey = (mcu_fn)dlsym(handle, "GetRkey");
    mcu_get_ekey = (mcu_fn)dlsym(handle, "GetEkey");
    mcu_get_skey = (mcu_fn)dlsym(handle, "GetSkey");
    mcu_bkl_is_on = (mcu_fn)dlsym(handle, "BklisOn");
    mcu_bkl_turn = (mcu_fn)dlsym(handle, "BklTurn");
    mcu_get_pow_state = (mcu_fn)dlsym(handle, "GetPowState");
    mcu_send_bye_bye = (mcu_fn)dlsym(handle, "SendByeBye");
    if (!mcu_init || !mcu_next || !mcu_task || !mcu_get_pkey || !mcu_get_rkey ||
        !mcu_get_ekey || !mcu_get_skey || !mcu_bkl_is_on || !mcu_bkl_turn ||
        !mcu_get_pow_state || !mcu_send_bye_bye) {
        ALOGE("dlsym failed: %s", dlerror());
        return -1;
    }
    return 0;
}

int main(void) {
    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);

    apply_properties();

    if (load_mcu_library() < 0) return 1;

    int rc = mcu_init();
    ALOGI("Init() = %d", rc);

    int fd = create_uinput();
    if (fd < 0) return 1;

    /* Optional self-test: set persist.mcu_keys.test to a vendor key code
     * (decimal, e.g. 8 for MENU) to inject one synthetic press at startup. */
    int test_code = prop_int("debug.mcu_keys.test", 0);
    if (test_code) {
        /* Give InputReader time to open the uinput node before injecting. */
        usleep(500000);
        handle_key(fd, test_code, "test");
    }

    int acc_handshake = prop_int("persist.mcu_keys.acc_handshake", 1);

    ALOGI("825X_Pro MCU key daemon running");
    unsigned int ticks = 0;
    unsigned int acc_off_ticks = 0;
    int acc_off = -1;
    int bye_sent = 0;
    while (running) {
        int task_rc = mcu_task();
        ticks++;
        /* MainUI calls mcunext() only after mcutask() has brought the MCU
         * link up (it opens UART4 in the first few passes). Calling it too
         * early makes c_Uart4Read() dereference a null context. */
        if (ticks == 20) ALOGI("mcunext() = %d", mcu_next());
        if (ticks % 100 == 0) ALOGI("mcutask() = %d", task_rc);

        /* Stock MainUI.GetMcuState(): state 3 is ACC off, 0/2 are ACC on. */
        int pow = mcu_get_pow_state();
        int off = !((pow & 0x2) || (pow & 0x8));
        if (off != acc_off) {
            ALOGI("ACC %s (pow=0x%02x)", off ? "off" : "on", pow);
            if (!off) write_boot_state("resume_end");
            acc_off = off;
            acc_off_ticks = 0;
            bye_sent = 0;
        }
        if (off && acc_handshake && !bye_sent &&
            ++acc_off_ticks >= ACC_OFF_TICKS) {
            write_boot_state("suspend_start");
            ALOGI("ACC off -> SendByeBye() = %d", mcu_send_bye_bye());
            send_key(fd, KEY_POWER, 1);
            usleep(60000);
            send_key(fd, KEY_POWER, 0);
            bye_sent = 1;
        }
        handle_key(fd, mcu_get_pkey(), "P");
        handle_key(fd, mcu_get_rkey(), "R");
        handle_key(fd, mcu_get_ekey(), "E");
        handle_key(fd, mcu_get_skey(), "S");
        usleep(50000);
    }

    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
