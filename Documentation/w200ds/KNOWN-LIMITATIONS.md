# Known limitations

- Only the listed W200DS / P720P01 Android 13 firmware is supported.
- Root hiding is best-effort and cannot guarantee acceptance by every app or
  future remote integrity policy.
- SELinux remains Enforcing. The Manager's **Hide SELinux modifications**
  feature is not supported; SELinux-hide is a separate, frozen research track.
- KernelSU ADB Root is not enabled. Normal ADB remains an ordinary shell unless
  the user explicitly grants Shell root through the Manager.
- KernelPatch runtime KPM load/unload was validated, but this release embeds no
  KPM and ships no KPM payload.
- The OEM `/system/bin/su` and `/system/bin/sh` alias behavior is not solved by
  replacing the kernel. Hiding the shared inode can also affect the shell.
- The accepted boot contains an unchanged ramdisk derived from the supported
  OEM boot image. Users must assess redistribution rules for the binary release.
- Module compatibility depends on the module and its configuration. Private or
  third-party modules are outside this repository.

