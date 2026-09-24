# Lifecycle rules

Future N3/N4 work must never silently replace FINAL evidence. A new architecture
revision receives a new case identity unless an in-place correction is explicitly
approved before archival freeze.

Once a `dss_final` package is FINAL, its source is immutable. A correction
requires a new working case, functional verification, matched synthesis,
promotion, and an INDEX lifecycle update.

Every FINAL case must contain `CASE.md`, `README.md`, `src/`, `verification/`,
and `synthesis/`. New cases must preserve the current address-contract boundary
and record source/synthesis/archive hash correspondence before comparison.
