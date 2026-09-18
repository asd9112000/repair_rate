# P3-BL-RTL-A — latency and commit audit

```text
P3BLRTLA_STATUS: COMPLETE
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
CLOSURE_COMMIT: ffe2a97
```

The producer has sixteen sequential analyzer capture cycles. SEARCH starts only after its complete raw map is retained. Candidate visits remain a search-work metric and are not relabeled as cycles.

| Segment | Cycles / rule |
|---|---|
| Accepted start to producer request | registered top-control handoff |
| Candidate generation | 16 fixed capture cycles |
| Map-ready to search start | registered handoff |
| GLOBAL search | data-dependent bounded DFS cycles |
| Search done to commit start | registered handoff |
| Commit processing after accepted adapter start | 5 fixed cycles: stage A/B/C/D then publish |
| Commit acceptance to done | one registered top observation cycle |

The integrated directed success test reports the measured total below; the all-invalid-C negative case is intentionally search-length dependent for OPT0 and remains a functional, not latency, comparison.

```text
CANDIDATE_PRODUCTION_REQUESTS: 16
CANDIDATE_GENERATION_CYCLES: 16
GLOBAL_SEARCH_CYCLES: DATA_DEPENDENT
COMMIT_CYCLES: 5
INTEGRATED_DIRECTED_SUCCESS_CYCLES: 34
INTEGRATED_DIRECTED_FAILURE_CYCLES: 13192 (C snapshot overflow; OPT0 exhaustive failure path)
DIRECTED_SUCCESS_TOTAL_END_TO_END_CYCLES: 34
DIRECTED_SUCCESS_IS_FIXED_GLOBAL_LATENCY: NO
```

`34` is a directed all-zero-snapshot witness under the testbench counting
convention below, **not a fixed GROUP_GLOBAL latency**. Candidate visits and
search cycles are distinct: a DFS may spend multiple cycles advancing,
backtracking, or testing candidates, so a visit count must not be relabelled as
a cycle count.

## Implemented commit timing

This audit follows production `atomic_group_commit.v`, not the design plan.
The GLOBAL core asserts `search_done` and registers the selected tuple on the
rising edge that takes its terminal legal depth-3 candidate from
`STATE_SEARCH` to `STATE_IDLE`. The top shell observes that registered pulse on
the next edge in `STATE_SEARCH_WAIT`, then emits its registered
`commit_start_q` pulse on the following edge in `STATE_COMMIT_START`.

When the commit adapter observes that pulse while idle, it copies the persistent
ledger into private shadow registers, clears per-commit requirements/count, sets
`stage_q = A`, and enters `STATE_STAGE`. That is an initialization edge; it is
not one of the reported five processing cycles. The implemented sequence is:

| Counted commit cycle | Commit FSM before edge | Operation at edge | Persistent ledger change | `commit_accepted_o` after edge |
|---:|---|---|---|---|
| 1 | `STATE_STAGE`, A | Validate/update A in shadow | No | 0 |
| 2 | `STATE_STAGE`, B | Validate/update B in shadow | No | 0 |
| 3 | `STATE_STAGE`, C | Validate/update C in shadow | No | 0 |
| 4 | `STATE_STAGE`, D | Validate/update D in shadow; enter publish | No | 0 |
| 5 | `STATE_PUBLISH` | Copy shadow released/borrowed/borrower-ID state to persistent ledger | **Yes** | **1** |

Thus `COMMIT_CYCLES = 5` means A + B + C + D + publish. The publish/update
edge is included. `group_commit_accepted` (`commit_accepted_o`) asserts on the
same publish edge as the only persistent-ledger update. The top shell observes
that registered acceptance on one subsequent rising edge in
`STATE_COMMIT_WAIT`, asserts `done_o`, and returns to idle. `done_o` is therefore
one top-control observation edge later than commit acceptance.

### Concrete directed-success trace

Relative cycle 0 is the rising edge where the search core registers its legal
four-SA tuple and asserts `search_done`; this is the all-zero-snapshot directed
case in `tb_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_equivalence.cpp`.

| Relative cycle | Top FSM after edge | Commit FSM after edge | Shadow operation | Persistent ledger changes? | `commit_accepted` | `done_o` |
|---:|---|---|---|---|---:|---:|
| 0 | `STATE_SEARCH_WAIT` | `STATE_IDLE` | Search core registers tuple/done; top cannot consume same-edge registered output | No | 0 | 0 |
| 1 | `STATE_COMMIT_START` | `STATE_IDLE` | Top consumes repairable search result | No | 0 | 0 |
| 2 | `STATE_COMMIT_WAIT` | `STATE_IDLE` | Top registers `commit_start_q`; adapter sees its prior value | No | 0 | 0 |
| 3 | `STATE_COMMIT_WAIT` | `STATE_STAGE` (A) | Adapter accepts start and copies persistent ledger to shadow | No | 0 | 0 |
| 4 | `STATE_COMMIT_WAIT` | `STATE_STAGE` (B) | A shadow validation/update | No | 0 | 0 |
| 5 | `STATE_COMMIT_WAIT` | `STATE_STAGE` (C) | B shadow validation/update | No | 0 | 0 |
| 6 | `STATE_COMMIT_WAIT` | `STATE_STAGE` (D) | C shadow validation/update | No | 0 | 0 |
| 7 | `STATE_COMMIT_WAIT` | `STATE_PUBLISH` | D shadow validation/update | No | 0 | 0 |
| 8 | `STATE_COMMIT_WAIT` | `STATE_IDLE` | Publish shadow to persistent ledger | **Yes** | **1** | 0 |
| 9 | `STATE_IDLE` | `STATE_IDLE` | Top observes acceptance and publishes result tuple | No further change | 0 | **1** |

For a legal four-SA tuple the five processing cycles are fixed and independent
of which legal tuple was selected. A search failure does not enter the commit
FSM: the top sets failed `done_o` directly from `STATE_SEARCH_WAIT`. A forced
commit error does not have a five-cycle guarantee: non-empty persistent state
rejects at adapter start, while a stage legality failure terminates on the
failing stage; neither publishes persistent state.

## End-to-end formula and the 34-cycle witness

The integrated test calls the accepted-start edge before its loop and returns
the number of **subsequent** `tick()` calls through the `done_o` observation.
For a successful request:

```text
T_after_start = 1 top-producer-start
              + 1 producer-start acceptance
              + 16 producer captures
              + 1 producer-done observation
              + 1 top-search-start
              + 1 search-start acceptance
              + T_DFS
              + 1 search-done observation
              + 1 top-commit-start
              + 1 commit-start/shadow initialization
              + 5 commit processing (A/B/C/D/publish)
              + 1 commit-accepted observation / done

              = 30 + T_DFS
```

`T_DFS` is data-dependent and is the number of post-search-start DFS progression
edges required by the selected search core. In the directed all-zero case the
core accepts A/B/C/D in four DFS progression edges, hence:

```text
34 = 1 + 1 + 16 + 1 + 1 + 1 + 4 + 1 + 1 + 1 + 5 + 1
```

The initial accepted `start_i` edge is intentionally excluded from `34` by that
testbench convention. Including that acceptance edge would give 35 rising
edges from assertion/acceptance through `done_o`.

## Future search measurements

No measurement RTL was added in this audit. Future runs must report separate
distributions for both `search cycles` and `candidate visits`: mean, median,
P95, P99, and maximum observed. The current directed test supplies a
cycle-to-done witness, not these distributions.
