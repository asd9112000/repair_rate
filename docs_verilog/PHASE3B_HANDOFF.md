# Phase 3B Shared RECAM Analyzer Handoff

> Date: 2026-09-10  
> Historical handoff; its prior blockers are superseded by the frozen 27-bit
> geometry in [PHASE3B_ANALYZER_INTERFACE.md](PHASE3B_ANALYZER_INTERFACE.md).  
> Historical handoff status only.  Phase 3B is frozen and Phase 3C is now
> authorized/completed functionally; see [PHASE3C_ARCHITECTURE.md](PHASE3C_ARCHITECTURE.md)
> and [PHASE_STATUS.md](PHASE_STATUS.md) for current status.

## 1. Objective and approved boundary

Phase 3B is one fully combinational, pattern-parallel Shared Multi-Config
Analyzer:

```text
canonical CAM logical state + ConfigID
    → shared Matrix/candidate analysis
    → PatternID
```

Supported physical ConfigIDs are:

```text
CFG0  2R2C       CFG4  1R2C (transpose of CAN1)
CFG1  2R1C       CFG5  2R3C (transpose of CAN2)
CFG2  3R2C       CFG6  1R3C (transpose of CAN3)
CFG3  3R1C
```

Canonical classes are `2R2C`, `2R1C`, `3R2C`, and `3R1C`. Candidate patterns
within the selected ConfigID remain parallel. The analyzer contains no clock,
reset, pipeline, FSM, scheduler, EARLY, GROUP, ConfigPatternMap,
PhysicalResourceLedger, or PendingRepairBuffer.

The frozen boundary starts after collection/storage. It accepts pivot logical
state, Must threshold metadata, and compressed Hybrid logical entries; it does
not reclassify raw faults. Collector generation and CAM storage remain excluded
from the analyzer-only area boundary, but must be charged in a later complete
hardware comparison. See [PHASE3B_ANALYZER_INTERFACE.md](PHASE3B_ANALYZER_INTERFACE.md).

## 2. Current implementation and artifacts

| Artifact | Purpose |
|---|---|
| `rtl/recam/recam_shared_config_analyzer.sv` | Current shared combinational RTL. `dictionary_overflow_o` makes the dictionary-full negative behavior observable. |
| `tb/recam/recam_phase3b_verification_top.sv` | Verification wrapper that reconstructs Phase 3A full Hybrid tuples for CFG0 comparison. |
| `tb/recam/recam_shared_config_analyzer_test.cpp` | Independent logical golden, directed/random CFG0 comparison, ConfigID, threshold, transpose, MAX_K=5, and negative tests. |
| `scripts/simulation/run_recam_phase3b_functional.sh` | Reproducible Phase 3B lint and regression entry point. |
| `docs_verilog/PHASE3B_FUNCTIONAL_VERIFICATION.md` | Detailed command results and blocker evidence. |
| `docs_verilog/PHASE3B_ANALYZER_INTERFACE.md` | Frozen boundary, ownership, and descriptor convention. |

## 3. Completed work

- Candidate enumeration no longer has Verilator WIDTH warnings. It uses fixed
  canonical pattern tables, rather than integer mask/bit/candidate conversions.
- Hybrid descriptor mapping was corrected from executable upstream evidence:

  ```text
  descriptor = 0: same pivot row; differing address is a column
  descriptor = 1: same pivot column; differing address is a row
  transpose: invert the differing dimension
  ```

  Phase 3A carries full Hybrid row/column tuples and has no descriptor bit.
- The compact-prefix pivot contract was verified from `shared_pivot_cam.sv`:
  writes target `valid_mem[occupancy_o]`, followed by occupancy increment. The
  testbench accepts contiguous low-index entries and detects sparse validity
  such as slots `{0,2}` as an invalid input contract.
- Lint passes with no warnings.
- Independent functional checks pass for CFG0–CFG6, all physical Must
  thresholds, MAX_K=5/pointer 4, transpose pairs, invalid ConfigID, conventional
  overflow, dictionary-full behavior, no-valid-candidate, and sparse-prefix
  negative input handling.

## 4. Verification results and exact blocker

Run:

```bash
bash scripts/simulation/run_recam_phase3b_functional.sh
```

Lint exits 0. The regression deliberately exits 1 because the mandatory CFG0
Phase 3A equivalence gate fails:

```text
vectors tested: 2,008
mismatches:     810
first mismatch: one valid pivot at M[0][0], no Must, no Hybrid, no overflow

Phase 3A candidate_valid: 000111
Phase 3B candidate_valid: 111111
PatternID in both:        1
```

For a diagonal fault `M[0][0]`, each 2R2C candidate repairs the fault: the
first three select Row0 and the other three select Col0. Therefore the C++
RECAM solution semantics and Phase 2 analyzer require all six candidates to be
valid.

The discrepancy is in frozen Phase 3A
`recam_2r2c_pattern_analyzer.sv`:

```systemverilog
!(!pattern[row] || !pattern[col])
```

This simplifies to `pattern[row] && pattern[col]`. The required uncovered
condition is instead:

```text
pattern[row] && !pattern[col]
```

Evidence for the latter is the C++ solution matrix construction in
`src/SolGenerator.cpp`, plus the Phase 2 `config_analyzer.sv` and
`config_candidate_evaluator.sv`.

The existing Phase 3A smoke test passes only because it does not assert this
one-pivot candidate-valid bitmap. `make test_recam_paper_faithful` passes and
confirms the C++ solution-space convention.

## 5. Second unresolved semantic policy

The latest Phase 3B negative-test requirement says that a full logical
row/column dictionary plus an unseen differing address must assert
`dictionary_overflow_o` and invalidate every candidate. The shared analyzer
implements and tests that behavior, including no fallback write to index 0.

Phase 3A matrix builder does something different for the corresponding full
tuple: it marks the known counterpart as a complete matrix row/column. Thus,
even after resolving the candidate predicate, Phase 3A and Phase 3B cannot be
claimed bit-exact equivalent for this case until the project owner chooses one
policy.

## 6. Resolved Hybrid capacity

The prior four-entry limitation is obsolete. The shared interface is frozen at
`MAX_HYBRID_ENTRIES=7`, covering the maximum 3R2C/2R3C capacity:

```text
Rs × (Cs - 1) + Cs × (Rs - 1) = 3 × 1 + 2 × 2 = 7
```

The regression drives `hybrid_valid[6]` in CFG2 and verifies that it changes
the candidate bitmap. The frozen address boundary is asymmetric:
`ROW_ADDR_W=9`, `COL_ADDR_W=5`, and `DIFF_ADDR_W=9` with zero-extended
ColumnWord payloads.

## 7. Next authorized action

All former functional blockers are resolved. Stop at Phase 3B and wait for an
explicit research authorization before regenerating corrected Phase 3A or
Phase 3B synthesis reports. Any later retained synthesis report must include
cell area, leaf-cell count, combinational/sequential cells,
NAND2X1-equivalent gate count, critical path, WNS, and Fmax estimate.

## 8. Explicitly forbidden until a new authorization

```text
Phase 3C
Config scheduler
EARLY / GROUP
PhysicalResourceLedger
ConfigPatternMap registers
PendingRepairBuffer
Pattern serialization
Pipeline / sequential state
Phase 3B synthesis
```

## 9. Status at handoff

```text
CURRENT_PHASE: 3B
STATUS: FUNCTIONAL_VERIFICATION_COMPLETE_WITH_FROZEN_27_BIT_ADDRESS_GEOMETRY
PHASE_3B_FUNCTIONAL_VERIFICATION_COMPLETE: YES
SYNTHESIS: NOT RUN
PHASE_3C: NOT STARTED
```
