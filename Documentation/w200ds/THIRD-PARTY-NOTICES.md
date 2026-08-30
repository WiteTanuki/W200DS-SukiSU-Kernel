# Third-party notices and provenance

This repository aggregates work with different upstream notices. It does not
replace or override them.

## Linux / Android Common Kernel and vendor-derived kernel source

The kernel tree retains `COPYING`, `LICENSES/` and per-file SPDX identifiers.
The accepted device integration was tested from commit
`df118757b7642c0c3c9e22b82a59d2dbfb03f4af`.

## SukiSU / KernelSU-derived integration

The integrated baseline is recorded as
`b1d534bc41941b2c818d7a1a1dac341e4aabfc2d`. Its source and notices are present
under `drivers/kernelsu/` and related integration files.

## KernelPatch runtime

The fixed runtime snapshot is under `tools/kernelpatch-runtime/`, revision
`1b0fddd090bc724ae5798e684acdcd1bcd31071f`. It declares GPL-2.0 and retains its
own `LICENSE`, README and credits.

## SUSFS-derived code

The 5.4 integration traces to the recorded `76affd70` lineage, with modern
contract reference `ab4c23cfc7cb26821abb7a9d2071206713c070fe`. Its upstream
GPL-3.0-or-later license text is retained at
`LICENSES/third-party/SUSFS-GPL-3.0-or-later.txt`.

The coexistence of GPL-3.0-or-later SUSFS-derived code with a GPL-2.0-only
kernel distribution has been identified as a provenance/compatibility question
for the relevant copyright holders. This release candidate preserves the
component boundary and notices, makes no claim to relicense either component,
and records the unresolved clarification instead of concealing it. A publisher
should update this notice if the authors provide a definitive permission or
compatible lineage.

## Excluded material

No Manager APK, third-party module, private LSPosed build, OEM firmware,
partition dump, FDL loader, device log, serial number, credential or private key
is part of this source repository.

