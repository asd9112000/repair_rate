# Phase 4H — Unified Hardware Characterization

## Scope and comparison boundary

This record compares the same implemented DSS decision/resource-management
boundary at the frozen 2×2 Directional point.  It does not compare final
repair-table reconstruction, runtime remap encoding, Scratch, SRAM, 1×4, or
post-BIST event timing.

| Design | Frozen decision semantics |
|---|---|
| Legacy Phase 3J EARLY | ConfigPatternMap collection followed by EARLY greedy selection |
| Legacy Phase 3J GROUP | ConfigPatternMap collection followed by GROUP greedy selection |
| V2 Specialized EARLY | Stream → first feasible select → commit → discard |
| V2 GROUP-NoScratch | Collect all canonical candidates → retain compressed history → deferred GROUP allocation |

V2 EARLY and V2 GROUP-NoScratch were separately verified at selected
ConfigID/PatternID, donor/release action, ledger evolution, failure position,
and group repairability.  The V2 GROUP regression used seed `20260910` for 50
and 1000 vectors with zero mismatches.

## Master hardware table

All areas are DC **Total cell area**, under TSMC018 `slow.db`, 20 ns, and the
same NAND2X1 normalization (`9.979200`).  Logical state and mapped sequential
cell count are separately reported.

| Metric | Legacy EARLY | Legacy GROUP | V2 EARLY | V2 GROUP-NoScratch |
|---|---:|---:|---:|---:|
| Cell area | 117219.010491 | 117139.176890 | 80871.437539 | 105393.658552 |
| GE | 11746.33 | 11738.33 | 8104.00 | 10561.33 |
| WNS @ 20 ns | +0.01 ns | +0.01 ns | +0.01 ns | +0.00 ns |
| Timing met | Yes | Yes | Yes | Yes |
| Sequential area | 25882.718727 | 25882.718727 | 4101.451260 | 8781.696129 |
| Sequential cells | 485 | 485 | 75 | 157 |
| Architectural state | 485 bits | 485 bits | 73 bits | 157 bits |
| Candidate-history state | about 400 bits | about 400 bits | 0 | 80 bits |
| ConfigPatternMap present | Yes | Yes | No | No |
| Representative DSS execution latency | all-local 30; fail-A 27 | not separately measured | all-local 4; fail-A 4 | all-local 20; fail-A/B/C/D 20/21/22/23; slot2-last 26; release+borrow 22 |

The legacy 485-bit value is supported by the Phase 3J state audit: four
registered 80-bit ConfigPatternMaps (about 400 bits) plus scheduler,
controller, allocator, ledger, and result state.  The reported execution
latencies are from accepted `start_i` to `done_o`; they are DSS decision
latencies, not post-BIST analysis latencies.

## Derived comparisons

### V2 EARLY versus legacy EARLY

```text
area reduction             = 31.0083%
GE reduction               = 31.0082%
state reduction            = 84.5361%  (485 → 75 mapped-state reference)
sequential-area reduction  = 84.1537%
```

### V2 GROUP-NoScratch versus legacy GROUP

```text
area reduction             = 10.0270%
GE reduction               = 10.0270%
state reduction            = 67.6289%  (485 → 157)
candidate-history reduction = 80.0000% (about 400 → 80 bits)
sequential-area reduction  = 66.0712%
```

### V2 GROUP-NoScratch versus V2 EARLY

```text
area overhead              = 24522.221013 (+30.3225%)
GE overhead                = 2457.33 (+30.3224%)
state overhead             = 84 bits
candidate-history overhead = 80 bits
sequential-cell overhead   = 82 cells
execution-latency overhead = +16 cycles for all-local and fail-A representative cases
```

## SolutionTakePolicy cost

EARLY has no retained candidate history: it streams the current analyzer
result, selects the first feasible candidate in the frozen role order, commits
the transaction, and proceeds.  GROUP must first collect all four canonical
slots for every SA, then retain `valid + PatternID` for each candidate:

```text
16 candidates × (1 valid bit + 4 PatternID bits) = 80 bits
```

The measured GROUP overhead therefore represents deferred selection and its
80-bit compressed history, not a retained generalized ConfigPatternMap.

## Phase 4G architecture-space pruning

A separate GROUP-WithScratch implementation was not pursued.  The lifetime
audit proved that frozen GROUP allocation requires all sixteen `{valid,
PatternID}` records after collection.  Per-SA scratch merely redistributes the
same 80 bits.  A recompute design would require a new external input-stability
contract, or a roughly 204-bit raw analyzer-input snapshot, removing the
intended storage benefit.  This is an architecture-space pruning result, not
an implementation failure.

## Critical paths and area composition

| Design | Worst registered path | Arrival / WNS | Dominant composition |
|---|---|---|---|
| V2 EARLY | `c/sa_q_reg[1] → c/l/borrower_id_q_reg[7]`; config decode → analyzer → solution-valid → ledger commit | 19.70 ns / +0.01 ns | Analyzer 70938.8070 (87.7180%); EARLY core 9932.6305 (12.2820%) |
| V2 GROUP-NoScratch | `core/sa_q_reg[1] → core/store/store_q_reg[70]`; slot decode → analyzer → candidate-store write | 19.74 ns / +0.00 ns | Analyzer 73756.2678 (70.0%); store 21129.2930 (20.0%); complete GROUP core 31620.7588 (30.0%) |

Both paths meet the same 20-ns constraint.  The data does not establish a
frequency advantage; it establishes that both specialized implementations meet
the fixed target while differing in latency and storage architecture.

## Paper-level hardware conclusions

1. Specializing the frozen Phase 3J decision boundary removes the generalized
   map state and reduces cell area for both policies.
2. EARLY is the smallest, lowest-state, and lowest DSS-execution-latency
   specialized policy because it does not defer selection.
3. GROUP retains a real hardware cost: its 80-bit history, collection phase,
   and deferred allocation add 30.3225% cell area over V2 EARLY.
4. The 80-bit representation is sufficient for frozen GROUP semantics and
   replaces about 400 bits of legacy map-related state.
5. Scratch provides no credible reduction below that 80-bit minimum at the
   current decision/resource-management boundary.

## Phase 4I event-model inputs

The following parameters are frozen for a later post-BIST event-timing study;
no sweep is performed in this phase.

| Item | EARLY | GROUP-NoScratch |
|---|---|---|
| Candidate-evaluation order | A/D: CFG0→CFG4→CFG5→CFG6; B/C: CFG0→CFG2→CFG1→CFG3 | canonical slots: 1→0→3→2 |
| Collection behavior | none; consume current candidate | 16 cycles: four slots for each A/B/C/D SA |
| Allocation behavior | first feasible then immediate commit | A→B→C→D after collection; latest ledger for next SA |
| Decision-ready event | registered `done_o` after final commit or first failure | registered `done_o` after final commit or first failure |
| Representative execution model | 4 cycles, all-local/fail-A | 20–26 cycles, candidate/rank dependent |

## Phase control

```text
PHASE_4H_UNIFIED_CHARACTERIZATION = PASS
PHASE_4H = COMPLETE
READY_FOR_PHASE_4I = YES

V2_PHASE = PHASE_4H
V2_NEXT_PHASE_AUTHORIZED = YES
```
