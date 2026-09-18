# DSS Thesis Execution — Cross-Window Handoff

> Status: Current handoff
> Last completed phase: H5
> Active phase: None
> Next authorized phase: None
> Updated: 2026-09-14

This document lets a later Codex window continue safely.  It is a navigation
and boundary document, not a replacement for the authoritative status,
evidence, and conflict records listed below.

## 1. Purpose and project boundary

The work extends the frozen DATE 2026 DSS V2 RTL from the verified 2x2
Directional CAM architecture point `(RS, CS, SHARE_M) = (2, 2, 1)` to the
isolated target point `(3, 3, 1)`.  The target remains a four-SA (`A,B,C,D`)
Directional topology with the existing V2-derived EARLY and GROUP-NoScratch
policies.

This is a scalability study of the existing DSS decision/resource-management
semantics.  It is not a new sharing policy, a globally optimal allocator, an
arbitrary-parameter implementation, or a final repair-map reconstruction
implementation.

The old DATE 2026 DSS V2 baseline is frozen through Phase 4K.  Do not alter
its RTL behavior, golden semantics, accepted characterization numbers, or
historical evidence as part of target work.

## 2. Read these documents first

Read in this order before editing or running an experiment:

1. `AGENTS.md`
2. `docs/dss_execution/DSS_EXECUTION_STATUS.md` — current phase authority
3. `docs/dss_execution/00_DSS_EXECUTION_MASTER.md` — scope and architecture
4. `docs/dss_execution/01_DSS_EXECUTION_PHASE_CONTROL.md` — phase gates
5. `docs/dss_execution/DSS_EXECUTION_CONFLICT_LOG.md` — open semantic issues
6. `docs/dss_execution/DSS_EXECUTION_MASTER_EVIDENCE.md` — accepted evidence
7. `docs/dss_execution/H0_REUSE_PARAMETERIZATION_AUDIT.md`
8. `docs/dss_execution/H1_RS3_CS3_M1_CONFIG_SPACE.md`
9. `docs/dss_execution/H2_RS3_CS3_M1_HARDWARE_SIZING.md`
10. `docs/dss_execution/H3_RS3_CS3_M1_RTL_IMPLEMENTATION.md`
11. `docs/dss_execution/H4_RS3_CS3_M1_FUNCTIONAL_VERIFICATION.md`
12. `docs/dss_execution/H5_RS3_CS3_M1_HARDWARE_CHARACTERIZATION.md`

For frozen 2x2 V2 context also consult `docs_verilog/PHASE_STATUS.md` and
`docs_verilog/PHASE4E_TO_4K_MASTER_EVIDENCE.md`.

## 3. Current execution state and hard stop

```text
DSS_EXECUTION_LAST_COMPLETED_PHASE = H5
DSS_EXECUTION_ACTIVE_PHASE         = NONE
H0/S0/H1/H2/H2R/H3/H4/H5           = COMPLETE
H5O_STATUS                         = NOT_NEEDED
NEXT_AUTHORIZED_PHASE              = NONE

S1/E0/E1/A0/A1/P0                  = NOT AUTHORIZED
D0/D1/D2/D3                        = PROPOSED / NOT AUTHORIZED
```

No new work phase may begin without an explicit authorization from the user.
In particular, do not start simulator integration, experiment execution,
Scratch work, device/global-CAM work, a timing/area optimization attempt, or
H5O merely because a future step is listed in this document.

## 4. Frozen target architecture and policy semantics

### Architecture point

```text
Layout/topology       = 2x2 Directional CAM
RS, CS, SHARE_M       = 3, 3, 1
SA traversal          = A -> B -> C -> D
Shared resources      = A_ROW, D_ROW, B_COL, C_COL
RESOURCE_NUM          = 4
```

`SHARE_M=1` means increasing local `RS/CS` does not create additional
shareable ledger resources.  A releaseable resource is one physical shared
line and may have at most one borrower.

### Role slots and configuration space

Each role has four semantic slots.  The seven physical configurations are
`3R3C`, `2R3C`, `3R4C`, `2R4C`, `3R2C`, `4R3C`, and `4R2C`.

| Role | Slot 0: LOCAL | Slot 1: RELEASE_ONLY | Slot 2: BORROW_ONLY | Slot 3: RELEASE_AND_BORROW |
|---|---|---|---|---|
| A/D | 3R3C | 2R3C | 3R4C | 2R4C |
| B/C | 3R3C | 3R2C | 4R3C | 4R2C |

Canonical mathematical classes and transpose normalization are:

```text
3R3C
2R3C <-> 3R2C
3R4C <-> 4R3C
2R4C <-> 4R2C
```

Candidate counts are fixed: 20 for 3R3C, 10 for 2R3C/3R2C, 35 for
3R4C/4R3C, and 15 for 2R4C/4R2C.  Maximum dimensions are `K=7`, a 49-bit
matrix, 7 address entries, 17 Hybrid entries, a 35-bit candidate bitmap, and
a 6-bit PatternID (`0` invalid; `1..35` valid for the maximum class).

### Frozen allocation semantics

```text
EARLY A/D rank:  0 -> 1 -> 2 -> 3
EARLY B/C rank:  0 -> 2 -> 1 -> 3
GROUP rank:      1 -> 0 -> 3 -> 2  (all roles)

A donor priority: B_COL then C_COL
D donor priority: C_COL then B_COL
B donor priority: A_ROW then D_ROW
C donor priority: D_ROW then A_ROW
```

Both policies use deterministic A-to-D traversal, latest-committed ledger
state, atomic legal commit, first-failure termination, and no rollback.
GROUP-NoScratch is a greedy resource-pressure-aware policy; it is neither
global exhaustive search nor a dominance policy over EARLY.

The target GROUP candidate-history architectural minimum is 112 bits:
`4 SA x 4 slots x (candidate_valid + PatternID[5:0])`.  Config/action identity
is implicit in role-slot position.  The target's measured retained state is
197 bits, including that history plus control, ledger, selected-result, and
diagnostic state.

## 5. Implementation locations and isolation boundary

All target-specific RTL is isolated here:

```text
rtl/dss_v2/rs3cs3m1/
  dss_v2_rs3cs3m1_config_table.sv
  dss_v2_rs3cs3m1_shared_config_analyzer.sv
  dss_v2_rs3cs3m1_topology.sv
  dss_v2_rs3cs3m1_group_candidate_store.sv
  recam_dss_v2_rs3cs3m1_early_core.sv
  recam_dss_v2_rs3cs3m1_early_top.sv
  recam_dss_v2_rs3cs3m1_group_core.sv
  recam_dss_v2_rs3cs3m1_group_top.sv
```

The existing V2 ledger is reused without a semantic redesign.  The legacy
2x2 analyzer and accepted Phase-4 evidence must remain unchanged.

Target smoke/verification sources are under `tb/dss_v2/`; the independent
target policy golden is
`tb/dss_v2/recam_dss_v2_rs3cs3m1_policy_equivalence_test.cpp`, with the
test-only RTL bridge at
`tb/dss_v2/recam_dss_v2_rs3cs3m1_policy_equivalence_top.sv`.

## 6. Completed functional evidence (H3/H4)

H3 implementation and smoke closure completed without modifying simulator
repair semantics.  H4 then completed the independent target functional gate.

```text
H4 random seed                    = 20260910
EARLY vectors / RTL-golden errors = 1000 / 0
GROUP vectors / RTL-golden errors = 1000 / 0

Paired outcomes:
  BOTH_PASS  = 579
  EARLY_ONLY = 0
  GROUP_ONLY = 116
  BOTH_FAIL  = 305
```

The H4 harness covers all seven configurations, the candidate-count and
bitmap boundaries, PatternID `0/1/35`, K=7, pivot entry 6, Hybrid entry 16,
per-configuration Must thresholds, all transpose pairs, action/donor/ledger
traces, first failure, and no rollback.  Its directed suite includes local,
release-only, primary/alternate donor, release-and-borrow, conflict, and
fail-at-A/B/C/D cases for the relevant policies.

The target source provenance recorded for H4 is Git `3304c86` and SHA-256
`854572fd9cd7ec10470ebc261887e4cd5f5d5d1de17b725a671e90c2c940f4b7`.

The old frozen 2x2 regression also passed unchanged:

```bash
bash scripts/simulation/run_recam_phase3hi_functional.sh
```

Use the target H4 runner only when a later authorized task calls for it:

```bash
bash scripts/simulation/run_dss_v2_rs3cs3m1_h4_functional.sh
```

## 7. Completed hardware evidence (H5)

H5 completed the original, H4-verified target RTL mapping using Synopsys DC
W-2024.09-SP2, TSMC018 `slow.db`, 20-ns `clk_i`, zero IO delays,
`tsmc18_wl10`, `compile -map_effort low`, and NAND2X1 area basis
`9.979200`.  Raw reports are retained in:

```text
results/dss_v2_rs3cs3m1/h5_hardware_retry2/EARLY/
results/dss_v2_rs3cs3m1/h5_hardware_retry2/GROUP/
results/dss_v2_rs3cs3m1/h5_hardware_retry2/hardware_characterization.csv
```

| Point | Area | GE | Comb. / seq. area | Comb. / seq. cells | Critical path | WNS / TNS | Timing |
|---|---:|---:|---:|---:|---:|---:|---|
| 2,2,1 EARLY baseline | 80,871.44 | 8,104.00 | 76,769.99 / 4,101.45 | 4,039 / 75 | 19.70 ns | +0.01 / 0.00 ns | CLOSED |
| 3,3,1 EARLY target | 12,190,913.30 | 1,221,632.33 | 12,186,139.92 / 4,773.38 | 297,933 / 83 | 62.52 ns | -42.65 / -3531.71 ns | NOT CLOSED |
| 2,2,1 GROUP baseline | 105,393.66 | 10,561.33 | 96,611.96 / 8,781.70 | 5,162 / 157 | 19.74 ns | +0.00 / 0.00 ns | CLOSED |
| 3,3,1 GROUP target | 12,053,273.51 | 1,207,839.66 | 12,042,276.44 / 10,997.08 | 286,767 / 197 | 64.14 ns | -44.24 / -4951.16 ns | NOT CLOSED |

The original runs completed under the authorized 14,400-s limit (EARLY
9,428.95 s; GROUP 8,635.48 s).  `STATUS=PASS` in their metadata means DC
completed with resolved design/library and reports; it does **not** mean
20-ns timing closed.

The 3,3 analyzer/candidate evaluation dominates mapped area (99.9% EARLY,
99.5% GROUP) and the critical paths.  This is evidence for this exact generic
enumerative RTL representation, not a proof that a single isolated
architectural structure alone caused the scaling.  Raw mode-reused CAM
requirements, 280 bits for 2,2 and 536 bits for 3,3, are storage accounting
only and must never be reported as synthesized CAM area.

Because the original H5 mapping completed, the conditional H5O optimization
branch was not started and is `NOT_NEEDED`.  A later timing/area optimization
or synthesis rerun requires fresh explicit authorization and must preserve the
H4 semantic boundary before its results can be compared.

## 8. Open conflict and experiment/device boundary

`H0S0-CL-004` remains **OPEN**.  Existing C++
`DynamicRepairSimulator::findCompressedGroupChoice` enumerates cross-SA
combinations, whereas frozen V2 GROUP-NoScratch uses greedy ranked A-to-D
commits.  Existing DynamicSpareSharing `GroupCompressed` results must not be
renamed, merged with, or used as evidence for V2 GROUP-NoScratch.

Consequences:

- S1, E0, and E1 are not ready or authorized until the policy-semantic
  calibration conflict is resolved under a separate authorization.
- Do not assert `EARLY success => GROUP success`; H4 paired results show the
  policies can legally differ.
- Do not make repairability, repair-rate, or cost-per-repairability claims
  from the H5 hardware data.

The planned device workstream is distinct from group evidence:

```text
Tier0: local spare resources
Tier1: DSS borrowing within one four-SA group
Tier2: persistent finite global online CAM per device
```

Tier1 borrowing is not Tier2 CAM reuse.  Device state must persist across
groups; group and device result streams must remain separate (for example,
`reports/group/...` versus `reports/device/...`) and must never be averaged
or merged without an explicitly justified mapping.

## 9. Proposed work only — not authorization

These are candidates for a future user-approved work package, not instructions
to execute now:

| Candidate | Purpose | Prerequisite / boundary |
|---|---|---|
| S0R | Audit/calibrate the simulator GROUP policy against frozen V2 GROUP-NoScratch | Resolves H0S0-CL-004; no relabeling of legacy results |
| S1 / E0 / E1 | Simulator experiment and evidence work | Require explicit authorization and S0R semantic clearance |
| A0 / A1 | Scratch architectural audit / RTL | A0 is technically ready but not authorized; preserve reconstruction boundary |
| D0-D3 | Device integration, Tier2 demand/capacity/mechanism studies | Proposed only; maintain persistent per-device CAM semantics |
| Future H5O-like work | Semantics-preserving representation/timing investigation | Needs new authorization; H5O itself was not triggered |

## 10. Repository and worktree safety

Before any work run:

```bash
git status --short
git diff --check
```

The worktree already contains user-owned unrelated changes, including
`.gitignore`, broad documentation/script edits, and untracked target/result
directories.  Preserve them.  Do not use reset/checkout to clean the tree and
do not overwrite historical result directories or accepted evidence.

## 11. Safe handoff procedure for the next window

1. Read the documents in Section 2 and check `DSS_EXECUTION_STATUS.md`.
2. Verify the requested task is explicitly authorized.  If not, stop and ask
   for an authorization package; do not infer one from the proposed roadmap.
3. Identify whether the work belongs to frozen 2x2 V2, isolated 3,3 target
   RTL, group simulation, or future device simulation.  Never mix their
   metrics or semantic labels.
4. Preserve H4's observable equivalence boundary (selected action/PatternID,
   donor/release action, ledger trace, failure position, group result) for any
   semantics-preserving target RTL change.
5. For any new synthesis, retain exact tool/library/clock/provenance and
   report timing closure separately from successful tool completion.  DC must
   run from the user's licensed EDA shell; it is not available in the Codex
   sandbox by default.
6. Update only the status/evidence/conflict documents authorized by the future
   task, and leave `NEXT_AUTHORIZED_PHASE` unchanged unless the user grants
   the next phase explicitly.

## 12. Current final pointer

```text
TARGET                 = 2X2_DIRECTIONAL_CAM_RS3_CS3_M1
POLICIES               = EARLY, GROUP_NOSCRATCH
FUNCTIONAL EVIDENCE    = H4 COMPLETE (1000 + 1000, zero RTL/golden mismatch)
HARDWARE EVIDENCE      = H5 COMPLETE (original RTL mapped; 20-ns timing open)
OPEN SEMANTIC CONFLICT = H0S0-CL-004
ACTIVE PHASE           = NONE
NEXT AUTHORIZED PHASE  = NONE
```
