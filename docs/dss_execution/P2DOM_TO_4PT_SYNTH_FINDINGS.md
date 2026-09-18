# P2DOM to four-point synthesis findings

## FINDING_ID: P2DOM4PT-F01

```text
DATE: 2026-09-18
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
OPT_LEVEL: OPT1
OBSERVATION: Raw-map safe class filtering preserves all compared selected outputs.
DATA: group172, effect-only counterexample, 1,000 seed-20260918 maps, 0 mismatch.
CAUSE: Explicit-release is retained in the class key.
LIMITATION: The raw 40-position cursor remains, so no RTL-cycle/PPA claim exists.
PAPER_RELEVANCE: Semantic candidate evaluation reduction is distinct from cycle reduction.
```

## FINDING_ID: P2DOM4PT-F02

```text
DATE: 2026-09-18
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
OPT_LEVEL: UNRESOLVED
OBSERVATION: Existing Single-Hop GLOBAL is C++ only and uses demand/ledger semantics,
not the 2x2 explicit-release candidate interface.
DATA: P2DOM_1X4_SINGLE_HOP_RTL_PORT_AUDIT.md.
CAUSE: No synthesizable 1x4 baseline candidate/analyzer/policy/ledger boundary exists.
LIMITATION: No hardware optimization portability or PPA claim is available.
PAPER_RELEVANCE: Prevents invalid cross-topology synthesis comparison.
```
