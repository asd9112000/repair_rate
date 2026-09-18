# T1 — DATE 2026 Group Provisional Candidate-Analysis Latency

## 1. Research question

For a 2x2 DSS group under the verified serial BIST schedule, this experiment measures the provisional candidate-analysis time exposed after BIST completes.

## 2. Frozen T1 timing model

The standalone `BistOverlapTimingModel` preserves the closed ownership-aware event-driven contract: serial A -> B -> C -> D BIST, starts A=0/B=16384/C=32768/D=49152, and group TEST_DONE=65536 cycles. Geometry is 512 rows x 8192 physical cell columns with a 256-bit BIST word and `floor(Fault::c / 256)` timing column. Current-owner readiness takes four uninterrupted sweep edges; non-owner readiness is ownership wait plus four edges. Same-edge accepted faults win over readiness. No ledger or final-commit latency is added.

## 3. Fault-generation model

The run reuses `DynamicFaultGenerator` in `src/DynamicFaultGenerator.cpp`, with the exact frozen DATE configuration in `tests/e0l_generate_candidate_corpus.cpp` and the `E0L_DATE_2X2_LATENCY_V1` recipe in `docs/dss_execution/S1D_DATE_2X2_FORMAL_LATENCY_EXPERIMENT_PREFLIGHT.md`:

- `Uniform`, 28 fixed faults/group, therefore 7 fixed faults per A/B/C/D (`lambda_SA=7`, not Poisson).
- `Mixed` spatial model: 0.20 cluster probability and 0.30 effective unconditional same-line probability.
- master seed = scenario seed = `20260914`.

Moderate/strong/hotspot are generic repair-rate stress settings, not frozen DATE latency presets, so they were not substituted here.

## 4. Experimental scenarios

| Scenario | Parameters |
| --- | --- |
| `E0L_DATE_2X2_LATENCY_V1` | 2x2 directional CAM, RS/CS/SHARE_M=2/2/1; Uniform 28 fixed/group; 7/SA; Mixed; seed 20260914 |

## 5. Seeds and sample sizes

Stage A generated 10,000 deterministic groups and had 10,000 valid groups. Stage B independently regenerated the deterministic formal prefix of 100,000 groups and had 100,000 valid groups. Because the standalone model is lightweight, a preserved supplemental 1,000,000-group baseline was also generated; it is the source for the primary DATE table below.

```text
Stage-A raw SHA-256: 9f499462b5af708fd797662d1273950af6e824ca9e79c97d8143afb3d5e396eb
Stage-B raw SHA-256: 9ef2ecf98c61683ad6667683c96256aa69184815076097e3939eddabfb3e8d93
Supplemental 1M raw SHA-256: 784e1771cff947b90082576fb5798942c771b313a6e0cb1b7df9db7d1d210ba3
```

## 6. T1-envelope handling

The retained-state envelope is `MAX_FAULTS_PER_SA=12`. Every group is checked before model evaluation. A violation is logged verbatim as `OUT_OF_T1_TIMING_ENVELOPE`, is not clamped or modified, and is excluded from latency statistics. Both stages have zero violations; each classification CSV contains its header only.

## 7. Validation before run

The required validation passed before the formal run:

```text
make test_bist_overlap_timing_model
T1_BIST_OVERLAP_TIMING_MODEL PASS directed=15 randomized=1000 mismatches=0

make test_fault_address_bist
Fault-address/BIST schedule tests passed

build/t1r_retained_overlap/obj/Vrecam_dss_v2_retained_overlap_core
S1GA2G_RETAINED_OVERLAP_CORE_TEST PASS
```

The RTL/C++ retained-controller comparison is zero mismatch for its reproducible successful trace. The DATE run itself neither modifies nor executes production RTL.

## 8. Group-level latency results

For each valid group, `GROUP_PROVISIONAL_POST_BIST_ANALYZE_LATENCY = max(0, latest Provisional Candidate Ready - 65536)` and `GROUP_HIDDEN_ANALYSIS_SLACK = 65536 - latest Provisional Candidate Ready`.

Across the 1,000,000-group supplemental baseline, exposed-latency mean/median/p90/p95/p99/max is `0.003884 / 0 / 0 / 0 / 0 / 4` cycles. Hidden-slack mean/median/p5/p95/minimum/maximum is `2859.315087 / 2166 / 160 / 7940 / -4 / 16304` cycles. The required 100,000-group formal result is retained separately under `formal_*` artifacts.

## 9. Per-SA catch-up results

| SA | Mean catch-up | Median | P95 | P99 | Max | Mean ownership wait |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 4.000000 | 4 | 4 | 4 | 4 | 0.000000 |
| B | 4.000000 | 4 | 4 | 4 | 4 | 0.000000 |
| C | 4.000000 | 4 | 4 | 4 | 4 | 0.000000 |
| D | 4.000000 | 4 | 4 | 4 | 4 | 0.000000 |

For this serial schedule and sparse fixed baseline, ownership is available at each final physical-fault event, so the measured catch-up is the four sweep edges.

## 10. Fully-hidden fraction

`99.8446%` (998,446/1,000,000) have all Provisional Candidates Ready by group BIST completion; `0.1554%` (1,554/1,000,000) have exposed latency greater than zero.

| Condition | Probability |
| --- | ---: |
| latency = 0 | 99.8446% |
| latency <= 1 cycle | 99.8825% |
| latency <= 2 cycles | 99.9223% |
| latency <= 4 cycles | 100.000% |
| latency <= 8 / 16 / 32 cycles | 100.000% / 100.000% / 100.000% |

## 11. P95/P99/tail behavior

P95 and P99 are zero cycles because the exposed tail is below one percent. Exact supplemental-CDF mass is 998,446 at 0 cycles, then 379/398/399/378 groups at 1/2/3/4 cycles. The four-cycle maximum is a D fault accepted at cycle 65536 followed by the frozen four clean sweep edges.

## 12. Critical-SA distribution

Critical SA is the first A->B->C->D SA whose candidate-ready cycle equals the group maximum. There were no ties in the formal output.

| A | B | C | D |
| ---: | ---: | ---: | ---: |
| 0.000% | 0.000% | 0.000% | 100.000% |

D is critical because it owns the final serial-BIST window; this is not a statement about final repair feasibility.

## 13. Worst-case examples

`tables/formal_1m_worst_10.csv` retains all SA fault counts, final accept cycles, ready cycles, ownership waits, critical SA, and the critical last-fault location. The first three IDs are 89, 3502, and 5713; each has D accepted at 65536, D ready at 65540, `hidden_slack=-4`, and exposed latency 4 cycles.

The descriptive Pearson correlation of latest group fault-accept cycle with exposed latency is `0.041163`; no causality is claimed. Fault-count correlation is undefined because every baseline group has exactly 28 faults. Quartile stratification is in `summary/formal_1m_fault_location_correlation.csv`.

## 14. Normalized latency

Against the 65,536-cycle BIST duration, mean exposed latency is `0.000006%`; p95 and p99 are `0.000000%`; maximum is `0.006104%`. These are cycle ratios, not time conversions.

## 15. DATE-ready result table

| Scenario | Fault parameters | Groups | Fully hidden | Mean exposed | Median | P95 | P99 | Max | Mean hidden slack |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| E0L_DATE_2X2_LATENCY_V1 | Uniform; 28 fixed/group; 7/SA; Mixed; seed 20260914 | 1,000,000 | 99.845% | 0.004 | 0 | 0 | 0 | 4 | 2859.315 |

Under the evaluated fault model, 99.8446% of 2x2 DSS groups complete provisional candidate analysis before the end of the 65,536-cycle serial BIST schedule, giving zero exposed post-BIST analyze latency. Across all groups, mean/median/p95/p99 exposed latency is `0.003884 / 0 / 0 / 0` cycles.

## 16. Interpretation

The measured distribution supports the limited conclusion that provisional candidate analysis is almost entirely hidden by BIST for this frozen DATE baseline. The rare exposed tail is bounded at four cycles and corresponds to the final D BIST word plus the specified sweep.

## 17. Limitations

This experiment characterizes provisional candidate-analysis timing. It does not include final shared-resource ledger resolution or final solution commit latency. It does not measure device latency, cross-group scheduling, repair rate, raw BIST collector handshakes, or a physical clock period.

`T2` remains `DEFERRED / FUTURE OPTIONAL INTEGRATION`; no `HierarchicalRECAM` path is used. `S1G-A2G synthesis` remains `PAUSED`.

## 18. Reproduction commands

```bash
make t1_date_latency_experiment_b
build/bin/T1DateLatencyExperiment results/date2026/t1_group_latency stage_a 10000
make test_bist_overlap_timing_model
make test_fault_address_bist
build/t1r_retained_overlap/obj/Vrecam_dss_v2_retained_overlap_core
build/bin/T1DateLatencyExperiment results/date2026/t1_group_latency formal 100000
build/bin/T1DateLatencyExperiment results/date2026/t1_group_latency formal_1m 1000000
```

Artifacts are under `results/date2026/t1_group_latency/{raw,summary,tables,cdf,logs}`.
