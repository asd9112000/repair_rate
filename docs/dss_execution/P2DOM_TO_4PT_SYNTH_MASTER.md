# P2DOM to four-point synthesis master

> Historical pre-P3-BL-RTL snapshot. It is retained for the P2DOM planning
> record, but its blocked 1×4 and boundary statuses are superseded by
> [P3_4PT_GLOBAL_OPT_AUDIT.md](P3_4PT_GLOBAL_OPT_AUDIT.md), which is the
> authoritative current four-point GLOBAL optimization decision.

```text
P2DOM_OPT_2X2: ACTIVE
P2DOM_OPT_1X4_SINGLE_HOP: BLOCKED
COMMON_OPT_LEVEL: UNRESOLVED
P3_4PT_SYNTH: BLOCKED
```

| Exp | Architecture | Opt level | Summary bits | DOM entries | Equivalence | Area | GE | WNS | Critical path | Status |
|---|---|---|---:|---:|---|---:|---:|---:|---|---|
| OPT1-2x2 | GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL | OPT1 | 0 | 0 | 1,000 maps + directed, 0 mismatch | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | FUNCTIONAL_COMPLETE |
| OPT2-2x2 | GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL | OPT2 | <=224 planned | 0 | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_STARTED |
| OPT3-2x2 | GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL | OPT3 | <=224 planned | 1 planned/useful depth | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_STARTED |
| OPT1-1x4 | LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL | UNRESOLVED | NOT_FROZEN | NOT_FROZEN | BLOCKED: no RTL baseline/key | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | BLOCKED |
| SYN-A | GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_STREAMING_EARLY | existing | N/A | N/A | retained prior evidence | retained only | retained only | retained only | retained only | NOT_RERUN |
| SYN-B/C/D | required four-point set | UNRESOLVED | UNRESOLVED | UNRESOLVED | blocked by boundary audit | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | BLOCKED |

No four-point DC run was launched. The next safe work is to freeze and
implement comparable integrated baseline RTL boundaries, beginning with a
separately authorized 1x4 Single-Hop hardware contract; it is not an OPT port.

## Required status summary

```text
P2DOM_TO_4PT_STATUS: BLOCKED
2X2_ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
2X2_MAX_SAFE_OPT_LEVEL: OPT1
2X2_SAFE_EQUIVALENCE_KEY: (explicit_release,actual_release,actual_borrow)
2X2_SUMMARY_BITS: 0 (OPT1 implementation)
2X2_DOM_ENTRIES_PER_DEPTH: 0 (OPT1 implementation)
1X4_ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
1X4_MAX_SAFE_OPT_LEVEL: BLOCKED_NO_SYNTHESIZABLE_RTL_BASELINE
1X4_SAFE_EQUIVALENCE_KEY: NOT_FROZEN
1X4_SUMMARY_BITS: NOT_FROZEN
1X4_DOM_ENTRIES_PER_DEPTH: NOT_FROZEN
COMMON_GROUP_GLOBAL_OPT_LEVEL: UNRESOLVED
SYN_A_STATUS: RETAINED_EVIDENCE_NOT_RERUN
SYN_B_STATUS: BLOCKED_NO_INTEGRATED_2X2_GLOBAL_TOP
SYN_C_STATUS: BLOCKED_NO_1X4_STREAMING_EARLY_RTL
SYN_D_STATUS: BLOCKED_NO_1X4_GROUP_GLOBAL_RTL
ALL_GROUP_GLOBAL_USE_COMMON_OPT_LEVEL: NO
ALL_BOUNDARIES_COMPARABLE: NO
ALL_EQUIVALENCE_GATES_PASS: NO (1x4 RTL gate unavailable)
WITHSCRATCH_STARTED: NO
RS3_STARTED: NO
FORMAL_100K_TOUCHED: NO
DEVICE_SWEEP_STARTED: NO
NEXT_PROPOSED_PHASE: freeze comparable integrated 1x4 and 2x2 GLOBAL RTL boundaries
```
