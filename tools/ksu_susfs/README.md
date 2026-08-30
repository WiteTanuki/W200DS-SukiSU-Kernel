# ksu_susfs cumulative R2 tool

This directory contains the cumulative userspace client for the SUSFS UAPI.
The current client exposes `add_sus_path`, `add_sus_path_loop`, static kstat,
dynamic kstat add/update, dynamic full-clone update, modern single
`add_sus_map`, v2.2 open redirect schemes 0-4, AVC audit-log spoofing, and
version/variant/enabled-feature queries.

Build for Android arm64 with an Android NDK:

```sh
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk
```

The wire constants, 260-byte SUS_PATH request, and 376-byte arm64 kstat request
match the audited SUSFS4KSU v2.2 contract. The ctime-seconds flag uses bit 8;
the upstream `(1 < 8)` typo is intentionally not reproduced.

SUS_MAP hides file-backed mappings from the upstream v2.2 proc interfaces for
an umounted app process. It does not hide anonymous memory or injection hooks.

Open redirect accepts only existing, non-FUSE files. Reverse lookup spoofing
covers readlink, proc fd links/fdinfo, maps and statfs for an umounted app.
AVC spoofing changes only the rendered audit target context; it does not change
the SELinux access decision.
