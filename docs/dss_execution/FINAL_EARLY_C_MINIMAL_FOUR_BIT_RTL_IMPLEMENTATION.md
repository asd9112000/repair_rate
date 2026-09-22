# FINAL-EARLY-C — Minimal Four-Bit RTL Feasibility Result

```text
FINAL_EARLY_C_STATUS: BLOCKED
STARTING_HEAD: 84951830b653a89d42d8e15ded1cf7364d0ab142
FINAL_COMMIT: NONE
```

The required production-simulator lockstep check disproved the proposed
four-token implementation. Per the FINAL-EARLY-C hard-stop rules, the
experimental RTL and test integration were withdrawn, the mother design was
not modified, and no commit was created.

## Mother provenance

The immutable mother is
`dss_final/recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch`.
Its eight-file source manifest was read before RTL work. Every archived source
is byte-identical to its canonical counterpart at the starting HEAD.

| Archived source | Canonical source | SHA-256 | Match |
| --- | --- | --- | --- |
| `dss_v2_params_pkg.sv` | `rtl/dss_v2/common/dss_v2_params_pkg.sv` | `46222a354cd9bc9464ddd5f9e0011476886f46a39220c5d7bad5aa648a5e8d3e` | YES |
| `dss_v2_types_pkg.sv` | `rtl/dss_v2/common/dss_v2_types_pkg.sv` | `06cfc6f2660664efe0753ddf37b48b6483a4640a8368011837ca1e9d51939894` | YES |
| `dss_v2_group_candidate_store.sv` | `rtl/dss_v2/group/dss_v2_group_candidate_store.sv` | `7ca8e4b2219b0d16b0b05f56d32f1c783a8b6480e0a0a78124a08b2cf007e3db` | YES |
| `dss_v2_group_slot_decode.sv` | `rtl/dss_v2/group/dss_v2_group_slot_decode.sv` | `69b713b8bf36bcc715ffefedea7b215a04fc47a613012239d83e232fe22d4fd7` | YES |
| `recam_shared_config_analyzer.sv` | `rtl/recam/recam_shared_config_analyzer.sv` | `b4aa07117af410e84c809ae833f7e926ef440a880523c78e11cd1a7df3dd5b6c` | YES |
| `recam_dss_hyp02_static_selector.sv` | `rtl/dss_hyp02/recam_dss_hyp02_static_selector.sv` | `5edc23a40e97da04c5c9e105699ec60821a507bb467cf597890c104df077b740` | YES |
| `recam_dss_hyp02_static_global_core.sv` | `rtl/dss_hyp02/recam_dss_hyp02_static_global_core.sv` | `29d18300d87ef107df64d34702dad288ba76c829f4f8bbbdf8d270dae5ccb64c` | YES |
| `recam_dss_hyp02_static_global_top.sv` | `rtl/dss_hyp02/recam_dss_hyp02_static_global_top.sv` | `c78d63e7d612b56d78d00e2fcf4d03b463813d3cb81b549f550f29795ae2578c` | YES |

```text
MOTHER_DESIGN_SOURCE_MANIFEST_VERIFIED: YES
EARLY_TOPOLOGY_MATCHES_FINAL_GROUP: YES
DSS_FINAL_MODIFIED: NO
SHARED_ANALYZER_MODIFIED: NO
```

The verified fixed topology is A_ROW→C, D_ROW→B, B_COL→A, and C_COL→D.
The attempted EARLY order was A→B→C→D with slot priority R→L→RB→B.

## GROUP-to-EARLY classification

| Block | Intended EARLY action | Result |
| --- | --- | --- |
| Shared analyzer | Keep byte-identical | Kept identical; its interface is insufficient for exact commit accounting |
| Config/slot decode | Reuse | Reused in the withdrawn prototype |
| Fixed topology | Encode as four availability tokens | Insufficient to reproduce production semantics |
| Candidate store | Remove | Removed in the withdrawn prototype |
| 81-path selector | Replace with streaming first-legal selection | Implemented only in the withdrawn prototype |
| Resource ledger | Replace with four token bits | Disproved by production lockstep |
| Commit controller | Stream A→B→C→D | Passed only against the invalid fixed-mask abstraction |

## Disproved assumption

The proposed state was four availability bits
`{C_COL,B_COL,D_ROW,A_ROW}`. It assumed that SA/slot/ConfigID determines a
constant resource-consumption mask. That assumption is false for the frozen
production simulator.

`DynamicRepairSimulator::compressedPlansForSubarray` derives each candidate's
actual `usedRows` and `usedColumns` from the decoded remap. PatternID selects a
solution orientation, but `decodeSolution` counts only source row/column
addresses that are actually present in that fault instance. The sequential
`PhysicalResourceLedger` therefore commits remap-dependent demand, not merely
the ConfigID capacity envelope. ConfigID plus PatternID is not a fixed demand
mask. The shared analyzer exposes validity and PatternID, but not the actual
row/column demand needed to update four availability tokens exactly.

## Production counterexample

With seed 20260922, production vector 216 at F_GROUP=16 selected:

| SA | Slot | ConfigID | PatternID | Actual demand |
| --- | --- | ---: | ---: | --- |
| A | R | 4 | 1 | 1 row, 1 column |
| B | R | 1 | 2 | 2 rows, 1 column |
| C | L | 0 | 2 | 2 rows, 1 column |
| D | B | 5 | 3 | 2 rows, 3 columns |

Production succeeds and D borrows `C_COL`. The fixed-mask abstraction treats
C:L/ConfigID 0 as consuming `C_COL`, rejects D:B, and therefore disagrees.
This is direct evidence that the required resource update is Pattern/remap
dependent. It is not representable by the requested four persistent bits plus
the frozen analyzer interface and fixed Config masks.

The observed mismatch directly triggers hard-stop conditions 8 and 10. An
exact repair would need newly authorized current-candidate demand/remap decode
or a changed analyzer interface, which would invoke conditions 6 and/or 11;
that alternative was not designed after the mandatory stop.

The configured `maximumGroupBorrowedSpares=1` may impose an additional history
requirement, but no workaround was pursued because the demand-dependent
counterexample already triggers the hard stop.

## Verification evidence

Command attempted before the invalid implementation was withdrawn:

```text
make test_final_early_c_minimal_four_bit
```

The fixed-mask RTL and its matching abstract reference agreed:

```text
ABSTRACT_DIRECTED_VECTORS: 26
ABSTRACT_EXHAUSTIVE_MAPS: 65536
ABSTRACT_EXHAUSTIVE_STATES: 60
ABSTRACT_EXHAUSTIVE_TRANSITIONS: 361856
ABSTRACT_EXHAUSTIVE_EVALUATIONS: 361856
ABSTRACT_EXHAUSTIVE_MISMATCHES: 0
ABSTRACT_RANDOM_LOCKSTEP_VECTORS: 1000
ABSTRACT_RANDOM_LOCKSTEP_MISMATCHES: 0
```

That result does not establish production equivalence. Against 1000 candidate
maps generated by the actual production simulator (four F_GROUP points,
250 groups each), the decisive result was:

```text
PRODUCTION_SIMULATOR_LOCKSTEP_VECTORS: 1000
PRODUCTION_SIMULATOR_LOCKSTEP_MISMATCHES: 19
RESULT: FAIL
```

## Final disposition

```text
FINAL_EARLY_TOP_DELIVERED: NO
FINAL_EARLY_CORE_DELIVERED: NO
COMMON_SPARE_POLICY_BITS_PROPOSED: 4
FOUR_BITS_SUFFICIENT_FOR_FROZEN_SEMANTICS: NO
HISTORICAL_CONFIGID_POLICY_BITS: 0
HISTORICAL_PATTERNID_POLICY_BITS: 0
BORROWER_ID_POLICY_BITS: 0
DONOR_ID_POLICY_BITS: 0
RELEASE_HISTORY_POLICY_BITS: 0
CANDIDATE_STORE_BITS: 0
GENERIC_LEDGER_ADDED: NO
DIRECTED_ABSTRACT_CHECK: PASS
EXHAUSTIVE_ABSTRACT_CONTROLLER_CHECK: PASS
PRODUCTION_SIMULATOR_LOCKSTEP: FAIL
SYNTHESIS_RUN: NO
FORMAL_REPAIR_SWEEP_RUN: NO
READY_FOR_SYNTHESIS: NO
WORKTREE_CLEAN_AFTER_COMMIT: NO
FINAL_COMMIT: NONE
```

Proceeding requires an explicit semantic change, such as making production
commit Config-envelope demand, exposing exact candidate demand/remap metadata
through the analyzer path, or allowing more policy state. Each changes a
frozen FINAL-EARLY-C premise, so none was applied implicitly.

## Required final status

```text
FINAL_EARLY_C_STATUS: BLOCKED
STARTING_HEAD: 84951830b653a89d42d8e15ded1cf7364d0ab142
FINAL_COMMIT: NONE
MOTHER_DESIGN: recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch
MOTHER_DESIGN_SOURCE_MANIFEST_VERIFIED: YES
DSS_FINAL_MODIFIED: NO
FINAL_EARLY_TOP: NOT DELIVERED; WITHDRAWN AFTER LOCKSTEP FAILURE
FINAL_EARLY_CORE: NOT DELIVERED; WITHDRAWN AFTER LOCKSTEP FAILURE
SA_ORDER: A,B,C,D
ROLE_SLOT_ORDER: R,L,RB,B
SLOT_SEQUENCE: {1,0,3,2}
FIXED_TOPOLOGY: A_ROW->C; D_ROW->B; B_COL->A; C_COL->D
EARLY_TOPOLOGY_MATCHES_FINAL_GROUP: YES
COMMON_SPARE_POLICY_BITS: 4 IN WITHDRAWN PROTOTYPE; INSUFFICIENT
HISTORICAL_CONFIGID_POLICY_BITS: 0
HISTORICAL_PATTERNID_POLICY_BITS: 0
BORROWER_ID_POLICY_BITS: 0
DONOR_ID_POLICY_BITS: 0
RELEASE_HISTORY_POLICY_BITS: 0
GENERIC_LEDGER_PRESENT: NO
CANDIDATE_STORE_PRESENT: NO
DYNAMIC_RESOURCE_INDEXING_PRESENT: NO
VARIABLE_RESOURCE_PART_SELECT_PRESENT: NO
RUNTIME_DONOR_SEARCH_PRESENT: NO
SHARED_ANALYZER_MODIFIED: NO
SAME_ANALYZER_IMPLEMENTATION_AS_GROUP: YES
SAME_CONFIG_ENCODING_AS_GROUP: YES
SAME_PATTERN_ENCODING_AS_GROUP: YES
SAME_FIXED_TOPOLOGY_AS_GROUP: YES
GROUP_BASELINE_FILES_REUSED_IDENTICALLY: params package, types package, shared analyzer, slot decode
GROUP_BASELINE_FILES_ADAPTED: NONE
GROUP_ONLY_FILES_REMOVED: candidate store, 81-path selector, group-global core/control
NEW_EARLY_ONLY_FILES: NONE DELIVERED; PROTOTYPE CORE/TOP WITHDRAWN
STREAMING_PER_SA_COMMIT: PASS IN ABSTRACT PROTOTYPE
NO_ROLLBACK: PASS IN ABSTRACT PROTOTYPE
FAILURE_PREFIX_SEMANTICS: PASS IN ABSTRACT PROTOTYPE
DIRECTED_PRIORITY_TESTS: PASS IN ABSTRACT PROTOTYPE
FOUR_BIT_EQUIVALENCE: FAIL
EXHAUSTIVE_CONTROLLER_EQUIVALENCE: FAIL
EXHAUSTIVE_STATES: 60 ABSTRACT STATES
EXHAUSTIVE_TRANSITIONS: 361856 ABSTRACT TRANSITIONS
EXHAUSTIVE_EVALUATIONS: 361856 ABSTRACT EVALUATIONS
EXHAUSTIVE_MISMATCHES: 0 AGAINST INVALID FIXED-MASK ABSTRACTION; PRODUCTION COUNTEREXAMPLE EXISTS
RANDOM_LOCKSTEP: FAIL
RANDOM_LOCKSTEP_VECTORS: 1000
RANDOM_LOCKSTEP_MISMATCHES: 19
RTL_HARDWARE_FRIENDLINESS_AUDIT: PARTIAL; STRUCTURALLY CLEAN PROTOTYPE WAS SEMANTICALLY INVALID
POLICY_SPECIFIC_DIFFERENCE_LIMITED_TO_SELECTION_RESOURCE_COMMIT: PARTIAL
SYNTHESIS_RUN: NO
FORMAL_REPAIR_RATE_SWEEP_RUN: NO
WORKTREE_CLEAN_AFTER_COMMIT: NO; NO COMMIT CREATED AND THIS BLOCKER REPORT IS UNTRACKED
IMPLEMENTATION_READY_FOR_SYNTHESIS: NO
NEXT_ACTION: STOP FOR HUMAN RTL ARCHITECTURE REVIEW; RESOLVE THE FROZEN DEMAND/ANALYZER CONTRACT BEFORE FINAL-EARLY-D
```

The additional non-policy issue behind the `PARTIAL` answer is that exact
production resource commit needs actual decoded demand/remap information that
the unchanged analyzer interface does not provide.

```text
VERILOG_SKILL_RULES_USED:
- Read readable-verilog-generator/SKILL.md completely.
- Read references/rules/asic-verilog-quality.md completely.
- Applied the written synthesizable-RTL, explicit-state, fixed-width,
  constant-decode, and hardware-friendliness rules manually.
- Did not modify the skill package despite the known Python 3.8 dict[str,...]
  incompatibility.
```
