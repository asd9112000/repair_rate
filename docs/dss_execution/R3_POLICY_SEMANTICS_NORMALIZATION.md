# R3 sharing-policy semantics normalization

> Status: Current. Scope: legacy **group-level** `DynamicSpareSharing` only.
> This document neither changes RTL nor reinterprets Hierarchical-RECAM data.

## Result

The canonical, same-contract ladder is now `LOCAL_FIRST`, `EARLY`, and
`GLOBAL` wherever a frozen candidate contract exists.  A policy ID is an
experiment-facing name; `solution_policy` remains the implementation-facing
sidecar value.  Historical implementations and IDs remain callable.

| Canonical topology | LOCAL_FIRST | EARLY | GLOBAL | contract |
|---|---|---|---|---|
| 2x2 directional m=1 (N2/N3) | `directional_m1_local_first` | `directional_m1_early` | `directional_m1_global` | `DIRECTIONAL_V2` |
| 2x2 directional m=2 | blocked | blocked | blocked | no frozen m=2 role/config contract |
| 2x2 edge row m=1 | `pairwise_row_m1_local_first` | `pairwise_row_m1_early` | `pairwise_row_m1_global` | `GENERIC_RECAM` |
| 1x4 single-hop row m=1 | `single_hop_m1_local_first` | `single_hop_m1_early` | `single_hop_m1_global` | `R1B_1X4` |
| 1x4 two-pairwise row m=1 | `two_pairwise_m1_local_first` | `two_pairwise_m1_early` | `two_pairwise_m1_pair_global` | `R1B_1X4` |

`local_no_sharing` is a separate baseline, not a member of the ladder.

## Code-path audit

All rows below use `DynamicRepairSimulator::run()`. Candidate validity comes
from the RECAM solver's retained `validSolList`; no selector re-solves RECAM.
`compressedPlansForSubarray()` preserves attempt order followed by ascending
PatternID. `PhysicalResourceLedger::allocateSequential()` applies every
sequential policy's A->B->C->D committed prefix; it owns every physical line,
performs legal borrowing, and has no rollback.

| Existing implementation / policy | layout, topology, sharing | generator and capacity/config options | priority / IDs / processing order | ledger, release, borrow, commit | class and scope |
|---|---|---|---|---|---|
| `directional_v2_early` | 2x2 directional, m=1 row+col | `v2CapacityOptions`: LOCAL, RELEASE_ONLY, BORROW_ONLY, RELEASE_AND_BORROW; frozen role-slot ConfigID table; ascending valid PatternID | slots `0,1,2,3`; A->B->C->D. ConfigID is role-slot encoding; PatternID is one-based lowest valid RECAM pattern | same directional ledger; release/borrow represented by slot; immediate prefix commit; no rollback | `LOCAL_FIRST`; canonical alias `directional_m1_local_first` |
| `group_greedy_rtl_canonical` | 2x2 directional, m=1 row+col | same frozen V2 options and ConfigID/PatternID contract | slots `1,0,3,2`; A->B->C->D | same ledger and actions; immediate prefix commit; no future-SA lookahead or rollback | `EARLY`; canonical alias `directional_m1_early` |
| `directional_v2_group_global` | 2x2 directional, m=1 row+col | same V2 slots; retains every slot/PatternID candidate | stable slot `0..3`, then PatternID; joint A->B->C->D DFS | same ledger on each prefix; reversible speculative allocation; first legal complete tuple commits | `GLOBAL`; canonical alias `directional_m1_global` |
| `early` | 2x2 directional m=2 and edge m=1; also legacy generic use | `capacityOptions`, all legal extra row/column combinations; no ConfigID; PatternID is generic RECAM candidate index | ranks current legal choices by borrowed lines, used lines, PatternID, attempt; A->B->C->D | prefix ledger; may release/borrow when capacity allows; immediate commit; no rollback/lookahead | `EARLY` for edge m=1; `GENERIC_LEGACY` for directional m=2 |
| `group_global` | generic 2x2 directional/edge | generic compressed candidates, demand-deduplicated only where safe; no ConfigID | full A->D tuple ranking: borrowed, used lines, PatternID tuple, attempt tuple | prefix ledger DFS with speculative backtracking; final atomic result | `GLOBAL` for edge m=1; `GENERIC_LEGACY` for directional m=2 and historical directional m=1 |
| `local_first` | generic 2x2, including edge m=1 | same `capacityOptions`/generic RECAM candidates as `early` | natural local-capacity then ascending PatternID first legal; A->B->C->D | prefix ledger; immediate commit; no rollback/lookahead | `LOCAL_FIRST`; canonical alias `pairwise_row_m1_local_first` |
| `one_by_four_single_hop_early_v1` | 1x4 neighbor, row m=1 | R1B `capacityOptions`: local then increasing extra rows; generic PatternID | first ledger-legal candidate, A->B->C->D; middle donors resolve left before right | neighbor-only ledger; immediate commit; no rollback/lookahead | `LOCAL_FIRST`; canonical alias `single_hop_m1_local_first` |
| `one_by_four_single_hop_release_aware_early_v1` | 1x4 neighbor, row m=1 | same R1B candidates and PatternIDs | ranks legal prefix choices by borrowed rows, used rows, PatternID, attempt; A->B->C->D | same neighbor ledger; immediate commit; no rollback/lookahead | `EARLY`; canonical alias `single_hop_m1_early` |
| `one_by_four_single_hop_global_v1` | 1x4 neighbor, row m=1 | same R1B candidates / PatternIDs | complete A->D tuple search | same neighbor ledger with speculative prefix search and backtracking | `GLOBAL`; canonical alias `single_hop_m1_global` |
| `one_by_four_two_pairwise_early_v1` | 1x4 pair (AB, CD), row m=1 | same R1B candidates / PatternIDs | first legal, A->B->C->D | pair-only ledger; immediate commit; no rollback | `LOCAL_FIRST`; canonical alias `two_pairwise_m1_local_first` |
| `one_by_four_two_pairwise_release_aware_early_v1` | 1x4 pair, row m=1 | same R1B candidates / PatternIDs | borrowed-row/used-row/PatternID/attempt rank, A->B->C->D | same pair-only ledger; immediate commit; no rollback/lookahead | `EARLY`; canonical alias `two_pairwise_m1_early` |
| `one_by_four_two_pairwise_pair_global_v1` | 1x4 pair, row m=1 | same R1B candidates / PatternIDs | exhaustive AB then CD pair tuple ranking | same pair ledger; speculative pair search and final whole-ledger check | `GLOBAL` with pair (not four-SA) scope; canonical alias `two_pairwise_m1_pair_global` |

Historical `directional_m1_group_global`, `group_compressed_legacy`, and
`group_no_scratch_v2` are retained as `GENERIC_LEGACY` / `OTHER` records.
They must not be relabelled as a canonical directional m=1 ladder member.

## Directional m=2 containment block

`v2CapacityOptions()` is the only frozen four-class table.  For directional
m=1, A/D have `{(R,C),(R-1,C),(R,C+1),(R-1,C+1)}` and B/C have
`{(R,C),(R,C-1),(R+1,C),(R+1,C-1)}`, corresponding respectively to LOCAL,
RELEASE_ONLY, BORROW_ONLY, RELEASE_AND_BORROW.  Generic directional m=2 uses
`capacityOptions()`'s Cartesian extra-row/extra-column range instead; it has
neither role-specific ConfigIDs nor a frozen release/borrow class mapping for
all N.

```text
M2_POLICY_NORMALIZATION: BLOCKED
```

Consequently canonical R3 membership excludes directional m=2.  Its `early`
and `group_global` implementations remain available for historical analysis
only, and their results may not be presented as a same-contract ladder.

## Required containment checks

For every canonical `EARLY` / `GLOBAL` pair, the R3 runner compares raw,
same-corpus sidecars and fails on `EARLY_PASS_GLOBAL_FAIL`.  It also persists
both `LOCAL_FIRST_PASS_EARLY_FAIL` and `LOCAL_FIRST_FAIL_EARLY_PASS` in paired
outcomes rather than assuming priority containment.  Two-pairwise GLOBAL is
pair-scoped by physical topology; its containment check is against the same
R1B candidate universe within each independent pair.
