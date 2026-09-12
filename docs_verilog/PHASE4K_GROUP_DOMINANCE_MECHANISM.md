# Phase 4K — GROUP Dominance Mechanism Analysis

## Scope and method

Phase 4K is a post-processing analysis of the accepted Phase 4J paired raw
artifact.  It does not regenerate faults, change RTL, change either frozen
policy, run synthesis, or add topology/storage variants.

The input is all 90,000 Phase 4J paired results generated with seed
`20260910`.  The analysis selects all and only the accepted `GROUP_ONLY`
population (`EARLY` fails, `GROUP-NoScratch` succeeds), then traces their
existing selected ConfigID/PatternID, donor, and ledger states.

```text
GROUP_ONLY_CASES = 4691
RAW_ARTIFACT = phase4j_policy_tradeoff_raw.csv
```

For every GROUP_ONLY case, the first divergent SA is the first position at
which ConfigID, PatternID, or donor differs.  A failed selection has no commit,
so its post-decision ledger is its recorded pre-decision ledger.

## Traceability and first divergence

| First divergent SA | Count | Fraction |
|---|---:|---:|
| A | 4,691 | 100.000000% |
| B | 0 | 0.000000% |
| C | 0 | 0.000000% |
| D | 0 | 0.000000% |

All first divergences occur before EARLY's eventual failure: `4,691 / 4,691`
(100%).  Thus the observed benefit is proactive resource preparation, not a
different choice made only at the failure point.

The first-divergence Config transition matrix contains exactly one transition:

| Role | EARLY Config | GROUP Config | Count |
|---|---|---|---:|
| A/D (observed SA A) | CFG0 | CFG4 | 4,691 |

At A, EARLY's `CFG0` is local and leaves `A_ROW` private.  GROUP's highest
ranked usable `CFG4` releases `A_ROW`.  This is the measured initiating
mechanism for every GROUP_ONLY trace in the corpus.

## EARLY failure position

| EARLY failure position | GROUP_ONLY count | Fraction |
|---|---:|---:|
| A | 0 | 0.000000% |
| B | 929 | 19.803880% |
| C | 1,398 | 29.801748% |
| D | 2,364 | 50.394372% |

The first unavailable donor candidate at EARLY's failed SA is `A_ROW` for the
B failures, `D_ROW` for the C failures, and `C_COL` for the D failures.  This
is a diagnostic first-blocked-donor label, not an assertion that no secondary
donor was also unavailable.

## Resource-effect accounting

The following categories classify the **first divergent decision** of every
GROUP_ONLY trace.  They do not force later differences into the same category.

```text
GROUP_MORE_RELEASE          = 4691
GROUP_LESS_BORROW           = 0
GROUP_DIFFERENT_DONOR       = 0
GROUP_SAME_RESOURCE_EFFECT  = 0
MULTI_EFFECT                = 0
UNEXPLAINED_CASES           = 0
```

Consequently, the Phase 4J gain is not explained by GROUP taking a different
donor or merely avoiding an EARLY borrow at the first divergent SA.  It is
explained by the GROUP release-first rank selecting `CFG4` at A, establishing a
shared-resource ledger state that enables later release/borrow choices in the
same greedy A→B→C→D traversal.

Later GROUP choices remain visible in the saved traces.  For example, the
representative D-failure trace releases `A_ROW` at A, then `B_COL` and `C_COL`
at B/C (`CFG1`), allowing D `CFG6` to borrow `C_COL`; EARLY keeps local CFG0
at those SAs and fails at D.  This is an actual greedy resource-availability
chain, not a global optimization.

## Severity dependence

The main Phase 4J moderate-imbalance sweep accounts for the following
GROUP_ONLY population:

| Faults/group | GROUP_ONLY cases |
|---:|---:|
| 8 | 0 |
| 12 | 16 |
| 16 | 831 |
| 20 | 347 |
| 24 | 733 |
| 28 | 1,277 |
| 32 | 1,076 |

At 20 faults/group, the existing imbalance sensitivity points contain 37
GROUP_ONLY cases for uniform, 347 for moderate, and 374 for strong imbalance.
The concentration at nonuniform/higher-pressure points is consistent with the
release-preservation mechanism, but this remains an empirical observation of
the frozen corpus.

## Representative measured traces

Three actual traces are retained, covering each observed EARLY failure
position:

| Faults/group | Imbalance | Vector ID | First divergence | EARLY failure | First blocked donor |
|---:|---|---:|---|---|---|
| 12 | moderate | 103 | A: CFG0 → CFG4 | D | C_COL |
| 20 | moderate | 444 | A: CFG0 → CFG4 | C | D_ROW |
| 24 | moderate | 3,208 | A: CFG0 → CFG4 | B | A_ROW |

Each row's full candidate-valid/PatternID map, four-SA EARLY/GROUP selection,
donor, and ledger sequence is in
[`phase4k_representative_traces.csv`](../results/phase4k/phase4k_representative_traces.csv).

## Zero EARLY_ONLY audit

`EARLY_ONLY = 0` was checked against all 90,000 Phase 4J paired outcomes.  No
formal dominance proof was performed: the frozen ranks are two different greedy
policies, and the Phase 4J/4K CSV analysis cannot establish behavior outside
the sampled fault models and topology.

```text
EARLY_ONLY_ZERO_CLASSIFICATION = EMPIRICAL_ONLY
```

The absence of EARLY_ONLY cases is neither attributed to a hidden simulator
constraint nor promoted to a mathematical consequence of the ranking.

## Reproduction and artifacts

```bash
make test_phase4k_group_dominance_analysis
```

The test reads Phase 4J's existing raw data and validates that the classified
population has exactly 4,691 unique GROUP_ONLY IDs with no unexplained cases.

- [`phase4k_group_only_classification.csv`](../results/phase4k/phase4k_group_only_classification.csv)
- [`phase4k_config_transition_matrix.csv`](../results/phase4k/phase4k_config_transition_matrix.csv)
- [`phase4k_representative_traces.csv`](../results/phase4k/phase4k_representative_traces.csv)

## Phase control

```text
PHASE_4K_GROUP_DOMINANCE_ANALYSIS = PASS
PHASE_4K = COMPLETE

V2_PHASE = PHASE_4K_COMPLETE
V2_NEXT_PHASE_AUTHORIZED = NO
```
