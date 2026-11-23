#!/usr/bin/env python3
"""Put the AC8257 825X_Pro into fastboot via the MTK preloader USB handshake.

No volume buttons needed. Start this script FIRST, then plug in / power on.
The MTK preloader re-enumerates every few seconds, so every read/write is
wrapped in retries and the port is rediscovered continuously.
"""
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ModuleNotFoundError:
    sys.exit("pyserial missing: sudo apt install python3-serial  (or pip install pyserial)")

VID, PID = 0x0E8D, 0x2000          # MediaTek preloader USB download port
CMD = b"FASTBOOT"                   # FASTBOOT / METAMETA / FACTFACT / ADVEMETA / FACTORYM
EXPECT = b"TOOBTSAF"                # preloader echoes the command reversed, after "READY"


def find_port():
    for p in list_ports.comports():
        if p.vid == VID and p.pid == PID:
            return p.device
    return None


def main():
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
                    f"Permission denied on {port}. Fix with:\n"
                    f"  sudo python3 {sys.argv[0]}\n"
                    f"  sudo usermod -aG dialout $USER   # log out/in (Arch: uucp)\n"
                    f"  sudo systemctl stop ModemManager\n"
                    f"  or udev: SUBSYSTEM==\"tty\", ATTRS{{idVendor}}==\"0e8d\", "
                    f"ATTRS{{idProduct}}==\"2000\", MODE=\"0666\""
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


if __name__ == "__main__":
    main()
