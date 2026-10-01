# N3 RC-GROUP Formal Latency Characterization

## Measurement contract

Latency is measured from the rising edge that accepts the final active-SA state update to the rising edge that registers the current-generation four-SA GROUP solution-ready result:

```text
latency_cycles = registered_solution_ready_cycle
               - final_active_sa_state_update_cycle
```

Raw fault arrival, BIST detection, external collection, and testbench setup are outside this boundary. The internal combinational GROUP-decision cycle is not externally observable and production RTL was not modified to expose it.

## FORMAL_10K

The primary corpus contains exactly 10,000 deterministic `GENERAL` cases generated with seed `0x4e33524f`. It is a latency/control characterization corpus, not a repair-rate experiment or real-world defect distribution.

```text
cases                         10000
functional mismatches         0
repairable                    0
unrepairable                  10000
stale generation mix          0
earlier-SA replay             0
partial scan finalization     0
old candidate accepted        0
```

The observed repairable fraction is diagnostic only and is explicitly **not a repair-rate result**. Generation probabilities were not adjusted to balance outcomes.

Exact latency histogram:

| Cycles | Count |
|---:|---:|
| 5 | 10000 |

Statistics:

```text
minimum   5 cycles
mean      5.0 cycles
median    5 cycles
p95       5 cycles
maximum   5 cycles
fixed     YES
```

Median uses the middle ordered observations; both central observations are 5. P95 uses deterministic nearest rank, rank `ceil(0.95 * 10000) = 9500`, whose value is 5.

The formal cycle trace verified for every case is:

```text
final-SA update accepted      offset 0
Config 1 result stored        offset 1
Config 2 result stored        offset 2
Config 3 result stored        offset 3
Config 4 result stored        offset 4
registered solution ready     offset 5
```

This supports the decomposition of four externally observed Config-evaluation steps plus one registered solution-ready event. Representative absolute local harness cycles for cases 0, 4999, and 9999 are archived in `N3_RC_GROUP_LATENCY_TRACE.csv`.

Because all formal cases were unrepairable, the formal corpus has zero valid selected GROUP tuples and zero valid selected PatternIDs. Consequently:

```text
selected Config distribution denominator   0 valid SA selections
selected PatternID distribution denominator 0 valid SA selections
PatternID <= 15 count                       0
PatternID > 15 count                        0
maximum selected PatternID                  N/A
```

The zero Config/Pattern fields in unrepairable per-case CSV rows are invalid-output placeholders and are not counted as selected Config 0 or PatternID 0.

## DIRECTED_COVERAGE_SIDECAR

The six-case sidecar is separate from the formal histogram and contains one case from each directed generator mode: `REPAIRABLE`, `UNREPAIRABLE`, `HIGH_PATTERN`, `SEVENTH_PIVOT`, `HYBRID_GT_7`, and `PHYSICAL_COLUMN_WIDE`.

```text
cases                         6
repairable                    5
unrepairable                  1
functional mismatches         0
latency histogram             5:6
seventh pivot                 PASS
Hybrid entry > 7              PASS
PatternID > 15                PASS (validated PatternID 20 witness)
physical column wide          PASS
full-width address guard      PASS
stale generation mix          0
earlier-SA replay             0
partial scan finalization     0
old candidate accepted        0
```

These six cases prove coverage only and are not included in the formal 10,000-case denominator or histogram.

## PRIOR_100_CASE_SMOKE

After the formal-recording harness extension, the prior Layer-C smoke was rerun with the same seed and 100-case mixed smoke policy:

```text
cases                         100
repairable                    5
unrepairable                  95
functional mismatches         0
latency histogram             5:100
all directed coverage gates   PASS
```

This remains `SMOKE_ONLY_NON_FORMAL` and is not merged with formal statistics.

## Evidence and integrity

- Per-case evidence: `N3_RC_GROUP_LATENCY_CASES.csv` (10,000 rows plus header).
- Exact histogram: `N3_RC_GROUP_LATENCY_HISTOGRAM.csv`.
- Representative cycle traces: `N3_RC_GROUP_LATENCY_TRACE.csv`.
- Formal harness compares candidate records, repairability, Config, action-derived resource signals, six-bit PatternID, resource ledger, line-valid, row/column identity, and full 13-bit addresses against the Layer-A/B oracle chain for every case.
- Production RTL hashes are identical before and after the formal run.
- Source freeze, synthesis, commit, push, tag, and merge were not performed.

