# W200DS SukiSU Kernel

Unofficial, device-specific Android 13 kernel for the ZTE W200DS / P720P01.
It combines the tested vendor Linux 5.4.276 baseline with SukiSU (KernelSU),
SUSFS inline hooks and KernelPatch runtime support.

> [!WARNING]
> This release is only for the exact firmware fingerprint listed below. An
> unlocked bootloader and a known-good copy of your own stock `boot_a` are
> required. A wrong image can leave the device unable to boot. No warranty is
> provided.

This project is not affiliated with or endorsed by ZTE, KernelSU, SukiSU,
SUSFS, KernelPatch or Magisk.

## Tested baseline

- Device/product: ZTE W200DS / P720P01
- Firmware: `ZTE/CN_P720P01/P720P01:13/TP1A.220624.014/20250218.231611:user/release-keys`
- Android / kernel: Android 13 / `5.4.276-android12-9-g83f519fbd509`
- Slot tested: A
- SukiSU: manager/driver 40856; kernel source revision recorded in
  [SOURCE-REVISIONS.md](Documentation/w200ds/SOURCE-REVISIONS.md)
- SUSFS: 2.2.0 inline hook
- SELinux: Enforcing
- KernelPatch: runtime KPM supported; embedded KPM count is zero
- Release marker: `k11f0@`

## Release status

`v0.1.0-rc1` is a reproducible, device-tested release candidate. The accepted
boot image boots Android, is recognized by SukiSU Manager, keeps SELinux
Enforcing, loads the tested module stack after reboot, and passed the recorded
Google services and Apple Music application checks. These observations are not
a guarantee for another firmware, module set or future server-side integrity
policy.

## Repository contents

- complete corresponding kernel source and the accepted W200DS configuration;
- the fixed KernelPatch runtime source under `tools/kernelpatch-runtime/`;
- deterministic build, post-link, marker and boot repack helpers;
- component provenance, installation, recovery and limitation documentation.

SukiSU Manager, third-party root modules, OEM firmware, recovery/FDL loaders,
partition dumps and private keys are not bundled.

## Build and install

See [BUILDING.md](Documentation/w200ds/BUILDING.md) for the fixed toolchain and
reproducible pipeline. See [INSTALL.md](Documentation/w200ds/INSTALL.md) and
[RECOVERY.md](Documentation/w200ds/RECOVERY.md) before changing a device.

## Important limitations

Root hiding is best-effort. SELinux-hide and KernelSU ADB Root are not included,
and no embedded KPM is shipped. Details are in
[KNOWN-LIMITATIONS.md](Documentation/w200ds/KNOWN-LIMITATIONS.md).

## Licenses and provenance

The Linux kernel tree retains its GPL-2.0-only `COPYING` file and per-file SPDX
notices. Imported components retain their own copyright and license material;
see [THIRD-PARTY-NOTICES.md](Documentation/w200ds/THIRD-PARTY-NOTICES.md) and
[NOTICE](NOTICE). Nothing in this repository relicenses third-party work.

## Reporting issues

Include the release version, exact firmware fingerprint, reproduction steps and
sanitized logs. Remove device serials, account identifiers, attestation data,
tokens and key material. Never attach proprietary firmware or partition dumps.

