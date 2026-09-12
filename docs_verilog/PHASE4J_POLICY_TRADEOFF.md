# Phase 4J — SolutionTakePolicy Repairability Tradeoff

## Scope and paired boundary

This experiment measures the repairability tradeoff between the verified V2
Specialized EARLY and V2 GROUP-NoScratch decision/resource-management policies.
It uses the frozen 2×2 Directional envelope (`RS=CS=2`, sharing degree
`m=1`), A→B→C→D traversal, directional donor priority, atomic commit,
first-failure termination, and no rollback.  Production RTL, policy semantics,
Scratch, SRAM, 1×4, and synthesis were not changed.

The candidate-generation input is the existing DATE `DynamicFaultGenerator`
with the existing `DirectionalMultiConfigAnalyzer`.  The policy adapter is the
same frozen V2 ranking/ledger evaluation used by Phase 4I:

- EARLY: A/D `CFG0→CFG4→CFG5→CFG6`; B/C `CFG0→CFG2→CFG1→CFG3`, selecting and
  committing the first feasible candidate.
- GROUP-NoScratch: A/D `CFG4→CFG0→CFG6→CFG5`; B/C
  `CFG1→CFG0→CFG3→CFG2`, with the same greedy traversal and ledger semantics.

The historical `DirectionalMultiConfigAnalyzer::analyzeGroup(...Group)`
global-search path was deliberately not used: it does not implement the frozen
V2 GROUP greedy allocator.

Every vector ID is generated once, analyzed once, then evaluated by both
policies.  The only variable between paired results is `SolutionTakePolicy`.

```text
SEED = 20260910
CASES_PER_POINT = 10000
MAIN_SEVERITY_SWEEP = 8,12,16,20,24,28,32 faults per 2×2 group
FAULT_MODEL / SPATIAL_MODEL = moderate_imbalance / mixed
```

At 20 faults, existing `uniform` and `strong_imbalance` count models provide
the imbalance sensitivity points.  This yields nine points and 90,000 paired
repair-group instances overall.

## Outcome accounting

Each pair is classified exactly once:

```text
BOTH_PASS  = EARLY pass and GROUP pass
EARLY_ONLY = EARLY pass and GROUP fail
GROUP_ONLY = EARLY fail and GROUP pass
BOTH_FAIL  = EARLY fail and GROUP fail
```

For every point:

```text
BOTH_PASS + EARLY_ONLY + GROUP_ONLY + BOTH_FAIL = TOTAL_CASES
EARLY_REPAIR_RATE = (BOTH_PASS + EARLY_ONLY) / TOTAL_CASES
GROUP_REPAIR_RATE = (BOTH_PASS + GROUP_ONLY) / TOTAL_CASES
```

## Main severity sweep

| Severity | Faults/group | Cases | EARLY repair % | GROUP repair % | GROUP−EARLY pp | EARLY-only | GROUP-only | Both fail |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| LOW | 8 | 10,000 | 100.00 | 100.00 | 0.00 | 0 | 0 | 0 |
| LOW | 12 | 10,000 | 99.84 | 100.00 | +0.16 | 0 | 16 | 0 |
| LOW | 16 | 10,000 | 88.02 | 96.33 | +8.31 | 0 | 831 | 367 |
| MEDIUM | 20 | 10,000 | 80.67 | 84.14 | +3.47 | 0 | 347 | 1,586 |
| HIGH | 24 | 10,000 | 70.45 | 77.78 | +7.33 | 0 | 733 | 2,222 |
| HIGH | 28 | 10,000 | 60.62 | 73.39 | +12.77 | 0 | 1,277 | 2,661 |
| HIGH | 32 | 10,000 | 50.93 | 61.69 | +10.76 | 0 | 1,076 | 3,831 |

Across the primary moderate-imbalance severity sweep (70,000 paired groups):

```text
BOTH_PASS  = 55053
EARLY_ONLY = 0
GROUP_ONLY = 4280
BOTH_FAIL  = 10667

EARLY_REPAIR_RATE = 78.647143%
GROUP_REPAIR_RATE = 84.761429%
GROUP_MINUS_EARLY_PP = +6.114286 pp
```

## Imbalance sensitivity at 20 faults/group

| Imbalance model | Cases | EARLY repair % | GROUP repair % | GROUP−EARLY pp | EARLY-only % | GROUP-only % |
|---|---:|---:|---:|---:|---:|---:|
| Uniform | 10,000 | 99.51 | 99.88 | +0.37 | 0.00 | 0.37 |
| Moderate | 10,000 | 80.67 | 84.14 | +3.47 | 0.00 | 3.47 |
| Strong | 10,000 | 69.12 | 72.86 | +3.74 | 0.00 | 3.74 |

The paired GROUP-only fraction is larger for the nonuniform moderate/strong
models than for the uniform point.  This is a measured sensitivity in this
frozen corpus; it is not a claim of global GROUP dominance for every possible
fault model or topology.

## Overall paired statistics and policy classification

Across all nine measured points (90,000 paired groups):

```text
BOTH_PASS  = 71916  (79.906667%)
EARLY_ONLY = 0      (0.000000%)
GROUP_ONLY = 4691   (5.212222%)
BOTH_FAIL  = 13393  (14.881111%)

EARLY_REPAIR_RATE = 79.906667%
GROUP_REPAIR_RATE = 85.118889%
GROUP_MINUS_EARLY_PP = +5.212222 pp

POLICY_CLASSIFICATION = GROUP_DOMINATES
```

`GROUP_DOMINATES` is strictly a classification of this 90,000-instance,
fixed-seed DATE corpus: no sampled `EARLY_ONLY` result occurred and 4,691
`GROUP_ONLY` results did occur.  It must not be generalized into a proof that
the greedy GROUP policy dominates EARLY under all future distributions,
topologies, or architectures.

## Divergence diagnostics

Representative `GROUP_ONLY` cases are retained in
[`phase4j_divergence_examples.csv`](../results/phase4j/phase4j_divergence_examples.csv).
Each preserved record contains vector ID, per-SA valid/PatternID candidate
map, selected ConfigID and PatternID, donor, ledger before/after each SA, and
failure position for both policies.  No `EARLY_ONLY` artifact exists because
the measured corpus contained zero such outcomes.

The low-12/moderate example (`vector_id=103`) illustrates the policy effect:
EARLY takes local `CFG0` at A/B/C, then fails at D; GROUP selects release-aware
choices (`CFG4`, `CFG1`, `CFG1`) before choosing D `CFG6`, which makes the
required directional resource available and completes the group.  This is a
greedy ordering difference, not a new resource or a global search.

## Hardware and latency tradeoff

The hardware and post-BIST latency figures below are accepted Phase 4H/4I
measurements; Phase 4J ran no synthesis.

| Metric | EARLY | GROUP-NoScratch |
|---|---:|---:|
| Repair rate, all measured paired points | 79.906667% | 85.118889% |
| Cell area | 80,871.437539 | 105,393.658552 |
| GE | 8,104.00 | 10,561.33 |
| Architectural state | 73 bits | 157 bits |
| Candidate-history state | 0 bits | 80 bits |
| WNS at 20 ns | +0.01 ns | +0.00 ns |
| Representative DSS execution latency | about 4 cycles | about 20 cycles |
| `G_zero` P50 | 4 cycles | 20 cycles |
| `G_zero` P95 | 4 cycles | 21 cycles |

```text
GROUP_AREA_OVERHEAD = +30.3225%
GROUP_STATE_OVERHEAD = +84 bits
GROUP_MEDIAN_G_ZERO_OVERHEAD = +16 cycles
```

The measured repairability gain is non-negligible in the primary severity
sweep (+6.114286 percentage points), particularly at 16–32 faults/group, but
it is obtained with the known area, state, and decision-latency costs.

## Reproducibility and integrity

```bash
make test_phase4j_policy_tradeoff
make test_directional_multi_config_analyzer test_solution_take_policy
```

All checks passed:

```text
PAIRED_VECTOR_IDS_MATCH = PASS
SAME_NUMBER_OF_EARLY_GROUP_CASES = PASS
OUTCOME_CLASS_ACCOUNTING = PASS
NO_DUPLICATE_VECTOR_IDS = PASS
NO_MISSING_POLICY_RESULT = PASS

RELEVANT_DIRECTIONAL_ANALYZER_REGRESSION = PASS
RELEVANT_SOLUTION_TAKE_REGRESSION = PASS
```

Artifacts:

- [`phase4j_policy_tradeoff_raw.csv`](../results/phase4j/phase4j_policy_tradeoff_raw.csv)
- [`phase4j_policy_tradeoff_summary.csv`](../results/phase4j/phase4j_policy_tradeoff_summary.csv)
- [`phase4j_divergence_examples.csv`](../results/phase4j/phase4j_divergence_examples.csv)

## Phase control

```text
PHASE_4J_POLICY_TRADEOFF = PASS
PHASE_4J = COMPLETE

V2_PHASE = PHASE_4J_COMPLETE
V2_NEXT_PHASE_AUTHORIZED = NO
```
