# Phase 3B Shared Analyzer Functional Verification

> Date: 2026-09-10  
> Status: **PASS — functional verification complete**  
> Scope: functional verification only; no synthesis and no Phase 3C work.

## Frozen geometry

This regression uses the frozen 27-bit system address: Domain/Bank/Group/SA
are `3/2/6/2` bits (`CHANNEL_W=13`), followed by `ROW_ADDR_W=9` and
`COL_ADDR_W=5`. Hybrid differing addresses use `DIFF_ADDR_W=9`, with a
ColumnWord zero-extended from five bits; `WORD_W=16`. The shared analyzer uses
`MAX_K=5`, `PIVOT_PTR_W=3`, five Address CAM entries, and seven Hybrid CAM
entries. Raw reusable CAM storage is 140 Address CAM bits plus 140 Hybrid CAM
bits, or 280 bits; this is not a silicon-area claim.

## Reproduction

```bash
bash scripts/simulation/run_recam_phase3b_functional.sh
```

The runner performs `verilator --lint-only --Wall -Wno-fatal`, builds the
verification wrapper, and executes the C++ regression.

The independent C++ RECAM solution-space regression was also run:

```bash
make test_recam_paper_faithful
```

It exits 0 with `RECAM paper-faithful solution tests passed`.

## Results

| Check | Result | Evidence |
|---|---|---|
| Verilator WIDTH cleanup | PASS | Lint exits 0 with no warnings; no suppression was added. |
| Pivot compact-prefix contract | PASS | Positive prefix and sparse `0b00101` negative check pass. |
| Phase 3A CFG0 equivalence | PASS | 2,008 vectors, 0 mismatches. |
| Phase 3A vs independent golden | PASS | 2,008 CFG0 vectors. |
| Phase 3B CFG0 vs independent golden | PASS | 2,008 CFG0 vectors. |
| CFG0–CFG6 directed cases | PASS | Each configuration covers repairable, unrepairable, Must, and non-Must. |
| Threshold decode | PASS | Physical CS=1/2/3 and RS=1/2/3 threshold inputs each alter candidate validity. |
| MAX_K=5 / pivot 4 / pointer 4 | PASS | Pointer 4 changes the candidate-valid bitmap relative to pointer 0. |
| Seven-Hybrid interface / entry[6] | PASS | A CFG2 Hybrid entry at index 6 changes the candidate bitmap. |
| Asymmetric address widths | PASS | 9-bit row MSB, 5-bit ColumnWord MSB, and 5→9 zero-extension checks affect matching. |
| 2R1C ↔ 1R2C | PASS | Transposed logical states have equal candidate/result semantics. |
| 3R2C ↔ 2R3C | PASS | Transposed logical states have equal candidate/result semantics. |
| 3R1C ↔ 1R3C | PASS | Transposed logical states have equal candidate/result semantics. |
| CFG7 / conventional overflow / no candidate | PASS | Result outputs are zero. |
| Row/column dictionary-full unseen address | PASS | RECAM line expansion constrains candidates without global overflow. |
| Sequential-state source audit | PASS | Shared analyzer contains no clock, edge-sensitive block, `always_ff`, FSM, or retained state. |

## Corrected semantics

The C++ solution definition says `false = spare row` and `true = spare
column`; a matrix location is covered by `!pattern[row] || pattern[col]`.
Therefore a fault invalidates a candidate only when:

```text
pattern[row] && !pattern[col]
```

Phase 3B and the existing Phase 2 `config_analyzer.sv` use that predicate.
Phase 3A now uses the same predicate; it previously used:

```systemverilog
!(!pattern[row] || !pattern[col])
```

which simplifies to `pattern[row] && pattern[col]`. A diagonal pivot therefore
incorrectly rejects every candidate whose corresponding bit selects a column.
The existing Phase 3A smoke test still passes because it does not assert the
full candidate bitmap for the one-pivot case.

The one-pivot diagonal directed regression now requires
`candidate_valid = 6'b111111`, `repairable = 1`, and `PatternID = 1`.
The pre-correction Phase 3A synthesis reports are no longer a baseline for the
corrected RTL. Regenerating that baseline requires later synthesis
authorization; synthesis is prohibited in this functional-only task.

When a logical row/column dictionary is full and a Hybrid differing address is
unseen, Phase 3B now matches Phase 3A RECAM matrix conversion: a known row
expands across valid logical columns, while a known column expands across valid
logical rows. `dictionary_overflow_o` remains reserved for invalid Hybrid
pointers or malformed logical state.

## Descriptor and prefix evidence

Phase 3A stores complete Hybrid row/column tuples and has no descriptor bit.
The executable upstream convention is established by
`shared_fault_collector.sv` and `RECAM_CAM.cpp`:

```text
descriptor 0: same pivot row, differing address is column
descriptor 1: same pivot column, differing address is row
transpose: invert descriptor interpretation
```

`shared_pivot_cam.sv` writes `valid_mem[occupancy_o]` and increments occupancy,
so valid entries are a compact low-index prefix. The testbench detects a sparse
valid vector as a contract violation without adding checker gates to the DUT.

## Current phase decision

Phase 3B is `FUNCTIONAL_VERIFICATION_COMPLETE`. No synthesis was run, no
area/gate-count result was generated, and Phase 3C was not started.
