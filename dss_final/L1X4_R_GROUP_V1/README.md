# L1X4_R_GROUP_V1 Frozen Final Package

This directory is a frozen evidence archive for the FINAL `L1X4_R_GROUP` case.
It is not a development source directory.

The `src/` directory is an immutable snapshot of every synthesizable RTL source
used by this frozen case. The final package intentionally carries private copies
of shared RTL dependencies so that future changes to working RTL libraries cannot
alter the archived implementation.

Development must continue under `rtl/`. Do not modify
`dss_final/L1X4_R_GROUP_V1/src/`.

## Contents

- `src/`: all eight synthesizable sources; no symlinks or mutable RTL dependencies.
- `verification/`: canonical 27-path table and functional closure summary.
- `synthesis/`: matched 20 ns TSMC018 slow-corner reports, hashes, audit, and
  both historical and self-contained manifests.

Source `synthesis/authoritative_source_manifest.tcl` for a future standalone DC
flow. It resolves only this package's `src/` files. The `*_original` manifests
are historical provenance only, not active archive dependencies.

Run `sha256sum -c synthesis/source_sha256.txt` from repository root to verify
the archived source snapshot.
