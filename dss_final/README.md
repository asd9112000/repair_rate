# DSS FINAL hardware archive

`dss_final/` is the immutable packaged-results archive for canonical hardware
cases; development RTL remains under `rtl/`. The current N2 set is the seven
packages marked `FINAL` in [INDEX.md](INDEX.md). Historical and superseded
packages remain preserved at top level, but are not current comparators.

Use [N2_HARDWARE_SUMMARY.md](N2_HARDWARE_SUMMARY.md) for final N2 PPA and
matched comparisons. Use [records/N2_ARCHITECTURE_CONTRACT.md](records/N2_ARCHITECTURE_CONTRACT.md)
for address, EARLY, and GROUP semantics; case packages provide `src/`,
`verification/`, and `synthesis/` evidence.

Do not use historical 5-bit RECAM PPA as the matched final baseline. Do not use
pre-storage-audit EARLY PPA as canonical. Every FINAL package is self-contained
and must not be edited in place.
