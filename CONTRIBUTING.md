# Contributing

Keep changes reviewable and tied to a demonstrated W200DS problem.

1. Base work on the current public branch and describe the tested firmware.
2. Follow Linux kernel coding style and run `scripts/checkpatch.pl` for kernel
   patches.
3. Preserve upstream authorship, SPDX identifiers and commit provenance.
4. Use `UPSTREAM:`, `BACKPORT:`, `FROMGIT:` or `ANDROID:` subjects where the
   Android Common Kernel guidance applies.
5. Add a `Signed-off-by:` line (`git commit -s`) to certify the Developer's
   Certificate of Origin.
6. Do not submit proprietary blobs, private modules, firmware, partition dumps,
   device identifiers, credentials or attestation data.

A change that affects boot, KernelSU ABI, the OEM module ABI, security hooks or
the boot image must include a focused test plan and a recovery path. Avoid
unrelated refactors and speculative compatibility layers.

