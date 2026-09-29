# Archive policy

`dss_final/` preserves immutable experiment packages rather than serving as a
development RTL tree. Historical directories are never deleted, but they must
never be selected by default for final PPA.

| Lifecycle tag | Meaning |
|---|---|
| `CANONICAL_CA_LIVE` | primary final Continuous-Analysis package |
| `VALID_NON_CA_REFERENCE` | valid non-CA reference for CA-overhead comparison |
| `HISTORICAL_CA_SB_PROTOTYPE` | preserved StateBank prototype evidence |
| `SUPERSEDED_FOR_FINAL_CA_PPA` | retained but prohibited as default final CA PPA source |
| `ABLATION` | supporting experiment, not canonical PPA |

Each CA-LIVE package has `CASE.md`, `README.md`, `src/`, `verification/`,
`synthesis/`, `provenance/`, a closure source manifest, and a closure SHA-256
file. A closure snapshot proves source-copy identity at freeze time; it does
not manufacture missing historical run provenance.

The authoritative lifecycle lookup is [../CURRENT_N2_CANONICAL.md](../CURRENT_N2_CANONICAL.md).
