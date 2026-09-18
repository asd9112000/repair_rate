# P2DOM 2x2 OPT1 class-collapse closure

```text
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
OPT_LEVEL: OPT1
IMPLEMENTATION: raw-map safe-class filter + unchanged canonical DFS core
GIT_REVISION: working tree after 630bb23
```

The new explicitly named wrapper retains only the first canonical candidate in
each per-SA `(explicit_release, actual_release, actual_borrow)` class and then
instantiates the unchanged canonical exhaustive core. It retains the raw 480-bit
history inside that core; no summary or dominance state is added.

```text
RAW_HISTORY_BITS: 480
SUMMARY_BITS: 0
DFS_STACK_BITS: 56 (unchanged child)
DOMINANCE_CACHE_BITS: 0
CONTROL_RESULT_BITS: 94 (unchanged child)
TOTAL_LOGICAL_STATE_BITS: 630
THEORETICAL_CANDIDATE_EVALUATION_BOUND: 4680
RTL_CYCLE_BOUND: NOT_REDUCED_BY_THIS_WRAPPER
```

The last line is intentional: this OPT1 implementation filters valid map bits
but retains the child core's 40-position cursor. Candidate evaluation reduction
must not be presented as an equivalent clock-cycle reduction.

## Equivalence gate

```text
COMMAND: make test_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed
DIRECTED_N2_F16_GROUP172: PASS
DIRECTED_EFFECT_ONLY_COUNTEREXAMPLE: PASS
RANDOM_CANDIDATE_MAPS: 1000
RANDOM_SEED: 20260918
REPAIRABILITY_MISMATCHES: 0
SELECTED_ACTION_CONFIG_PATTERN_DONOR_MISMATCHES: 0
RELEASE_BORROW_FINAL_STATE_MISMATCHES: 0
```

The future-donor obligation trace is deterministic from the identical selected
tuple and unchanged raw effects; no new external obligation-trace port was
added. The original canonical regression also remains PASS.

## Tooling gate

The Verilator toolchain test is PASS without warnings. The requested strict
readable-Verilog deliverable gate is BLOCKED before analysis: its installed
runtime uses Python generic syntax unsupported by available Python 3.8.10. No
Python >=3.9 interpreter is installed. This is a local tooling blocker, not an
RTL functional failure.

```text
OPT1_FUNCTIONAL_STATUS: COMPLETE
OPT1_SYNTHESIS_STATUS: NOT_STARTED
PRODUCTION_CANONICAL_GLOBAL_MODIFIED: NO
```
