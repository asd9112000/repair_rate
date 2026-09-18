# S1G-A2G — Complete Historical Retained-State RTL Implementation

## Executive status

S1G-A2G closes the isolated S1G-A RTL path. It adds a canonical 821-bit
retained collector bank per SA, reconstructs all A2F-derived fields, and feeds
the frozen 232-bit post-Must boundary to the existing nine-slot analyzer.
Baseline EARLY and GROUP RTL were not edited. This phase uses isolated
collector fault events; it adds neither a BIST producer nor device timing,
synthesis, repair-rate, or S1G-B work.

## Frozen A2F contract and packing

| Bits | Width | Field | Semantic ordering |
| --- | ---: | --- | --- |
| `[2:0]` | 3 | Pivot count | Valid P0..P4 prefix. |
| `[102:3]` | 100 | P0..P4 `(row[9:0], col[9:0])` | Historical append order. |
| `[106:103]` | 4 | Hybrid count | Valid H0..H10 prefix. |
| `[260:107]` | 154 | H0..H10 `(ptr[2:0], descriptor, differing[9:0])` | Historical H order. |
| `[264:261]` | 4 | Reuse count | Valid R0..R10 prefix. |
| `[484:265]` | 220 | R0..R10 `(row[9:0], col[9:0])` | Historical reuse order. |
| `[652:485]` | 168 | 12 row `(count[3:0], address[9:0])` counters | First-free/first-match identity. |
| `[820:653]` | 168 | 12 column equivalent counters | First-free/first-match identity. |

`RETAINED_COLLECTOR_BITS_PER_SA=821` and four-SA payload `3284` are checked
at elaboration. H11..H13 and R11 are excluded only because the frozen
`MAX_FAULTS=12` guarantees the first accepted fault is a Pivot and at most
eleven later accepted faults can append either store.

## Production RTL changes

| File | Role |
| --- | --- |
| `rtl/dss_v2/top/recam_dss_v2_retained_collector_bank.sv` | Coherent packing, decode, cfg/Must/count reconstruction, and update-wins generation state. |
| `rtl/dss_v2/top/recam_dss_v2_retained_analyzer_path.sv` | 232-bit serializer and frozen nine-slot analyzer plumbing. |
| `rtl/dss_v2/top/recam_dss_v2_retained_overlap_core.sv` | Isolated A→B→C→D owner/FSM and final-only ledger boundary. |

Each new production RTL file has a same-name specification document. No
existing production RTL was modified.

## Reconstruction and generation semantics

Hybrid membership is reconstructed as `pointer < K(ConfigID)`. Reuse
membership is reconstructed from the first ordered related Pivot, or all
Configs for an unrelated fault after five Pivots. RowMust and ColMust scan the
retained ordered counters using frozen Config geometry. Count is the row-count
sum and ready/full is `count < 12`.

Each accepted fault derives `retained_next` from the previous coherent packing
and commits all 821 bits on one edge. It increments `fault_generation` and
clears analysis-valid on that edge. Completion is accepted only for the
matching generation; the fault-update branch has priority over completion.

The controller allows a ledger transaction only after:

```text
test_done[owner]
&& analysis_valid[owner]
&& analysis_generation[owner] == fault_generation[owner]
&& candidate/resource feasibility
```

No provisional analysis updates the ledger; rollback is therefore unnecessary.

## RTL/reference and transition equivalence

`recam_dss_v2_retained_collector_lockstep_top` drives the unmodified
`shared_fault_collector` and retained bank with identical faults. It compares
valid-prefix payload/order, counter mapping, cfg reconstruction, Must, the
canonical 232-bit view, and nine-slot analyzer results after every transition.

```text
RTL lockstep states:             18,036
Config evaluations:             126,252
Packing mismatches:                   0
Must mismatches:                      0
232-bit view mismatches:              0
Analyzer-result mismatches:           0
Post-Must exactly 9:                  2
Membership-visible >9:            5,264
States with Hybrid payload:      10,371
States with CAM-reuse payload:    9,112
```

The test includes the one-Pivot plus eleven same-row boundary, both A2F
three-Pivot post-Must=9 witnesses, and 1,500 deterministic 12-fault traces.
Every checked state follows an actual fault transition, so it closes current
view and next-fault equivalence together. Invalid physical slots in the
historical collector have unspecified payload after clear; comparison is
therefore correctly scoped to valid-prefix payloads, as frozen by A2F.

## Restart, race, and ownership evidence

The bank test covers normal, Hybrid, and CAM-reuse same-edge
completion/update races; every update invalidates the old completion. A
post-Must=9 state occurs at the frozen twelve-fault bound, where a further
accepted fault is impossible; its latest generation and full behavior are
instead checked by lockstep.

The isolated controller test covers A generation 1→2→3, later-SA collection
while A owns the analyzer, A→B→C→D commit ordering, and 1,000 randomized
update/test-done schedules. A software reference tracks each accepted SA
generation; every observed commit has matching latest fault and analysis
generations. There were zero stale-generation commits and zero timeouts.

## Regression evidence

```text
S1GA baseline directed:         8/8 PASS
S1GA baseline functional:       1000 vectors, 0 mismatch
S1GA baseline timing-random:    1000 vectors, 0 mismatch
S1GA baseline restart stress:   PASS

A2E-A2 analyzer regression: PASS
  states=10,007; config evaluations=70,049
  membership >9=1,232; post-Must=9=2; post-Must>9=0

make test_dss_formal_latency_preflight: PASS
```

## Architectural state accounting

These are new isolated-path bits; pre-existing normal EARLY result registers
and ledger state are excluded.

| Category | Bits | Detail |
| --- | ---: | --- |
| Retained collector | 3284 | Exact `821 × 4`. |
| Generation/coherence | 36 | Fault generation 16, analysis generation 16, analysis-valid 4. |
| Candidate/result | 20 | Candidate-valid 4, PatternID bank 16. |
| Scheduler/control | 10 | Owner 2, role slot 2, FSM 2, test-done 4. |
| Debug observability | 13 | Update/ready/test events and final-ledger event. |
| Functional additional total | 3350 | Excludes debug. |
| Total additional state | 3363 | Functional plus debug. |

Original S1G-A reported 816 summary storage bits and 896 total additional
bits (883 functional). Collector-state growth is 2468 bits; total
additional-state growth is 2467 bits. This is not an area or timing estimate.

## Contract consistency

| Contract | Frozen requirement | Evidence | Result |
| --- | --- | --- | --- |
| A2D-R | Stable post-Must predicate/order | 232-bit lockstep equality | PASS |
| A2E-A2 | Nine-slot analyzer unchanged | Existing regression plus lockstep | PASS |
| A2F | Exact 821-bit state | Static checks and payload/counter lockstep | PASS |
| S1F | Update invalidates; no provisional ledger | Bank priority and final-only commit | PASS |
| S1G-A | Latest generation, A→B→C→D | Directed plus 1,000 timing-random schedules | PASS |

## Validation limitations and closure

Verilator 4.028 lint passed for all new production RTL and dependencies.
`git diff --check` passed. The readable-RTL strict delivery gate was attempted
but cannot start: both project venv and system Python are 3.8.10, while the
gate uses Python 3.9+ built-in generic type syntax. No Python installation was
changed; this limitation does not replace RTL lint or semantic regressions.

No synthesis, BIST integration, device latency, repair-rate, or S1G-B work was
performed. S1G-A2H and S1G-A2I require a separate explicit authorization.

```text
S1GA2G_STATUS:
COMPLETE

RETAINED_COLLECTOR_BITS_PER_SA:
821
RETAINED_COLLECTOR_BITS_4SA:
3284
POST_MUST_ANALYZER_INPUT_BITS:
232
POST_MUST_ANALYZER_HYBRID_SLOTS:
9
FROZEN_MAX_FAULTS:
12
RETAINED_STATE_RTL_IMPLEMENTED:
YES
PACKING_WIDTH_EXACT:
YES
PIVOT_ORDER_PRESERVED:
YES
HYBRID_ORDER_PRESERVED:
YES
COUNTER_MAPPING_PRESERVED:
YES
CAM_REUSE_ORDER_PRESERVED:
YES
HYBRID_CFG_VALID_RECONSTRUCTION:
PASS
CAM_REUSE_CFG_VALID_RECONSTRUCTION:
PASS
ROW_MUST_RECONSTRUCTION:
PASS
COL_MUST_RECONSTRUCTION:
PASS
VALID_FULL_RECONSTRUCTION:
PASS
FAULT_COUNT_RECONSTRUCTION:
PASS
CURRENT_VIEW_EQUIVALENCE:
PASS
CURRENT_VIEW_MISMATCHES:
0
ANALYZER_RESULT_EQUIVALENCE:
PASS
ANALYZER_RESULT_MISMATCHES:
0
NEXT_FAULT_TRANSITION_EQUIVALENCE:
PASS
NEXT_TRANSITION_MISMATCHES:
0
SAME_CYCLE_UPDATE_VS_DONE_PRIORITY:
PASS
LATEST_GENERATION_ONLY_COMMIT:
PASS
MULTI_GENERATION_RESTART:
PASS
A_TO_B_TO_C_TO_D:
PASS
PROVISIONAL_LEDGER_UPDATE:
NO
LEDGER_ROLLBACK:
NO
H11_H13_REACHABLE:
NO
R11_REACHABLE:
NO
A2F_PROOF_STATES_REUSED:
4
RTL_EQUIVALENCE_STATES:
18036
RANDOM_REACHABLE_STATES:
18000
TIMING_RANDOMIZED_VECTORS:
1000
S1GA_DIRECTED_REGRESSION:
PASS
S1GA_FUNCTIONAL_REGRESSION:
PASS
S1GA_TIMING_REGRESSION:
PASS
S1GA_RESTART_STRESS:
PASS
A2EA2_ANALYZER_REGRESSION:
PASS
RETAINED_COLLECTOR_BITS:
3284
GENERATION_COHERENCE_BITS:
36
CANDIDATE_RESULT_BITS:
20
SCHEDULER_CONTROL_BITS:
10
DEBUG_OBSERVABILITY_BITS:
13
TOTAL_FUNCTIONAL_STATE_BITS:
3350
TOTAL_ADDITIONAL_STATE_BITS:
3363
BASELINE_EARLY_CHANGED:
NO
GROUP_CHANGED:
NO
CONFIG_ID_CONTRACT_CHANGED:
NO
PATTERN_ID_CONTRACT_CHANGED:
NO
POST_MUST_PROJECTOR_CHANGED:
NO
S1GB_RESUMED:
NO
SYNTHESIS_RUN:
NO
PRODUCTION_RTL_SCOPE_VALID:
YES
GIT_DIFF_CHECK:
PASS
IMPLEMENTATION_DOCUMENT:
docs/dss_execution/S1GA2G_COMPLETE_RETAINED_STATE_RTL_IMPLEMENTATION.md
NEXT_RECOMMENDED_PHASE:
HUMAN_REVIEW_REQUIRED; S1G-A2H_OR_S1G-A2I_NOT_AUTHORIZED
```
