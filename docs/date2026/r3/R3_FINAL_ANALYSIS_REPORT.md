# R3 final analysis report

## 1. Scope

This closure analyzes existing group-level formal evidence only. It does not
run a simulator, create a corpus, alter simulator semantics, or make an RTL,
PPA, device-level, or vendor-calibrated defect-rate claim. `n3` in the R3
archive is the RS=CS=3 group-sweep label, not the N3 CA-LIVE hardware corpus.

## 2. Formal corpus

The frozen source is `results/date2026/repair_rate/r3_formal_group/manifest/r3_manifest.json`, SHA-256 `309d538d902006fd6fc5d7a2b44a3adb15c08fd4e01bb75cc4bf7e9a68dc356d`.

| Field | Frozen value |
| --- | --- |
| Geometry / sharing | RS=3, CS=3, m=1 |
| F_GROUP | 8, 12, 16, 20, 24, 28, 32 |
| Seeds | 20263930, 20263934, 20263938, 20263942, 20263946, 20263950, 20263954 |
| Corpus | materialized `multinomial_uniform`, identical replay per point/policy |
| Groups | 100000 per point/policy |
| Formal evaluations | 63 policy-point results / 6.3 million policy-group evaluations |
| Raw sidecar root | `/home/asd9112000/repair_rate/results/date2026/repair_rate/r3_formal_group` (14G, 636 files; left in place) |

The frozen raw sidecars were found and readable. All 63 frozen result files
have 100001 lines (header plus 100000 groups), and all match their per-policy
summary and the archived 63-row RS=CS=3 aggregate.

Nine non-frozen sidecar points (81 policy-point files: N2 F8--F32 plus N3 F18
and F30) are recorded in the final point-status table and excluded from every
primary table and figure.

## 3. Policy definitions

The exact nine-policy set, topology, semantics, and candidate-universe
qualification are in `R3_FINAL_POLICY_SCOPE_TABLE.csv`. In particular,
Directional GROUP-GREEDY uses `frozen_date_2x2_m1`, whereas generic
Directional GROUP-GLOBAL uses `generic_recam_candidate_v1`. They are paired
empirical comparisons on the same corpus, but not merely different search
orders over the same candidate universe.

## 4. R3 optimization/runtime history

The frozen manifest records the final search contract as demand-equivalence
canonicalization plus reversible A-to-B-to-C-to-D DFS with incremental ledger
legality pruning, no cutoff, and no heuristic fallback. Its provenance commit
is `3304c86acd7e8114033e684acd721bf6daace57c`; the active runner is
`scripts/group/r3_formal_group_repair_rate.py`.

The earlier Cartesian-enumeration implementation and the proposed benchmark
figures `73022 -> 319 us/group` and N3/F12 `p95 ~= 6603 us/group)` were not
recoverable with an exact benchmark command, corpus, and stored timing output.
They are therefore **NOT VERIFIED** and are not used as final quantitative
evidence.

## 5. Main repair-rate results

| F_GROUP | LOCAL | Directional EARLY | GROUP-GREEDY | EARLY gain / GROUP-GREEDY gain vs LOCAL |
| ---: | ---: | ---: | ---: | ---: |
| 8 | 99.998% | 100.000% | 100.000% | +0.002 / +0.002 pp |
| 12 | 99.958% | 99.998% | 99.998% | +0.040 / +0.040 pp |
| 16 | 99.368% | 99.865% | 99.883% | +0.497 / +0.515 pp |
| 20 | 96.831% | 98.813% | 99.049% | +1.982 / +2.218 pp |
| 24 | 90.861% | 95.252% | 96.386% | +4.391 / +5.525 pp |
| 28 | 80.599% | 86.691% | 90.112% | +6.092 / +9.513 pp |
| 32 | 67.265% | 72.959% | 79.723% | +5.694 / +12.458 pp |

Figure R3-1 contains all four directly relevant 2x2 curves, including the
explicitly qualified generic GROUP-GLOBAL comparator. The full 63-row machine
table is `R3_FINAL_REPAIR_RATE_TABLE.csv`.

## 6. Capacity-boundary analysis

DSS gain is negligible at F8--F12, rises from F16 through F28, and remains
large at F32. Within this frozen range the largest observed LOCAL-to-GROUP-
GREEDY gain is +12.458 pp at F32; a falling side of the capacity curve is not
observed. Thus the evidence supports a proximity-to-local-capacity-boundary
interpretation, not a universal threshold or a claimed peak beyond F32.

## 7. Fault-imbalance analysis

The historical fixed-variance sequence (`0,16,25,36`) and its quoted gains
could not be tied to a frozen manifest and are **NOT VERIFIED**. Instead,
Figure R3-3 and `R3_FINAL_IMBALANCE_TABLE.csv` provide a frozen F24 supporting
analysis: groups are ranked by the population variance of the four local-SA
fault counts, with 20000 groups per quintile. EARLY gain rises from +0.530 pp
to +6.970 pp and GROUP-GREEDY from +0.650 pp to +9.190 pp between the lowest
and highest imbalance quintiles. This is a conditional F24 result, not a
general causal law for fault imbalance.

## 8. m sensitivity

The frozen formal scope contains m=1 only. Claims that m=1 captures a stated
fraction of m=2 gain are **DEFERRED** because their denominator/comparison was
not recovered from frozen evidence.

## 9. Topology sensitivity

The frozen set contains 2x2 directional/edge and 1x4 pair/neighbor policies.
It contains no 4x1 primary point. Cross-topology candidate universes differ,
so the table supports descriptive topology comparison only; it does not
identify a topology-only causal effect.

## 10. EARLY versus GROUP

For the directional 2x2 rows, GROUP-GREEDY equals EARLY at F8/F12 and is
higher by +0.018, +0.236, +1.134, +3.421, and +6.764 pp at F16/F20/F24/F28/F32.
This is a paired empirical result under different candidate contracts, not a
proof that deferred timing alone causes the difference. Architecturally,
EARLY commits immediately with lower retained-state complexity; GROUP retains
choice before selection. Hardware/state costs remain a separately scoped
CA-LIVE result (see `docs/date2026/hardware/N2_SIX_CASE_FINAL_STATUS.md`).

## 11. Paper-safe implications

| Implication | Status |
| --- | --- |
| DSS is most valuable near the local repair-capacity boundary | SUPPORTED, qualified to the frozen range |
| Larger local spare budget shifts the high-gain region | REMOVE: no multi-budget frozen comparison |
| m=1 captures most practical larger-m gain | REMOVE: no recovered denominator |
| EARLY trades retained flexibility for less retained state | SUPPORTED architecturally; repairability difference is contract-qualified |
| Modest pp gains can matter near feasibility boundaries | QUALIFIED; supported only by the reported group-level points |

## 12. Limitations

The result is group-level `GENERIC_CPP_CAPACITY` for 2x2 and `NOT_PROVEN` CAM
semantics for 1x4. It excludes non-frozen sidecars, does not establish global
optimality, and makes no N3 hardware repair-rate claim. Existing pre-closure
plots are retained as `STALE`; only the three files under `r3_final/figures/`
are final.

## 13. Provenance

The machine-readable evidence manifest records per policy-point paths and
SHA-256 hashes of the authoritative summary inputs, plus raw-sidecar paths.
No multi-GB raw corpus was copied into canonical or `dss_final`.

Final figures: `R3-1_main_repairability`, `R3-2_capacity_boundary`, and
`R3-3_fault_imbalance_f24` in PNG/PDF/SVG. They have no titles, include
legends, and use the project paper axis style.
