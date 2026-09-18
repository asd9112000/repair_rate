# P2DOM RTL experiment master

```text
P2DOM_RTL_SPEC_STATUS: COMPLETE
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH
SPEC_COMMIT: c7907b634396cd75de8a61e95e483906e831d419
DATE: 2026-09-18
BOUNDARY: completed candidate maps -> GLOBAL DFS -> selected speculative tuple
```

This append-only master index does not authorize RTL implementation. The golden
reference remains `rtl/dss_canonical/policy/global/recam_dss_canonical_global_noscratch_core.v`
(SHA-256 `40244bdc77546dc9f1c81c1aecf18c8fb8bb9d1885a8e90813d2ba242a2dd362`).

| Exp | Status | Planned RTL top | Raw history | Summary | Candidates/depth | Dominance entries/depth | Worst visits | Mean visits | Total state | Area / GE / WNS | Total latency | Equivalence |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|---|
| A | retained | `recam_dss_canonical_global_noscratch_core` (historical name) | 480 | -- | 40 | 0 | 2,625,640 | 19 | 630 | 61,751.290432 / 6,188.00 / +0.02 ns | NOT_MEASURED_BY_P2DOM | golden |
| B | PLANNED | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_global_noscratch_classcollapsed_dfs_core` | 480 | 0 retained | <=8 | 0 | 4,680 | 12 reference-model only | 630 preliminary | NOT_RUN | NOT_RUN | required |
| C | PLANNED | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_global_noscratch_summary_dfs_core` | 0 after capture | <=224 | <=8 | 0 | 4,680 | NOT_RUN | <=374 preliminary | NOT_RUN | NOT_RUN | required |
| D | PLANNED | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_global_noscratch_summary_dominance1_dfs_core` | 0 after capture | <=224 | <=8 | 1 at useful depths | <=4,680 | 10 reference-model only | <=419 preliminary | NOT_RUN | NOT_RUN | required |
| E1 | conditional on D | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_global_noscratch_summary_dominance2_dfs_core` | 0 after capture | <=224 | <=8 | 2 at useful depths | <=4,680 | NOT_RUN | NOT_FROZEN | NOT_RUN | NOT_RUN | required |
| E2 | conditional on D | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_global_noscratch_summary_dominance4_dfs_core` | 0 after capture | <=224 | <=8 | 4 at useful depths | <=4,680 | NOT_RUN | NOT_FROZEN | NOT_RUN | NOT_RUN | required |

EXP-A mean is the 1,000-map P2-DOM reference-model value (seed `20260918`),
not an RTL-cycle measurement. Its PPA is retained DC evidence from
`HARDWARE_OPTIMIZATION_LEDGER.md` for the same golden RTL SHA and boundary.

| ID | Delta and rationale | Frozen semantics | Hypothesis / decisive metric | Test / synthesis |
|---|---|---|---|---|
| A | none; reference | all canonical behavior | retained evidence is traceable | `make test_canonical_global_noscratch`; retained DC PASS |
| B | add representative suppression; retain raw maps | raw interface, tuple, topology | deterministic frontier reduction; all equivalence mismatches = 0 | planned test; synthesis NOT_AUTHORIZED_TO_RUN |
| C | replace post-capture raw history with summary | EXP-B semantics/order | lower search-epoch state; all equivalence mismatches = 0 | planned; NOT_AUTHORIZED_TO_RUN |
| D | add one earlier-failed dominance entry | EXP-C and prefix rule | P95/P99 search benefit with PPA recorded | planned; NOT_AUTHORIZED_TO_RUN |
| E | 2 then 4 entries only if D benefits | EXP-D semantics | measured Pareto point, not intuition | planned; NOT_AUTHORIZED_TO_RUN |

Initially useful dominance depths are B/C/D (1/2/3): depth A has one root and
cannot receive a distinct earlier prefix. Implementation must re-audit this.

All non-baseline variants must match repairability, selected action/PatternID/
ConfigID, donor/release/borrow, per-depth future-donor obligations, and final
state on directed witnesses, N2/F16/group172, the effect-only counterexample,
and 1,000 maps with seed `20260918`. `MISMATCHES: 0` is mandatory.

```text
compile: PASS (existing canonical GLOBAL Verilator regression)
ast/readability/comment/profile: N/A (specification only; no RTL changed)
naming: PASS (planned explicit identifiers above)
testbench: PASS (existing RTL and P2-DOM audit regressions)
toolchain: PASS for existing Verilator regressions; no new DC run
```

## Reproduction records

| Exp | Date / Git commit | Source files; RTL / synthesis top | Test command; input corpus / seed | Synthesis command | Status |
|---|---|---|---|---|---|
| A | 2026-09-18; baseline source revision retained as `3304c86`, spec commit `c7907b6` | `rtl/dss_canonical/policy/global/recam_dss_canonical_global_noscratch_core.v`; `recam_dss_canonical_global_noscratch_core` | `make test_canonical_global_noscratch`; 6 directed + 1,000 random RTL vectors | `scripts/synthesis/run_recam_canonical_rs2_global_noscratch_dc.sh` | retained mapped PASS |
| B | planned; implementation commit NOT_FROZEN | planned class-collapsed core and same-name spec; top NOT_CREATED | planned architecture-specific test; P2DOM directed maps + 1,000 candidate maps / `20260918` | planned explicit runner; NOT_AUTHORIZED_TO_RUN | PLANNED |
| C | planned; implementation commit NOT_FROZEN | planned summary core and same-name spec; top NOT_CREATED | planned architecture-specific test; same P2DOM corpus / `20260918` | planned explicit runner; NOT_AUTHORIZED_TO_RUN | PLANNED |
| D | planned; implementation commit NOT_FROZEN | planned dominance-1 core and same-name spec; top NOT_CREATED | planned architecture-specific test; same P2DOM corpus / `20260918` | planned explicit runner; NOT_AUTHORIZED_TO_RUN | PLANNED |
| E1/E2 | conditional; implementation commit NOT_FROZEN | planned dominance-2/-4 cores and specs; top NOT_CREATED | planned architecture-specific test; same P2DOM corpus / `20260918` | planned explicit runners; NOT_AUTHORIZED_TO_RUN | PLANNED_CONDITIONAL |

## P2DOM-RTL-SPEC closure summary

```text
P2DOM_RTL_SPEC_STATUS: COMPLETE
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH
BASELINE_VARIANT_DEFINED: YES
CLASS_COLLAPSE_VARIANT_DEFINED: YES
224BIT_SUMMARY_VARIANT_DEFINED: YES
DOMINANCE_VARIANT_DEFINED: YES
SUMMARY_BUILD_EXPERIMENTS_DEFINED: YES
DOMINANCE_SCALING_DEFINED: YES
STATE_ACCOUNTING_FROZEN: YES
LATENCY_ACCOUNTING_FROZEN: YES
PPA_ACCOUNTING_FROZEN: YES
CRITICAL_PATH_CLASSIFICATION_DEFINED: YES
EXPERIMENT_MASTER_CREATED: YES
FINDINGS_LOG_CREATED: YES
CHALLENGE_LOG_CREATED: YES
PRODUCTION_RTL_MODIFIED: NO
CANONICAL_GLOBAL_REFERENCE_MODIFIED: NO
P2_SCRATCH_REOPENED: NO
WITHSCRATCH_STARTED: NO
RS3_STARTED: NO
FORMAL_100K_TOUCHED: NO
DEVICE_SWEEP_STARTED: NO
NEXT_PROPOSED_PHASE: P2DOM-RTL-A
```
