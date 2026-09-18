# P2DOM 1x4 Single-Hop RTL port audit

```text
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
STATUS: BLOCKED_NO_SYNTHESIZABLE_1X4_BASELINE
```

The repository has no synthesizable `rtl/dss_1x4/` implementation, no 1x4
Single-Hop candidate-map producer, no 1x4 Streaming EARLY top, and no 1x4
GROUP_GLOBAL RTL core. The existing 1x4 Single-Hop GLOBAL is C++ only:
`SolutionTakePolicy::OneByFourSingleHopGlobalV1` in
`src/DynamicRepairSimulator.cpp`. Therefore there is no frozen RTL port or
synthesis boundary to which 2x2 OPT1/2/3 may be ported.

| Required audit question | Implementation truth |
|---|---|
| Same `explicit_release` semantics? | NO. The C++ policy selects row-demand capacity attempts and uses ledger allocation; it has no 2x2 nominal R/L/RB/B release action. |
| Same 2x2 three-field key? | NO. It is not a valid 1x4 hardware key. R2F records C++ demand reduction by `(usedRows, usedColumns)` with lowest `(PatternID, attemptIndex)`. |
| Donor/resource identities? | Three adjacent ownership boundaries AB, BC, CD; middle B/C have ordered left/right donor possibilities. |
| DFS depths? | Four C++ SA depths A->B->C->D; no RTL DFS. |
| Nominal actions / PatternID slots? | NOT_FROZEN in RTL. C++ candidates are capacity attempts plus PatternIDs and vary with configuration. |
| Future-donor obligation? | No 2x2 `release_req_mask` contract exists. Ledger allocation controls adjacent ownership and forbids transitive sharing. |
| Same dominance theorem? | UNPROVEN. It must be re-derived over C++ demand/ledger state before any RTL DOM1. |
| Donor identity in key? | UNRESOLVED until a 1x4 RTL candidate/ledger boundary is frozen; B/C left/right identity cannot be silently discarded. |

The C++ evidence is valuable semantic provenance but cannot be used as RTL
equivalence, state, clock-cycle, or synthesis evidence. A new 1x4 baseline
would require an explicit architecture contract for candidate representation,
PatternID/attempt order, neighbor ledger interface, selected tuple, and an
integrated EARLY/GROUP_GLOBAL hardware boundary. That is a baseline RTL project,
not an OPT1 port.

```text
1X4_SAFE_EQUIVALENCE_KEY: NOT_FROZEN
1X4_SUMMARY_BITS: NOT_FROZEN
1X4_DOM1: UNSUPPORTED_PENDING_THEOREM
1X4_MAX_SAFE_OPT_LEVEL: OPT0_NOT_IMPLEMENTED_IN_RTL
```
