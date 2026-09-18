# R1A — 2×2 ConfigID and Paired-Corpus Contract Closure

> Status: COMPLETE for 2×2 R1A. This closes numeric ConfigID interpretation by
> architecture-point contract; it does not assert one global ConfigID table.
> No RTL was changed, no synthesis ran, and no formal repair-rate data was
> collected.

## Authoritative ConfigID registry

| Contract version | RS/CS/m | SA role | slot 0 / 1 / 2 / 3 ConfigID | semantic slots | source of truth |
|---|---|---|---|---|---|
| `frozen_date_2x2_m1` | 2/2/1 | A/D | 0 / 4 / 5 / 6 | local / release / borrow / release+borrow | `rtl/dss_v2/group/dss_v2_group_slot_decode.sv` plus `dss_legacy_config_adapter.sv` |
| `frozen_date_2x2_m1` | 2/2/1 | B/C | 0 / 1 / 2 / 3 | local / release / borrow / release+borrow | same |
| `retained_overlap_2x2_v1` | 2/2/1 | A/D | 0 / 4 / 5 / 6 | local / release / borrow / release+borrow | `recam_dss_v2_{early,retained}_overlap_core.sv` |
| `retained_overlap_2x2_v1` | 2/2/1 | B/C | 0 / 2 / 1 / 3 | local / borrow / release / release+borrow | same; EARLY traversal-specific ordering |
| `rs3cs3_m1` | 3/3/1 | A/D | 0 / 1 / 2 / 3 | local / release / borrow / release+borrow | `rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_config_table.sv` |
| `rs3cs3_m1` | 3/3/1 | B/C | 0 / 4 / 5 / 6 | local / release / borrow / release+borrow | same |
| `historical_cpp_v2_slot_map_v1` | historical C++ | A/D | 0 / 1 / 2 / 3 | historical C++ only | `v2RoleSlotMappings` |
| `historical_cpp_v2_slot_map_v1` | historical C++ | B/C | 0 / 4 / 5 / 6 | historical C++ only | same |

The apparent R1 conflict therefore was revision/envelope dependent. The
frozen 2×2 decoder is a DATE compatibility point; RS3 is an isolated target
namespace with a target-specific table. Numeric IDs must never cross those
contract boundaries.

## C++ contract-aware behavior

`group_greedy_rtl_canonical` now resolves a `ConfigContractVersion` before
selecting ConfigIDs:

- 2×2 directional m=1 → `frozen_date_2x2_m1`;
- 3×3 directional m=1 → `rs3cs3_m1`;
- other sharing envelopes → rejected rather than silently aliased.

The internal no-sharing baseline needed for paired reporting is accepted under
the resolved point, but does not change a sharing-policy contract. Historical
`group_no_scratch_v2` remains explicitly tagged
`historical_cpp_v2_slot_map_v1`.

For both canonical points, GROUP rank is `1,0,3,2`; this is role-slot rank,
not numeric ConfigID sort. The canonical tests cover both tables: 2×2 picks
ConfigIDs `4,1,1,4` in an all-feasible rank-0 release case; RS3 picks
`1,4,4,1`.

## Comparison contract

| Comparison | Required to match | Allowed to differ |
|---|---|---|
| RTL vs matching C++ Config contract | candidate validity, semantic role, PatternID, R/C demand, release/borrow meaning, ledger transition, repairability | numeric ConfigID only across different contract versions |
| historical V2 vs canonical greedy | same input corpus only | selected ConfigID/PatternID, ledger state, repair result when priority changes |
| canonical greedy vs GROUP-GLOBAL | same input corpus and ledger model | selected tuple, ConfigID/PatternID, ledger state, repair result |
| GROUP-GLOBAL vs oracle | repairability, winning attempt/Pattern tuple, borrowed counts, used counts, final ledger owners | none |

Thus the prior 128-vector result compared historical C++ V2 priority with
canonical greedy, not greedy with global. Its 0 success mismatches and 128
ConfigID/PatternID/ledger mismatches are expected priority characterization,
not a reference mismatch.

## RS3 lockstep

The isolated RS3 target lockstep test passed with 16 directed cases and 1,000
random candidate maps for both EARLY and GROUP. It reported zero semantic
mismatches for repairability, failure position, commits, ConfigID, PatternID,
borrow/release action, donor, and ledger state. This test uses the explicitly
versioned RS3 table; it is not evidence for frozen 2×2 numeric IDs.

## GROUP-GLOBAL oracle and counterexample

`tests/solution_take_policy_test.cpp` has an independent oracle over all valid
attempt/PatternID entries. It calls the ledger on every complete tuple and
reimplements the frozen objective/tie-break. Directed same-config PatternIDs
with different R/C demand are independently enumerated. Result: one directed
fixture plus 128 randomized legal candidate states, zero production/oracle
mismatches across repairability, winning tuple, borrowed/used lines, and final
ledger owners.

`GREEDY_FAIL_GLOBAL_PASS` remains a **CONFIRMED DIRECTED COUNTEREXAMPLE**. It
is a candidate-state regression, not a statistical repair-rate claim.

## Paired-corpus sidecar v1

`DynamicSpareSharing` now emits two additive sidecars:

| File | Identity and contents |
|---|---|
| `paired_corpus_v1.csv` | `dss_paired_corpus_v1`; policy-independent FNV-1a corpus hash/ID, generator version, seed, geometry, share values, fault model, group ID, and complete SA-local seven-field fault lists. |
| `paired_policy_results_v1.csv` | corpus/group ID plus policy/sharing/solution IDs, ConfigID contract version, simulator revision, selected IDs, repair result, borrow counts, and remaining resources. |

A one-group sidecar smoke check, using seed `20260916`, yielded the same
`dss_paired_corpus_v1-dd9823020bde0b29` for EARLY,
`group_greedy_rtl_canonical`, and `group_global`; the serialized SA-local
fault list is independent of policy. `FAULT_CORPUS_CHANGED_BY_POLICY=NO`.

## R1B roadmap only — no implementation

Primary future 1×4 policy: **Two-Pairwise Row Sharing** (`A⇄B`, `C⇄D`), with
`share_row=m`, `share_col=0`, `N-m` private rows plus `m` pair-shareable rows
per SA, and no B↔C, forwarding, or transitive resource access. Planned modes:
`TWO_PAIRWISE_EARLY` and `TWO_PAIRWISE_PAIR_GLOBAL`, where global search is
per pair, not four-SA exhaustive search.

Comparator future policy: **Single-Hop Nearest Neighbor** (`A—B—C—D`), with
direct-neighbor-only rows and planned `SINGLE_HOP_EARLY` / `SINGLE_HOP_GLOBAL`.
No 1×4 policy implementation is claimed by R1A.
