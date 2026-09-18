# DATE 2026 Formal Experiment Evidence Index

This index freezes completed DATE evidence.  It does not authorize a new RTL,
simulator, synthesis, device, or repair-rate phase.

## DATE-LAT-T1-1M

| Field | Frozen value |
| --- | --- |
| Evidence ID | `DATE-LAT-T1-1M` |
| Status | `FROZEN` |
| Formal run date | 2026-09-15 (Asia/Taipei) |
| Metric | Group-level provisional post-BIST analyze latency |
| Scenario | `E0L_DATE_2X2_LATENCY_V1` |
| Sample size | 1,000,000 groups |
| Fully hidden | 99.844600% |
| Mean exposed latency | 0.003884 cycles |
| P95 / P99 exposed latency | 0 / 0 cycles |
| Worst exposed latency | 4 cycles |
| Critical SA | A=0.000%, B=0.000%, C=0.000%, D=100.000% |

### What this evidence measures

```text
THIS RESULT MEASURES:
provisional candidate-analysis readiness

THIS RESULT DOES NOT MEASURE:
final ledger resolution
final repair decision
final solution commit
```

### Reproducibility and provenance

| Item | Frozen value |
| --- | --- |
| Fault generator | `src/DynamicFaultGenerator.cpp` |
| Scenario/config source | `tests/e0l_generate_candidate_corpus.cpp`; `docs/dss_execution/S1D_DATE_2X2_FORMAL_LATENCY_EXPERIMENT_PREFLIGHT.md` |
| Fault model | Uniform, 28 fixed faults/group, 7 per SA, Mixed spatial model |
| Master/scenario seed | `20260914` |
| Timing-model header SHA-256 | `7941aea357f4f6165bee566cd94b85b2c4ff5d69001ae823170f516d71c3d659` |
| Timing-model implementation SHA-256 | `073c5221242ccaa812e0dfab3f541a4b3093fa675f62e458fcbafc3bd7a02dcb` |
| DATE runner SHA-256 | `5780810fb1f7ab9ad82effb546edc001ec500f56f69a2154f537a51e8abc8d10` |
| Raw CSV SHA-256 | `784e1771cff947b90082576fb5798942c771b313a6e0cb1b7df9db7d1d210ba3` |
| Raw CSV | `results/date2026/t1_group_latency/raw/formal_1m_groups.csv` |

The reproducible commands used the frozen model and deterministic generator:

```bash
make t1_date_latency_experiment_b
build/bin/T1DateLatencyExperiment results/date2026/t1_group_latency stage_a 10000
make test_bist_overlap_timing_model
make test_fault_address_bist
build/t1r_retained_overlap/obj/Vrecam_dss_v2_retained_overlap_core
build/bin/T1DateLatencyExperiment results/date2026/t1_group_latency formal 100000
build/bin/T1DateLatencyExperiment results/date2026/t1_group_latency formal_1m 1000000
```

### Frozen artifacts

- `docs/dss_execution/T1_DATE2026_GROUP_LATENCY_FORMAL_RESULTS.md`
- `results/date2026/t1_group_latency/raw/formal_1m_groups.csv`
- `results/date2026/t1_group_latency/cdf/formal_1m_latency_cdf.csv`
- `results/date2026/t1_group_latency/cdf/formal_1m_latency_histogram.csv`
- `results/date2026/t1_group_latency/tables/formal_1m_date_summary.md`

The raw CSV is immutable evidence for this index entry and must not be
overwritten by a later simulator or architecture branch.
