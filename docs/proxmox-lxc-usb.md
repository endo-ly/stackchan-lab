# Proxmox LXC USB Setup

This project was developed inside a Proxmox LXC container.
The stable setup is:

- Proxmox host creates `/dev/stackchan` with a udev rule.
- Proxmox passes `/dev/stackchan` into the LXC with Device Passthrough (`dev0`).
- PlatformIO uses `/dev/stackchan`.
- ESP32-S3 upload uses `usb_reset`.

## Why `/dev/stackchan`

`/dev/ttyACM0` is order-dependent. It can become `/dev/ttyACM1` after USB
reconnects or when other USB serial devices are present.

Directly bind-mounting `/dev/ttyACM0` with `lxc.mount.entry` also caused a
stale mount after USB reconnects:

```sh
findmnt -T /dev/ttyACM0
```

```text
/dev/ttyACM0 udev[/ttyACM0//deleted] ...
```

That means the container still holds the old deleted device node. Avoid that
path for normal development.

## Host: Confirm Device

Run on the Proxmox host:

```sh
ls -l /dev/ttyACM0
ls -l /dev/serial/by-id/
```

Expected device example:

```text
crw-rw---- 1 root dialout 166, 0 ... /dev/ttyACM0
usb-Espressif_USB_JTAG_serial_debug_unit_44:1B:F6:E1:FE:5C-if00 -> ../../ttyACM0
```

The important IDs for StackChan/CoreS3 were:

```text
VID: 303a
PID: 1001
```

## Host: udev Rule

Create a udev rule on the Proxmox host:

```sh
nano /etc/udev/rules.d/99-stackchan.rules
```

Use this rule:

```text
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1001", MODE="0666", SYMLINK+="stackchan"
```

Reload and trigger udev:

```sh
udevadm control --reload-rules
udevadm trigger
```

Unplug and reconnect USB, then verify:

```sh
ls -l /dev/ttyACM0
ls -l /dev/stackchan
```

Expected:

```text
crw-rw-rw- 1 root dialout 166, 0 ... /dev/ttyACM0
lrwxrwxrwx 1 root root        7 ... /dev/stackchan -> ttyACM0
```

## Security Note

`MODE="0666"` allows any host user to read/write this specific Espressif
USB serial device.

For a single-user development host this is convenient and usually acceptable.
For a shared host, prefer a narrower rule such as `MODE="0660", GROUP="dialout"`
or include the device serial number.

Serial-specific rule example:

```text
SUBSYSTEM=="tty", ENV{ID_VENDOR_ID}=="303a", ENV{ID_MODEL_ID}=="1001", ENV{ID_SERIAL_SHORT}=="44:1B:F6:E1:FE:5C", MODE="0666", SYMLINK+="stackchan"
```

Check available udev properties:

```sh
udevadm info -q property -n /dev/ttyACM0 | grep -E 'ID_VENDOR_ID|ID_MODEL_ID|ID_SERIAL_SHORT|ID_SERIAL='
```

## Host: LXC Device Passthrough

Use Proxmox Device Passthrough (`dev0`) instead of raw `lxc.mount.entry`.

For CT ID `100`:

```sh
pct set 100 --dev0 path=/dev/stackchan,mode=0666
```

If old StackChan bind-mount lines were added, remove them from the CT config.
Do not remove unrelated settings such as `/dev/net/tun`.

Remove lines like these:

```text
lxc.cgroup2.devices.allow: c 166:* rwm
lxc.mount.entry: /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_XXXXXXXX-if00 dev/ttyACM0 none bind,optional,create=file
lxc.mount.entry: /dev/ttyACM0 dev/ttyACM0 none bind,optional,create=file
```

Restart the CT:

```sh
pct shutdown 100
pct start 100
```

If shutdown does not stop it:

```sh
pct status 100
pct stop 100
pct start 100
```

## Container: Verify

Run inside the LXC:

```sh
ls -l /dev/stackchan
platformio device list
```

Expected:

```text
crw-rw-rw- ... /dev/stackchan
```

`/dev/ttyACM0` is not required for this project.

## PlatformIO Settings

`platformio.ini` contains:

```ini
monitor_port = /dev/stackchan
upload_port = /dev/stackchan
board_upload.before_reset = usb_reset
```

`board_upload.before_reset = usb_reset` is important for ESP32-S3 USB
Serial/JTAG. Without it, upload through `/dev/stackchan` may fail with:

```text
Failed to connect to ESP32-S3: Wrong boot mode detected (0x29)
```

## Build And Upload

Inside the LXC:

```sh
pio run
pio run -t upload
```

Monitor:

```sh
pio device monitor
```

If the current shell is non-interactive and `pio device monitor` fails with a
TTY error, use a normal terminal or read with pyserial.

## Fallback: `lxc.mount.entry`

Use this only if `pct set --dev0` is unavailable.

```text
lxc.cgroup2.devices.allow: c 166:* rwm
lxc.mount.entry: /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_XXXXXXXX-if00 dev/stackchan none bind,optional,create=file
```

This fallback may require a CT restart after USB reconnects.
