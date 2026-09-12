# Phase 3E GROUP_COMPRESSED Functional Verification

`bash scripts/simulation/run_recam_phase3e_functional.sh` passes all required
directed policy checks. A/D A1–A5 verify CFG4, CFG0, CFG6, CFG5 priority and
the all-repairable case; B/C B1–B5 verify CFG1, CFG0, CFG3, CFG2 priority and
the all-repairable case. The same test instantiates EARLY and GROUP over each
map and verifies `early_solution_valid == group_solution_valid`, including four
cases where their selected map indices intentionally differ.

The regression also verifies valid-bit qualification, deterministic no-solution
zeros, PatternID and ten-bit candidate-bitmap propagation, selected-score
alignment, and repairable alignment from the same map record.

After Phase 3E implementation, the Phase 3D EARLY, Phase 3C, Phase 3B, and
RECAM paper-faithful regressions were rerun and pass. The subsequent Phase 3E
20 ns selector-only and integrated DC runs close; see
[PHASE3E_GROUP_SYNTHESIS_20NS.md](PHASE3E_GROUP_SYNTHESIS_20NS.md). This phase
adds no PhysicalResourceLedger or ownership logic.
