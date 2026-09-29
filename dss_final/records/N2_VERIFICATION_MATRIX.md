# N2 verification matrix

The existing non-CA archive remains preserved. The following matrix records
the six CA-LIVE cases without upgrading incomplete historical provenance.

| CA-LIVE case | Functional regression | C++/RTL or oracle verification | Latency evidence | 20 ns synthesis | Provenance |
|---|---|---|---|---|---|
| G2X2 RC EARLY | PASS | preserved CA-LIVE evidence | `state_update_to_response` | PASS | FULL |
| G2X2 RC GROUP | PASS | preserved CA-LIVE evidence | `state_update_to_response` | PASS | FULL |
| G2X2 R EARLY | PASS | 10,000-vector full-top replay; 0 mismatches | state update to commit | PASS | PARTIAL |
| G2X2 R GROUP | PASS | 10,000-vector full-top replay; 0 mismatches | final-SA update to solution ready | PASS | PARTIAL |
| L1X4 R EARLY | PASS | 10,000-vector full-top replay; 0 mismatches | state update to commit | PASS | PARTIAL |
| L1X4 R GROUP | PASS | 10,000-vector full-top replay; 0 mismatches | final-SA update to solution ready | PASS | PARTIAL |

`PARTIAL` means `SOURCE_PROVENANCE=PARTIAL` and `SOURCE_HASH_MATCH=UNKNOWN`.
Closure snapshot hashes prove current copied-source identity only; they do not
retroactively prove historical synthesis-source identity.
