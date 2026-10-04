# DSS FINAL hardware archive

`dss_final/` is the immutable packaged-results archive for canonical hardware
cases; development RTL remains under `rtl/`.

For final Continuous-Analysis hardware results, use
`dss_final/continuous_analysis_live/`. Do not use
`rtl/*_CONTINUOUS_ANALYSIS_reg` for final matched CA PPA unless it is explicitly
labelled CA-LIVE. The working RTL family corresponding to the final CA
implementation is `rtl/*_CONTINUOUS_ANALYSIS_LIVE_STATE_reg`.

| Family | Lifecycle | Use |
|---|---|---|
| `NON_CA_CANONICAL` | `VALID_NON_CA_CANONICAL_REFERENCE` | CA-overhead baseline and historical architecture comparison |
| `CA_SB` | `HISTORICAL_CA_SB_PROTOTYPE` | functional/prototype history only |
| `CA_LIVE` | `CANONICAL_CA_LIVE` | primary final Continuous-Analysis PPA and latency |

Use [CURRENT_N2_CANONICAL.md](CURRENT_N2_CANONICAL.md) as the first lookup
target, [INDEX.md](INDEX.md) for package navigation, and
[N2_HARDWARE_SUMMARY.md](N2_HARDWARE_SUMMARY.md) for family-separated PPA.

For consolidation navigation, use [STATUS.md](STATUS.md),
[EVIDENCE_INDEX.md](EVIDENCE_INDEX.md), and
[BRANCH_PROVENANCE.md](BRANCH_PROVENANCE.md). These files index evidence in
place; they do not relocate or rewrite packaged results.

The separate group-level R3 repair-rate closure is indexed at
[`docs/date2026/r3/R3_FINAL_ANALYSIS_REPORT.md`](../docs/date2026/r3/R3_FINAL_ANALYSIS_REPORT.md).
It is not hardware PPA evidence and its raw sidecars remain outside this
archive.
