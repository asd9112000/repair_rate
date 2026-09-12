# DSS V2 Master Evidence Record — Phases 4E to 4K

## Scope and evidence boundary

This is the authoritative DATE-scope consolidation of completed DSS V2 work.
It covers the frozen 2×2 Directional DSS decision/resource-management boundary
only.  It does not claim final pivot/Hybrid reconstruction, repair-table
generation, runtime remapping, eFuse programming, SRAM, 1×4, Scratch hardware,
or a new sharing policy.

```text
Phase 4E = COMPLETE                 Specialized EARLY
Phase 4F = COMPLETE                 GROUP-NoScratch
Phase 4G = COMPLETE_NEGATIVE_RESULT GROUP-WithScratch audit
Phase 4H = COMPLETE                 Unified hardware characterization
Phase 4I = COMPLETE                 Post-BIST latency characterization
Phase 4J = COMPLETE                 Paired repairability tradeoff
Phase 4K = COMPLETE                 GROUP_ONLY mechanism analysis
```

## 1. Functional correctness

Both policies preserve the frozen boundary: selected ConfigID/PatternID,
donor/release action, ledger evolution, failure position, and group
repairability.

| Policy | Directed verification | 50-vector bring-up | 1,000-vector regression | Mismatches |
|---|---|---:|---:|---:|
| V2 EARLY | PASS | PASS | PASS | 0 |
| V2 GROUP-NoScratch | PASS | PASS | PASS | 0 |

The V2 GROUP result includes `GROUP_PHYSICAL_FEASIBILITY_FALLBACK =
N/A_PROVEN_UNREACHABLE`: with the frozen topology, a donor-caused slot3
infeasibility also makes slot2 infeasible because the candidates have the same
donor pair.  This is a proven unreachable condition, not a failed test.

Evidence: [PHASE3HI_VERIFICATION.md](PHASE3HI_VERIFICATION.md),
[PHASE4F_CHARACTERIZATION.md](PHASE4F_CHARACTERIZATION.md), and the accepted
V2 equivalence regressions.

## 2. Hardware characterization

All areas are verified DC **Total cell area** under TSMC018 `slow.db`, a 20-ns
clock, and NAND2X1 reference area 9.979200.  WNS is reported at the same
constraint.

| Metric | Legacy EARLY | V2 EARLY | Legacy GROUP | V2 GROUP-NoScratch |
|---|---:|---:|---:|---:|
| Cell area | 117,219.010491 | 80,871.437539 | 117,139.176890 | 105,393.658552 |
| GE | 11,746.33 | 8,104.00 | 11,738.33 | 10,561.33 |
| WNS at 20 ns | +0.01 ns | +0.01 ns | +0.01 ns | +0.00 ns |
| Sequential area | 25,882.718727 | 4,101.451260 | 25,882.718727 | 8,781.696129 |
| Sequential cells | 485 | 75 | 485 | 157 |
| Architectural state | 485 bits | 73 bits | 485 bits | 157 bits |
| Candidate-history state | about 400 bits | 0 bits | about 400 bits | 80 bits |
| ConfigPatternMap present | Yes | No | Yes | No |

```text
V2 EARLY versus legacy EARLY area reduction = 31.0083%
V2 GROUP versus legacy GROUP area reduction = 10.0270%

GROUP versus V2 EARLY area overhead = 24,522.221013 (+30.3225%)
GROUP versus V2 EARLY state overhead = +84 bits
GROUP versus V2 EARLY candidate-history overhead = +80 bits
```

`73 bits` is the architectural-state accounting for EARLY.  DC maps 75
sequential cells; the two-cell difference is the Phase 4C transient
commit-status state included in mapped implementation accounting, not retained
architectural decision state.  This is an accounting-scope distinction, not a
numerical conflict.

Evidence: [PHASE4E_CHARACTERIZATION.md](PHASE4E_CHARACTERIZATION.md),
[PHASE4F_CHARACTERIZATION.md](PHASE4F_CHARACTERIZATION.md), and
[PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md](PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md).

## 3. Synthesis observations

- The shared analyzer is 70,938.8070 cell area, 87.7% of V2 EARLY total area.
  Removing history therefore produces very large sequential-area savings, but
  the combinational analyzer remains the dominant irreducible cost.
- In V2 GROUP-NoScratch, the analyzer is 73,756.2678 area (70.0%) and the
  compressed 80-bit candidate store is 21,129.2930 area (20.0%).  GROUP's
  required retained history makes its total-area reduction smaller than EARLY's.
- EARLY's worst registered path is controller SA state → ConfigID → shared
  analyzer → `solution_valid` → ledger register, with 19.70 ns arrival and
  +0.01 ns WNS.  GROUP's is SA/slot decode → analyzer → candidate-store write,
  with 19.74 ns arrival and +0.00 ns WNS.
- Specialization is more effective for EARLY because a candidate's lifetime
  ends at its first feasible decision.  GROUP must preserve valid+PatternID
  across collection until deferred allocation, leaving a proven 80-bit minimum.

Detailed, source-linked observations are in
[DSS_V2_SYNTHESIS_OBSERVATIONS.md](DSS_V2_SYNTHESIS_OBSERVATIONS.md).

## 4. Phase 4G negative result

```text
WITH_SCRATCH_ARCHITECTURALLY_JUSTIFIED = NO
```

Lifetime analysis ruled out Scratch as beneficial for the current GROUP
decision boundary: the required minimum retained candidate information remains
80 bits regardless of per-SA storage placement.  Recompute would require a new
input-stability contract or an approximately 204-bit raw-input snapshot, which
does not establish a storage advantage.  This is architecture-space pruning,
not an implementation failure.

## 5. Latency characterization

**DSS execution latency** (`start_i` accepted to `done_o`) and **post-BIST
exposed DSS decision latency** are distinct.  The latter uses:

```text
G = T_BIST_end - T_last_fault
L_post = max(0, T_decision_ready - T_BIST_end)
```

The endpoint is DSS decision ready: repairability, selected ConfigID/PatternID,
donor/release decision, and ledger final.  It is not repair-table-ready.

| Metric | EARLY | GROUP-NoScratch |
|---|---:|---:|
| Representative DSS execution latency | all-local about 4 cycles | all-local 20 cycles |
| Failure-B/C/D execution latency | — | 21 / 22 / 23 cycles |
| Slot2-last execution latency | — | 26 cycles |
| Mean `G_zero` | 4.0095 | 20.3156 |
| `G_zero` P50 | 4 | 20 |
| `G_zero` P95 | 4 | 21 |
| `G_zero` maximum | 7 | 24 |

The median zero-exposed-latency slack delta is 16 cycles (P95 delta 17).
Across 10,000 paired traces and 33 gaps, 330,000 paired observations were
measured: EARLY < GROUP in 203,156 (61.562424%), equal in 126,844
(38.437576%), and EARLY > GROUP in 0.

Eight representative production-top RTL cases matched the event model with
zero cycle mismatches, including same-edge zero latency, a non-dominant EARLY
path, and a longer GROUP path.

Evidence: [PHASE4I_FINAL_CHARACTERIZATION.md](PHASE4I_FINAL_CHARACTERIZATION.md).

## 6. Repairability and mechanism

Phase 4J used 90,000 paired groups (`seed=20260910`), sharing identical fault
maps, candidate generation, initial ledger, and topology between policies.

```text
BOTH_PASS  = 71,916
EARLY_ONLY = 0
GROUP_ONLY = 4,691
BOTH_FAIL  = 13,393

EARLY_REPAIR_RATE = 79.906667%
GROUP_REPAIR_RATE = 85.118889%
GROUP_MINUS_EARLY = +5.212222 percentage points
```

`GROUP_DOMINATES` applies empirically to this fixed DATE corpus only; it is
not a universal or mathematical dominance claim.

| Faults/group, moderate imbalance | EARLY % | GROUP % | Delta pp |
|---:|---:|---:|---:|
| 8 | 100.00 | 100.00 | 0.00 |
| 12 | 99.84 | 100.00 | +0.16 |
| 16 | 88.02 | 96.33 | +8.31 |
| 20 | 80.67 | 84.14 | +3.47 |
| 24 | 70.45 | 77.78 | +7.33 |
| 28 | 60.62 | 73.39 | +12.77 |
| 32 | 50.93 | 61.69 | +10.76 |

The gain is small under low pressure and material at several higher-pressure
points; the non-monotonic delta must not be described as a monotonic law.

Phase 4K traced all 4,691 GROUP_ONLY cases.  Every first divergence occurs at
A: EARLY takes `CFG0`; GROUP takes `CFG4` and releases `A_ROW`.  This proactive
release changes downstream ledger availability; EARLY later fails at B/C/D in
929/1,398/2,364 cases.  There are no unexplained first-divergence cases.

```text
EARLY_ONLY_ZERO_CLASSIFICATION = EMPIRICAL_ONLY
```

Evidence: [PHASE4J_POLICY_TRADEOFF.md](PHASE4J_POLICY_TRADEOFF.md) and
[PHASE4K_GROUP_DOMINANCE_MECHANISM.md](PHASE4K_GROUP_DOMINANCE_MECHANISM.md).

## 7. Final EARLY versus GROUP tradeoff

| Metric | EARLY | GROUP-NoScratch |
|---|---:|---:|
| Repair rate, measured corpus | 79.906667% | 85.118889% |
| Repairability delta | baseline | +5.212222 pp |
| Cell area | 80,871.437539 | 105,393.658552 |
| GE | 8,104.00 | 10,561.33 |
| Architectural state | 73 bits | 157 bits |
| Candidate history | 0 bits | 80 bits |
| WNS at 20 ns | +0.01 ns | +0.00 ns |
| Representative execution latency | about 4 cycles | about 20 cycles |
| `G_zero` P50 / P95 | 4 / 4 | 20 / 21 |

EARLY is the implementation-efficient design point: lower area, retained
state, execution latency, and post-last-fault slack.  GROUP-NoScratch is the
repairability-oriented design point: it costs more area/state/latency but
obtains materially higher repairability in the evaluated corpus through
proactive shared-resource release.

## Reproduction and evidence index

```bash
make test_phase4j_policy_tradeoff
make test_phase4k_group_dominance_analysis
make test_directional_multi_config_analyzer test_solution_take_policy
```

The paper claim boundaries and figure priorities are separately recorded in
[DSS_V2_PAPER_CLAIMS_AUDIT.md](DSS_V2_PAPER_CLAIMS_AUDIT.md) and
[DSS_V2_PAPER_FIGURE_PLAN.md](DSS_V2_PAPER_FIGURE_PLAN.md).

```text
MASTER_EVIDENCE_CONSOLIDATED = YES
V2_PHASE = PHASE_4K_COMPLETE
ACTIVE_PHASE = NONE
NEXT_ARCHITECTURE_PHASE_AUTHORIZED = NO
```
