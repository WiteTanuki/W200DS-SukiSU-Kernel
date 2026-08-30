# Installation

This is a device-specific release. Read the recovery guide first and preserve a
known-good copy of your own stock `boot_a` outside the device.

## Preconditions

- ZTE W200DS / P720P01 on the exact supported firmware fingerprint;
- unlocked bootloader;
- slot A active and a real bootloader (`is-userspace: no`);
- boot partition size `0x4000000` (64 MiB);
- release image SHA-256 verified against `SHA256SUMS`.

Do not continue if any identity differs. Do not use `fastboot boot`, flash
another slot or write any other partition.

## Minimal bootloader flow

```text
adb reboot bootloader
fastboot getvar current-slot
fastboot getvar is-userspace
fastboot getvar partition-size:boot_a
fastboot flash boot_a w200ds-sukisu-v0.1.0-rc1-boot.img
fastboot reboot
```

The commands are documentation, not an automated installer. Verify the one
connected device yourself. After Android starts, confirm the release marker,
slot A, SELinux Enforcing and SukiSU Manager status before enabling modules.

