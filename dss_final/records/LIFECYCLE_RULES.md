# Lifecycle rules

Future work must never silently replace archived evidence. A new architecture
revision receives a new case identity unless an in-place correction is
explicitly approved before archival freeze.

Once a `dss_final` package is archived, its source snapshot is immutable. A
correction requires a new working case, functional verification, matched
synthesis, promotion, and an INDEX lifecycle update.

For N2 Continuous-Analysis, the default final family is
`CANONICAL_CA_LIVE` under `dss_final/continuous_analysis_live/`.
`VALID_NON_CA_REFERENCE` is retained for CA-overhead comparisons; `CA_SB` is
historical/prototype evidence and is never a default final CA PPA selection.

Every canonical package contains `CASE.md`, `README.md`, `src/`,
`verification/`, and `synthesis/`; CA-LIVE packages additionally carry
`provenance/`, a closure manifest, and closure SHA-256 evidence.
