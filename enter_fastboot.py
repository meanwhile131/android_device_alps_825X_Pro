#!/usr/bin/env python3
"""Put the AC8257 825X_Pro into fastboot without root.

Two paths, in order:

1. If Android or recovery is alive, use `adb reboot bootloader`.  This is the
   normal case and needs no serial port at all.
2. If the device is hung/offline, use the MTK preloader USB handshake.  The
   preloader port is owned by root:uucp (mode 0660); being in the uucp group
   is enough to open it.  The preloader re-enumerates every few seconds, so
   every read/write is retried and the port is rediscovered continuously.

Start the script FIRST, then plug in / power on the device.
"""
import shutil
import subprocess
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ModuleNotFoundError:
    sys.exit("pyserial missing: pip install pyserial  (or apt install python3-serial)")

VID, PID = 0x0E8D, 0x2000          # MediaTek preloader USB download port
CMD = b"FASTBOOT"                   # FASTBOOT / METAMETA / FACTFACT / ADVEMETA / FACTORYM
EXPECT = b"TOOBTSAF"                # preloader echoes the command reversed, after "READY"


def adb_alive():
    adb = shutil.which("adb")
    if not adb:
        return False
    try:
        out = subprocess.check_output([adb, "devices"], text=True, timeout=5,
                                      stderr=subprocess.DEVNULL)
    except (subprocess.SubprocessError, OSError):
        return False
    return any("\tdevice" in line or "\trecovery" in line for line in out.splitlines()[1:])


def reboot_via_adb():
    print("Device is alive on adb; using 'adb reboot bootloader'.")
    subprocess.run(["adb", "reboot", "bootloader"], check=False)
    fastboot = shutil.which("fastboot")
    if not fastboot:
        print("fastboot not found on PATH; check with: fastboot devices")
        return
    for _ in range(60):
        try:
            out = subprocess.check_output([fastboot, "devices"], text=True,
                                          timeout=5, stderr=subprocess.DEVNULL)
        except (subprocess.SubprocessError, OSError):
            out = ""
        if out.strip():
            print("Device is in fastboot:", out.strip())
            return
        time.sleep(0.5)
    print("adb reboot sent; if fastboot does not appear, unplug/replug and rerun this script.")


def find_port():
    for p in list_ports.comports():
        if p.vid == VID and p.pid == PID:
            return p.device
    return None


def preloader_handshake():
    print(f"Waiting for MTK preloader {VID:04x}:{PID:04x} ... (plug in / power on)")
    deadline = time.time() + 90
    seen = b""
    tries = 0

    while time.time() < deadline:
        port = find_port()
        if not port:
            time.sleep(0.05)
            continue

        s = None
        try:
            s = serial.Serial(port, 115200, timeout=0.2)
            s.write(CMD)
            tries += 1
            try:
                seen += s.read(64) or b""
            except serial.SerialException:
                pass  # device vanished mid-read: expected with this preloader
        except serial.SerialException as e:
            msg = str(e)
            if "Permission denied" in msg:
                sys.exit(
                    f"Permission denied on {port}. Your user should be in the uucp "
                    f"group for the MTK preloader port (log out/in after adding), "
                    f"and ModemManager must not hold the port. Current groups: "
                    f"{subprocess.check_output(['id', '-nG'], text=True).strip()}"
                )
            # busy / re-enumerating / read error: just retry on next loop
        finally:
            if s is not None:
                try:
                    s.close()
                except Exception:
                    pass

        if EXPECT in seen:
            print(f"Accepted after {tries} writes. Reply: {seen!r}")
            print("Preloader is switching to fastboot. Wait a few seconds, then: fastboot devices")
            return

        seen = seen[-128:]
        time.sleep(0.05)

    sys.exit("Timeout. Start the script BEFORE powering on, use a data-capable USB cable.")


def main():
    if adb_alive():
        reboot_via_adb()
    else:
        preloader_handshake()


if __name__ == "__main__":
    main()
