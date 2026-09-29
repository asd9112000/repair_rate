# N2 Continuous-Analysis CA-LIVE canonical family

THIS IS THE FINAL / PRIMARY N2 CONTINUOUS-ANALYSIS IMPLEMENTATION.

Do not use the older `CONTINUOUS_ANALYSIS` non-LIVE StateBank prototype for
final PPA or latency reporting. Do not substitute a non-CA canonical case when
citing CA-LIVE area or latency results.

`CA_SB` is a historical StateBank prototype with duplicated analyzer snapshots,
a wide state selector, and a mismatched synthesis boundary. `CA_LIVE` reads the
upstream authoritative state through one 272-bit live active-SA interface,
uses one canonical analyzer, and has no four-by-272-bit duplicated analyzer
state bank. Its matched PPA boundary consistently excludes upstream
fault/CAM storage.

The exact six cases and their evidence links are indexed in [INDEX.md](INDEX.md).
The authoritative cross-family decision is in
[../records/N2_CONTINUOUS_ANALYSIS_CONTRACT.md](../records/N2_CONTINUOUS_ANALYSIS_CONTRACT.md).

This archive is a closure snapshot of the working RTL roots named
`rtl/*_CONTINUOUS_ANALYSIS_LIVE_STATE_reg`. A snapshot hash proves source-copy
identity at freeze time; it does not manufacture a historical synthesis-source
hash match for rows whose historical run provenance is incomplete.
