# R3 normalized 1k metadata and witness closure

## Status

```text
REPAIR_OUTCOMES_CHANGED: NO
FULL_1K_RERUN_REQUIRED: NO
METADATA_EMISSION_FIXED: YES
METADATA_RECONSTRUCTION_DETERMINISTIC: YES
UNMAPPABLE_RESULTS: 0
AMBIGUOUS_MAPPINGS: 0
LOCAL_BASELINE_TAXONOMY_FIXED: YES
PAIR_GLOBAL_TAXONOMY_FIXED: YES
FRESH_METADATA_SMOKE: PASS
DIRECTIONAL_LF_TO_EARLY_WITNESS: PASS
PAIRWISE_LF_TO_EARLY_WITNESS: PASS
SINGLE_HOP_LF_TO_EARLY_WITNESS: PASS
TWO_PAIRWISE_LF_TO_EARLY_WITNESS: PASS
DIRECTIONAL_EARLY_TO_GLOBAL_WITNESS: PASS
PAIRWISE_EARLY_TO_GLOBAL_WITNESS: PASS
SINGLE_HOP_SEARCH_GAIN_OBSERVED: NO
TWO_PAIRWISE_SEARCH_GAIN_OBSERVED: NO
EARLY_PASS_GLOBAL_FAIL_TOTAL: 0
SAFE_TO_INTERPRET_NORMALIZED_1K: YES
SAFE_TO_PLAN_FORMAL_SWEEP: YES
FORMAL_100K_DATA_TOUCHED: NO
HISTORICAL_1K_RAW_RESULTS_REWRITTEN: NO
```

The result root remains a **quick 1k validation dataset**, not formal
publication data.  No policy solver, corpus row, or historical raw sidecar was
rewritten.

## Metadata contract

Future `paired_policy_results_v1.csv` sidecars now emit distinct
`canonical_policy_id` and `implementation_policy_id`, plus all required
semantic, physical, and corpus fields.  `sharing_policy` retains the historical
internal spelling for compatibility; the new `topology` field is normalized
(`edge`, rather than `pairwise_edge`).

Normalized descriptors (`canonical | implementation | candidate contract |
priority | search scope | backtracking`):

```text
local_no_sharing | legacy | LOCAL | LOCAL_ONLY | LOCAL | false
directional_m1_local_first | directional_v2_early | DIRECTIONAL_V2 | LOCAL_FIRST | SEQUENTIAL_FIRST_LEGAL | false
directional_m1_early | group_greedy_rtl_canonical | DIRECTIONAL_V2 | RELEASE_AWARE | SEQUENTIAL_FIRST_LEGAL | false
directional_m1_global | directional_v2_group_global | DIRECTIONAL_V2 | JOINT_ORACLE | JOINT_COMPLETE_TUPLE_SEARCH | true
pairwise_row_m1_local_first | local_first | GENERIC_RECAM | LOCAL_FIRST | SEQUENTIAL_FIRST_LEGAL | false
pairwise_row_m1_early | early | GENERIC_RECAM | RELEASE_AWARE | SEQUENTIAL_RANKED_COMMIT | false
pairwise_row_m1_global | group_global | GENERIC_RECAM | JOINT_ORACLE | JOINT_GENERIC_GROUP_SEARCH | true
single_hop_m1_local_first | one_by_four_single_hop_early_v1 | R1B_1X4 | LOCAL_FIRST | SEQUENTIAL_FIRST_LEGAL | false
single_hop_m1_early | one_by_four_single_hop_release_aware_early_v1 | R1B_1X4 | RELEASE_AWARE | SEQUENTIAL_RANKED_COMMIT | false
single_hop_m1_global | one_by_four_single_hop_global_v1 | R1B_1X4 | JOINT_ORACLE | JOINT_COMPLETE_TUPLE_SEARCH | true
two_pairwise_m1_local_first | one_by_four_two_pairwise_early_v1 | R1B_1X4 | LOCAL_FIRST | SEQUENTIAL_FIRST_LEGAL | false
two_pairwise_m1_early | one_by_four_two_pairwise_release_aware_early_v1 | R1B_1X4 | RELEASE_AWARE | SEQUENTIAL_RANKED_COMMIT | false
two_pairwise_m1_pair_global | one_by_four_two_pairwise_pair_global_v1 | R1B_1X4 | PAIR_JOINT_ORACLE | PAIR_COMPLETE_TUPLE_SEARCH | true
```

The baseline is emitted as `LOCAL / LOCAL / LOCAL_ONLY / LOCAL`.  The
two-pair global policy is emitted as `PAIR_GLOBAL`, never generic `GLOBAL`.
`paper_canonical` and `legacy_alias_of` are explicit fields; the canonical
matrix does not expose an alias as a canonical ID.

`scripts/group/reconstruct_r3_normalized_metadata.py` deterministically mapped
all 252,000 frozen rows into
`tmp/repair_rate_matrix_v2_normalized_1k/aggregate/r3_normalized_policy_metadata.csv`.
It obtains the policy descriptor from the frozen manifest and hashes the
point-level paired corpus.  This is a derived layer only.

The fresh one-group smoke at
`tmp/r3_metadata_smoke_n2_f8_1g_closure_20260918` covered all 13 N=2 canonical
policies; `tests/r3_metadata_contract_test.py` passed.

## Isolated witness replay provenance

The replay root is `tmp/r3_normalized_1k_witness_replay_seeded_20260918`.
It contains 12 full `attempts.csv`, `runs.csv`, and policy-result sidecars.
Each replay used a flat fault file materialized from exactly one frozen row,
with the original point seed.  It generated no random faults and did not write
under the frozen root.

```text
n2_f12 corpus_id=dss_paired_corpus_v1-4c02dbbde3adcf22
  SHA256=df03a7489f0bd146ed75f5be05c17aaea6b81e75fbe406ff6fb7729e9e28ea8d
  seed=20262934; groups 738, 718, 49
n2_f16 corpus_id=dss_paired_corpus_v1-d579abc9146098f8
  SHA256=244655756fcf5946bbf6927cc24a98f73a9273c47c721f39c9093e78a7933cdb
  seed=20262938; groups 475, 172
```

The ledger notation below is the `runs.csv` physical available `(rows|columns)`
state after A, B, C, D.  A dash means the sequential policy stopped before
that SA.  Candidate attempt order and the full valid-candidate sets are
preserved in the adjacent `attempts.csv`; `selected_attempt_*` and
`selected_candidate_*` in `runs.csv` identify the committed entries.

### LOCAL_FIRST to EARLY

| Topology / frozen group | faults A/B/C/D | LOCAL_FIRST | EARLY | first divergence and demonstrated rescue |
|---|---:|---|---|---|
| directional, N2 F12 G738 | 2/1/6/3 | selected config/pattern `0/1, 0/1`; ledger `6|8 -> 5|8 -> -`; fail at C | `4/1, 1/1, 2/3, 4/1`; ledger `7|7 -> 6|7 -> 3|5 -> 2|3`; borrow `1R`; pass | A selects a different V2 config: EARLY releases a row at A, leaving a legal C config.  This is release-aware selection, demonstrated by the first ledger transition. |
| pairwise-row, N2 F16 G475 | 2/3/3/8 | generic attempts `0,0,0,-`; candidate `0,1,0,-`; `6|8 -> 4|7 -> 2|6 -> -`; fail at D | attempts `0,0,1,1`; candidate `0,4,0,2`; `6|8 -> 5|7 -> 3|7 -> 0|5`; borrow `1R`; pass | At B, EARLY commits attempt 0/candidate 4 instead of local-first candidate 1, preserving one row for D.  ConfigID/PatternID are not applicable to GENERIC_RECAM. |
| single-hop, N2 F12 G718 | 2/2/8/0 | attempts `0,0,-,-`; candidate `0,0,-,-`; `6|8 -> 4|8 -> -`; fail at C | attempts `0,0,2,0`; candidate `5,5,12,0`; `8|6 -> 8|4 -> 4|2 -> 4|2`; borrow `2R`; pass | EARLY uses the release-aware 1x4 candidate ranking from A onward; the ledger retains row capacity for C.  ConfigID/PatternID are not applicable to R1B generic candidates. |
| two-pairwise, N2 F12 G49 | 3/7/0/2 | attempts `0,-,-,-`; candidate `0,-,-,-`; `6|7 -> -`; fail at B | attempts `0,1,0,0`; candidate `2,2,0,5`; `7|6 -> 4|4 -> 4|4 -> 4|2`; borrow `1R`; pass | EARLY's different A candidate releases capacity, making B attempt 1 legal.  This is a paired-topology release-aware rescue, not a claim of global-search gain. |

The directional selected ConfigID/PatternID values are from the replayed
policy sidecar.  The non-directional candidate IDs above are the solver's
generic candidate indices, not V2 ConfigIDs or PatternIDs.  Per-SA candidate
order, fault count, selected index, resource use, and validity bitmap are
available in the stated `attempts.csv`/`runs.csv` artifacts.

### EARLY to GLOBAL

| Topology / frozen group | faults A/B/C/D | EARLY sequential commitment | GLOBAL tuple / ledger | classification |
|---|---:|---|---|---|
| directional, N2 F16 G172 | 5/0/7/4 | configs/patterns `0/3, 1/1, -/-`; `6|6 -> 6|6 -> -`; fails at C | `5/7, 0/1, 2/3, 0/1`; `7|5 -> 7|5 -> 4|3 -> 2|3`; borrow `1R,1C`; pass | The first irreversible sequential A choice prevents a feasible C continuation.  Joint search selects the complete V2 tuple, including A config 5, and succeeds. |
| pairwise-row, N2 F12 G738 | 2/1/6/3 | attempts/candidates `0/0,0/0,-/-,-/-`; `6|8 -> 5|8 -> -`; fails at C | tuple candidate indices `1,0,2,2`; borrow `1R`; pass | Joint generic group search finds a complete legal combination whereas the EARLY A/B commits have no C continuation.  V2 ConfigID/PatternID are not applicable. |

For the generic group-global path, `runs.csv` records the chosen complete
tuple but does not expose a sequential `remaining_after_*` ledger; the
authoritative final ledger is the paired policy sidecar (`borrowed=1R`,
`remaining=2R,3C`).  This is intentionally reported as a generic-contract
trace, not relabeled as directional V2.

## Paired 1k summary

All counts compare the same corpus rows.  `LF+ / E-` and `E+ / G-` are zero
for every topology.

| topology | max priority pp | max search pp | max total pp | LF- / E+ | LF+ / E- | E- / G+ | E+ / G- |
|---|---:|---:|---:|---:|---:|---:|---:|
| directional | 7.9 (N2 F24) | 1.9 (N2 F32) | 8.9 (N2 F32) | 359 | 0 | 46 | 0 |
| edge | 1.0 (N3 F28) | 2.3 (N2 F24) | 2.5 (N2 F24) | 44 | 0 | 128 | 0 |
| single-hop | 6.5 (N2 F24) | 0.0 | 6.5 (N2 F24) | 358 | 0 | 0 | 0 |
| two-pairwise | 3.4 (N3 F28) | 0.0 | 3.4 (N3 F28) | 187 | 0 | 0 | 0 |

Therefore `EARLY_FAIL_GLOBAL_PASS_COUNT` is zero for every 1k point in
single-hop and two-pairwise.  The correct conclusion is
`NO_SEARCH_GAIN_OBSERVED_IN_1K`; it is not a proof that EARLY and GLOBAL are
universally equivalent.

## Validation

```text
make dynamic_sharing_b                                  PASS
make test_solution_take_policy                          PASS
python3 tests/r3_metadata_contract_test.py \
  --root tmp/r3_metadata_smoke_n2_f8_1g_closure_20260918  PASS
python3 -m py_compile scripts/group/r3_formal_group_repair_rate.py \
  scripts/group/reconstruct_r3_normalized_metadata.py \
  tests/r3_metadata_contract_test.py                     PASS
```

No formal sweep was launched.
