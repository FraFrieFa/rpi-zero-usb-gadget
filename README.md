# minimal_usb_gadget

A minimal Linux-based USB mass-storage gadget for the Raspberry Pi Zero W v1.1,
booted over USB via [rpiboot](https://github.com/raspberrypi/usbboot).

The Pi boots a tiny custom kernel + initramfs (a freestanding `init` written in
C, no libc) that exposes the SD card (`/dev/mmcblk0`) as a USB mass-storage
device using configfs. The Zero W device tree, with the required USB gadget
overlay pre-applied, is appended to the kernel image so the rpiboot directory
does not need separate DTB or overlay files. On eject it reboots.

## Build

```sh
nix build
```

The `result` symlink is an rpiboot boot directory (`bootcode.bin`, `start.elf`,
`kernel.img`, `config.txt`, `cmdline.txt`, ...).

## Deploy

Put the Pi Zero W in USB boot mode (SD card inserted, connected via the data USB
port), then serve the boot directory to it:

```sh
rpiboot -d result
```

The Pi boots the custom kernel and re-enumerates as a USB mass-storage device
exposing its SD card. On boot it waits up to 20 seconds for the SD card
(`/dev/mmcblk0`) to appear; if it never does, the Pi reboots back into
bootloader mode. Ejecting the device host-side also makes the Pi reboot.

## Layout

- `init.c` — freestanding init: mounts filesystems, sets up the configfs
  mass-storage gadget, waits for eject.
- `flake.nix` — cross-compiles the kernel + initramfs and assembles the boot dir.
- `rpizero-minimal-linux.config` — minimal kernel config (built on `allnoconfig`).
- `config.txt` / `cmdline.txt` — Pi firmware boot configuration.
