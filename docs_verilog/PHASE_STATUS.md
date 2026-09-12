# RTL Phase Status

> Scope: RTL hardware project only. This status does not describe the C++ simulator roadmap.
> Updating this file alone does not authorize implementation of another phase.
> The legacy Phase 0–3 status below is retained as historical evidence. The DSS V2
> Phase 4 state is the current project pointer.

CURRENT_PROJECT_STATE = DATE_2026_DSS_V2
LAST_COMPLETED_PHASE = PHASE_4K
ACTIVE_PHASE = NONE
NEXT_AUTHORIZED_PHASE = NONE
NEXT_PROPOSED_PHASE = NONE

LEGACY_CURRENT_PHASE: 3J

## DATE 2026 DSS V2 Freeze Marker

```text
DATE_2026_DSS_V2_FREEZE_AFTER_PHASE_4K = TRUE

CURRENT_PROJECT_STATE = DATE_2026_DSS_V2
LAST_COMPLETED_PHASE  = PHASE_4K
ACTIVE_PHASE          = NONE
NEXT_AUTHORIZED_PHASE = NONE

ARCHITECTURE_EXPLORATION = FROZEN
RTL_BEHAVIOR             = FROZEN
GOLDEN_SEMANTICS         = FROZEN
PAPER_NUMERIC_RESULTS    = FROZEN
```

The active DATE scope is the frozen 2×2 Directional CAM/RECAM DSS with
`SolutionTakePolicy = EARLY | GROUP`.  New topology, storage, Scratch,
reconstruction, remapping, policy, or synthesis exploration requires explicit
user authorization and a documented architectural delta before RTL changes.
Current authorized work is paper integration, result/claim auditing, figure or
table generation from accepted artifacts, documentation consolidation, and
regression revalidation.

STATUS:

PHASE_3F_3G_FUNCTIONAL_VERIFICATION_COMPLETE
PHASE_3H_3I_FUNCTIONAL_VERIFICATION_COMPLETE
PHASE_3J_SYNTHESIS_COMPLETE

COMPLETED:

- [x] Phase 0
- [x] Phase 1
- [x] Phase 2
- [x] Phase 3A (FUNCTIONALLY_CORRECTED_AND_SYNTHESIS_REBUILT_FOR_FROZEN_27_BIT_ADDRESS_GEOMETRY)
- [x] Phase 3B (COMPLETE_WITH_FROZEN_27_BIT_ADDRESS_GEOMETRY)
- [x] Phase 3C (FUNCTIONAL_VERIFICATION_COMPLETE)
- [x] Phase 3D (EARLY_FUNCTIONAL_VERIFICATION_COMPLETE; 20_NS_SYNTHESIS_CLOSED)
- [x] Phase 3E (GROUP_COMPRESSED_FUNCTIONAL_VERIFICATION_COMPLETE; 20_NS_SYNTHESIS_CLOSED)
- [x] Phase 3F (READ_ONLY_PHYSICAL_RESOURCE_LEDGER_FUNCTIONAL_VERIFICATION_COMPLETE)
- [x] Phase 3G (RESOURCE_AWARE_FALLBACK_FUNCTIONAL_VERIFICATION_COMPLETE)
- [x] Phase 3H (REGISTERED_LEDGER_FUNCTIONAL_VERIFICATION_COMPLETE)
- [x] Phase 3I (GROUP_ALLOCATOR_FUNCTIONAL_VERIFICATION_COMPLETE)
- [x] Phase 3J (FULL_DIRECTIONAL_DSS_20_NS_SYNTHESIS_COMPLETE)

LEGACY_CURRENT_TARGET:

Phase 3J Full Directional DSS 20 ns synthesis closure is complete. This was
the pre-V2 Phase 3 target snapshot; its "No Phase 4 work is authorized"
statement is superseded by the DSS V2 Phase 4 status below.

PHASE_3J:

- [x] Full EARLY and GROUP tops include the shared analyzer/scheduler,
  ConfigPatternMaps, Phase 3G fallback, registered ledger, commit/controller,
  and A->B->C->D allocator. Both meet the constrained 20 ns target with
  +0.01 ns path slack.
- [x] Accepted RECAM baseline / Full EARLY / Full GROUP comparison and
  normalization are recorded in [PHASE3J_SYNTHESIS.md](PHASE3J_SYNTHESIS.md).

PHASE_3H_3I:

- [x] A->B->C->D registered allocation, atomic release/borrow commit,
  first-failure/no-rollback behavior, and final ledger observability pass the
  15-case directed matrix and independent RTL-vs-golden regression.
- [x] The Phase 3G B/C GROUP order implementation defect was corrected to the
  already-frozen `2 -> 0 -> 3 -> 1` ranking and revalidated; it is not a new
  policy or semantic change. See [PHASE3HI_VERIFICATION.md](PHASE3HI_VERIFICATION.md).

PHASE_3F_3G:

- [x] Phase 3F defines a read-only 12-bit logical ledger snapshot and legal
  resource-action decoder; it adds no registered storage or update behavior.
- [x] Phase 3G selects the first locally repairable and physically feasible
  record using EARLY or GROUP ranking, with deterministic donor tie-break and
  no additional analyzer pass. No commit, reservation, rollback, or group
  scheduler was introduced. See [PHASE3F_PHYSICAL_RESOURCE_LEDGER.md](PHASE3F_PHYSICAL_RESOURCE_LEDGER.md)
  and [PHASE3G_RESOURCE_FALLBACK_ARCHITECTURE.md](PHASE3G_RESOURCE_FALLBACK_ARCHITECTURE.md).

PHASE_3E_GROUP_COMPRESSED:

- [x] Pure combinational local selector implements `2*Borrow + (1-Release)`.
  A/D priority is CFG4 → CFG0 → CFG6 → CFG5 (map 1 → 0 → 3 → 2); B/C priority
  is CFG1 → CFG0 → CFG3 → CFG2 (map 2 → 0 → 3 → 1). Valid, solution-valid, and
  repairable qualification plus map-order tie-breaking are preserved.
- [x] A1–A5, B1–B5, EARLY-vs-GROUP selection differences, local solution-valid
  equivalence, PatternID/bitmap/score alignment, and deterministic defaults
  pass. No prior-phase RTL behavior, resource ownership, or synthesis was
  added. See [PHASE3E_GROUP_ARCHITECTURE.md](PHASE3E_GROUP_ARCHITECTURE.md)
  and [PHASE3E_GROUP_FUNCTIONAL_VERIFICATION.md](PHASE3E_GROUP_FUNCTIONAL_VERIFICATION.md).
- [x] Authorized 20 ns GROUP selector-only and Phase 3C+GROUP integrated DC
  synthesis closes. Selector-only is 1,127.649610 area / 113.00 GE / 83
  combinational and zero sequential cells; integrated is 73,693.066257 area /
  7,384.67 GE / 85 sequential cells, WNS/TNS 0.00/0.00 ns, and exactly one
  shared analyzer. See [PHASE3E_GROUP_SYNTHESIS_20NS.md](PHASE3E_GROUP_SYNTHESIS_20NS.md).

PHASE_3D_EARLY:

- [x] Combinational EARLY selects the first `valid && solution_valid &&
  repairable` four-entry map record in stored role-aware scan order, preserving
  ConfigID, PatternID, and full candidate bitmap.
- [x] Standalone priority/default/alignment tests and scheduler-to-EARLY
  integration pass; no scan early termination or resource/ownership logic was
  added. See [PHASE3D_EARLY_ARCHITECTURE.md](PHASE3D_EARLY_ARCHITECTURE.md)
  and [PHASE3D_EARLY_FUNCTIONAL_VERIFICATION.md](PHASE3D_EARLY_FUNCTIONAL_VERIFICATION.md).
- [x] Authorized 20 ns selector-only and integrated DC synthesis closes.
  Selector-only is 967.982408 area / 97.00 GE / 71 combinational and zero
  sequential cells; integrated is 73,533.399055 area / 7,368.67 GE / 85
  sequential cells, WNS/TNS 0.00/0.00 ns, and exactly one shared analyzer.
  See [PHASE3D_EARLY_SYNTHESIS_20NS.md](PHASE3D_EARLY_SYNTHESIS_20NS.md).

PHASE_3A_EVIDENCE:

- [x] `recam_2r2c_analyzer` implements the fixed `RS=2`, `CS=2`, `K=4`
  combinational path: logical CAM state → Matrix Builder → six parallel
  candidates → Pattern Encoder → PatternID.
- [x] Verilator functional test passes empty-state, pivot-diagonal, RowMust,
  lowest-PatternID, and CAM-overflow cases.
- [x] The candidate uncovered predicate is RECAM-corrected to
  `pattern[row] && !pattern[col]`; the one-pivot diagonal case now leaves all
  six candidates valid.
- [x] The corrected 27-bit RTL timing-driven TSMC018 `slow.db` synthesis closes
  at 20 ns: 50,877.288438 cell area, 2,413 combinational leaf cells, 0
  sequential cells, 5,098.33 NAND2X1-area GE, WNS 0.00 ns, and TNS 0.00 ns.
- [x] Each synthesized module reports zero sequential cells.
- [x] The corrected structural 20 ns mapping is retained as a diagnostic
  baseline (28,071.490005 area, 71.92 ns path, WNS -51.92 ns); it is not a
  timing-driven closure claim.
- [x] Detailed corrected Phase 3A hierarchy, Phase 3B comparison, actual
  `slow/NAND2X1` reference area, and target-specific interpretation are in
  [PHASE3_SYNTHESIS_27BIT.md](PHASE3_SYNTHESIS_27BIT.md).

PHASE_3A_UNRESOLVED_ASSUMPTIONS:

- Phase 3A starts from post-collection logical Address/Hybrid CAM state.  CAM
  storage and fault collection hardware are intentionally outside this
  analyzer-area boundary.
- The fixed RTL interface uses `ROW_ADDR_W=9`, `COL_ADDR_W=5`, and seven
  logical Hybrid entries. `DIFF_ADDR_W=9` represents rows directly and
  zero-extended ColumnWords. The frozen 27-bit system address maps RECAM
  Channel to `Domain[2:0]+Bank[1:0]+Group[5:0]+SA[1:0]` (`CHANNEL_W=13`).
- The pre-correction Phase 3A synthesis reports are historical only.  The
  corrected 27-bit baseline is recorded in
  [PHASE3_SYNTHESIS_27BIT.md](PHASE3_SYNTHESIS_27BIT.md).

PHASE_3A_1_MATRIX_BUILDER_AUDIT:

- [x] Analysis-only Matrix Builder audit completed in
  [PHASE3A_MATRIX_BUILDER_AUDIT.md](PHASE3A_MATRIX_BUILDER_AUDIT.md).
- [x] The 19,758.816 / 38.17 ns result is attributed to the unrolled,
  state-dependent four-Hybrid dictionary reconstruction and variable-index
  matrix update network; no RTL behavior, timing architecture, or Phase 3B
  scope was changed.

PHASE_3B_0_ANALYZER_INTERFACE_FREEZE:

- [x] The pre-implementation analyzer boundary is frozen in
  [PHASE3B_ANALYZER_INTERFACE.md](PHASE3B_ANALYZER_INTERFACE.md); it starts at
  stored Address/Hybrid CAM logical state and explicitly excludes collector
  generation/storage from analyzer-only area.
- [x] All future retained synthesis reports require mapped leaf-cell and
  NAND2X1-equivalent gate-count reporting in addition to area.

PHASE_3B_IMPLEMENTATION_SCOPE:

- [x] One fully combinational shared analyzer accepts one ConfigID at a time;
  scheduling and ConfigPatternMap storage remain Phase 3C work.
- [x] The Phase 3B interface is frozen at `MAX_K=5`, `PIVOT_PTR_W=3`,
  `MAX_ADDRESS_ENTRIES=5`, and `MAX_HYBRID_ENTRIES=7`. Physical reusable CAM
  raw storage is 140 Address CAM bits plus 140 Hybrid CAM bits (280 bits
  total), excluding CAM circuitry and analyzer logic.
- [x] No EARLY, GROUP, PhysicalResourceLedger, pending repair, pipeline, FSM,
  or Pattern-serial logic is authorized.

PHASE_3B_FUNCTIONAL_VERIFICATION:

- [x] Verilator lint has zero WIDTH warnings without warning suppression.
- [x] Phase 3A and Phase 3B CFG0 each match the independent RECAM golden for
  2,008 directed/random vectors; Phase 3A versus Phase 3B CFG0 has zero
  mismatches.
- [x] CFG0–CFG6 directed, threshold-decode, MAX_K=5/pointer-4,
  transpose-pair, conventional-overflow, invalid ConfigID, and sparse-prefix
  negative tests pass.
- [x] The 27-bit geometry regression passes 9-bit row high-bit, 5-bit
  ColumnWord high-bit, deterministic 5→9 zero-extension, and functional
  Hybrid entry[6] checks.
- [x] Full-dictionary, unseen-Hybrid cases use RECAM line expansion: a known
  row expands across valid logical columns and a known column expands across
  valid logical rows.  These valid cases no longer assert
  `dictionary_overflow_o` or globally invalidate candidates.
- [x] `PHASE 3B: FUNCTIONAL_VERIFICATION_COMPLETE` is asserted. Evidence is
  recorded in [PHASE3B_FUNCTIONAL_VERIFICATION.md](PHASE3B_FUNCTIONAL_VERIFICATION.md).

PHASE_3B_SYNTHESIS_VERIFICATION:

- [x] The Phase 3B shared analyzer timing-driven TSMC018 `slow.db` synthesis
  closes at the authorized primary 20 ns target: 67,229.870944 cell area,
  3,514 combinational leaf cells, 0 sequential cells, 6,737.00 NAND2X1-area
  GE, 19.98 ns critical path, WNS +0.02 ns, and TNS 0.00 ns.
- [x] The 15 ns timing-driven attempts had no terminal tool result in the
  available environment.  Retained 15 ns structural reports are explicitly
  diagnostic only, not a timing-closure claim.
- [x] The synthesis evidence was sufficient for the authorized Phase 3B
  analyzer scope.  It did not independently authorize Phase 3C or timing-fix
  architectural changes.  Full method, reports, hierarchy, and comparison are recorded in
  [PHASE3_SYNTHESIS_27BIT.md](PHASE3_SYNTHESIS_27BIT.md).

PHASE_3C_FUNCTIONAL_VERIFICATION:

- [x] `recam_role_aware_config_scheduler` wraps exactly one frozen
  combinational Phase 3B analyzer.  `sa_role_i=0` scans A/D as CFG0, CFG4,
  CFG5, CFG6; `sa_role_i=1` scans B/C as CFG0, CFG2, CFG1, CFG3.
- [x] Four analyzer evaluation intervals capture four 20-bit result entries
  (80 bits total), preserving valid, ConfigID, solution_valid, repairable,
  PatternID, and the full ten-bit candidate bitmap.
- [x] Reset/start/busy/done behavior, sequence, result alignment, candidate
  preservation, mixed repairability, restart, and busy-start-negative tests
  pass.  The required Phase 3B and C++ paper-faithful integration regressions
  also pass.  See [PHASE3C_ARCHITECTURE.md](PHASE3C_ARCHITECTURE.md) and
  [PHASE3C_FUNCTIONAL_VERIFICATION.md](PHASE3C_FUNCTIONAL_VERIFICATION.md).
- [x] Authorized 20 ns synthesis passes for control-only and integrated Phase
  3C views.  The integrated top has 85 sequential cells / 4,527.230457 area;
  Phase 3B remains independently zero-sequential.  The frozen stable collector
  state/no-snapshot contract and full results are in
  [PHASE3C_SYNTHESIS_20NS.md](PHASE3C_SYNTHESIS_20NS.md).

PHASE_3A_3_ARTIFACT_INDEX:

- [x] Reproducible functional, structural, and timing-driven synthesis commands
  plus terminal-result interpretation are recorded in
  [PHASE3A3_TIMING_CLOSURE_STUDY.md](PHASE3A3_TIMING_CLOSURE_STUDY.md).
- [x] Phase 3A.3 is COMPLETE: timing closure is verified at every tested point
  from 40 ns through 10 ns.  The 10 ns point is the minimum tested closed
  period, not a demonstrated absolute minimum; it remains an aggressive
  area/timing reference, while Phase 3B uses 20 ns primary and 15 ns secondary.

PHASE_2_MULTI_CONFIG_PROGRESS:

- [x] H1 ConfigID / PatternID package definitions
- [x] H2 shared five-entry Pivot payload with per-ConfigID activity metadata
- [x] H3 shared physical-address counter table and Must threshold decoder
- [x] H4 tagged Hybrid storage primitive
- [x] H5 CAM-reuse temporary-buffer primitive
- [x] H6 canonical shared-view matrix/pattern analyzer refactor
- [x] H7 lowest PatternID encoder
- [x] H8 transpose and seven physical ConfigID results
- [x] H9 EARLY ConfigPatternMap selection
- [x] H10 GROUP speculative ledger search
- [x] H11 pending repair/decode integration (D-004 tagged Hybrid delta retained)
- [x] H12 simulator-to-RTL directed golden regression
- [x] H13 synthesis/report integration
- [x] H14 post-correctness optimization

EVIDENCE_AND_REMAINING_RISKS:

The prior PivotPayload-only final-decode mismatch is resolved by accepted
decision D-004: retain one tagged Hybrid/dictionary delta per subarray until
the EARLY/GROUP result commits.  The directed C++-generated vector set
(`single_group`, `single_early`, and `must_hybrid_group`) now passes through
the final decoder and PendingRepairBuffer.  Broader randomized coverage remains
a future regression-expansion task, not an H12 failure.

H8 exposes seven physical results, the transpose wrapper passes signal-level
swap tests, and directed equivalence tests validate the physical PatternID
mapping for CAN1/CFG4, CAN2/CFG5, and CAN3/CFG6. H14 now time-multiplexes one
canonical analyzer across seven ConfigIDs, one candidate evaluator across all
40 ConfigID/Pattern views, and one final decoder across four subarrays. Runtime
counters were narrowed to their required widths, and the
final decoder no longer rebuilds the candidate-validity matrix after a pattern
has already been selected.  Parallel and AREA variants pass the same directed
golden regression.

H13 uses DC W-2024.09-SP2 with TSMC018 `slow.db`. The complete AREA top and all
major blocks have terminal `STATUS=PASS` structural-mapping reports at a 20 ns
clock and zero input/output delay. Conservative 14-entry reports cover the
collector, canonical analyzer, EARLY, GROUP, PendingRepairBuffer, and final
decoder; the four Hybrid-dependent blocks also have experimental 7-entry
reports. The AREA top maps to 49,023 leaf cells (10,418 sequential), with cell
area 1,703,003.72. This is a mapping/report-integration result, not timing
closure: post-map STA reports WNS -169.97 ns and 2,291 nets with design-rule
violations. Timing-driven DC runs remained above the 600-second limit.

Phase 2 exit gates are complete: legacy and directional functional regressions
pass, C++-generated golden selections and final repair mappings match, cycle
behavior is checked, standard-cell mapping completes, and hierarchical/per-block
area reports are available. Timing closure remains an explicitly unresolved
technical risk and is not claimed by the Phase 2 synthesis gate.

LAST_VERIFIED_COMMIT:

3aab91ad2e8ac8bb62654c7b74be8d319c91e350

NEXT_PHASE:

4

NEXT_PHASE_AUTHORIZED:

NO

---

# DSS V2 — Phase 4 Status

## Phase 4E — Specialized EARLY

Status: COMPLETE

Key results:

- Random equivalence: 1,000 vectors, 0 mismatches.
- Cell area: 80,871.437539.
- GE: 8,104.00.
- Architectural state: 73 bits.
- WNS at 20 ns: +0.01 ns.
- Legacy EARLY area reduction: 31.01%.
- Full ConfigPatternMap removed from V2 EARLY.

Evidence: [PHASE4E_CHARACTERIZATION.md](PHASE4E_CHARACTERIZATION.md).

## Phase 4F — GROUP-NoScratch

Status: COMPLETE

Key results:

- Random equivalence: 1,000 vectors, 0 mismatches.
- Candidate history: 80 bits.
- Architectural state: 157 bits.
- Cell area: 105,393.658552.
- GE: 10,561.33.
- WNS at 20 ns: +0.00 ns.
- Legacy GROUP area reduction: 10.027%.

Evidence: [PHASE4F_CHARACTERIZATION.md](PHASE4F_CHARACTERIZATION.md).

## Phase 4G — GROUP-WithScratch Audit

Status: COMPLETE_NEGATIVE_RESULT

Key results:

- The minimum frozen-GROUP candidate history remains 80 bits.
- Scratch provides no state advantage at the decision/resource-management
  boundary.
- Recompute would require either a new input-stability contract or an
  approximately 204-bit raw-input snapshot.

```text
WITH_SCRATCH_ARCHITECTURALLY_JUSTIFIED = NO
```

This is architecture-space pruning, not an unperformed phase or an RTL
implementation failure. Evidence: the Phase 4G audit recorded in
[PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md](PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md).

## Phase 4H — Unified Hardware Characterization

Status: COMPLETE

Key comparison:

- EARLY area: 80,871.44.
- GROUP area: 105,393.66.
- GROUP overhead: +24,522.22 cell area.
- EARLY architectural state: 73 bits.
- GROUP architectural state: 157 bits.
- Deferred-selection state overhead: +84 bits.

Evidence: [PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md](PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md).

## Phase 4I — Post-BIST Analyze-Latency Study

Status: COMPLETE

Corpus:

- 10,000 paired traces.
- Seed: 20260910.
- 33 gap points (`G=0..32`).
- 330,000 paired observations.

`G_zero`:

- EARLY: P50 = 4, P95 = 4.
- GROUP-NoScratch: P50 = 20, P95 = 21.

Paired result:

- EARLY < GROUP: 61.562424%.
- EARLY = GROUP: 38.437576%.
- EARLY > GROUP: 0%.

RTL event-model validation: eight representative production-top cases, zero
cycle mismatches.

Evidence: [PHASE4I_FINAL_CHARACTERIZATION.md](PHASE4I_FINAL_CHARACTERIZATION.md).

```text
V2_PHASE = PHASE_4K_COMPLETE
V2_NEXT_PHASE_AUTHORIZED = NO
```

## Deferred / Out-of-Scope Items

- Pivot/Hybrid final reconstruction.
- Repair-table generation.
- Runtime remapping.
- 1×4 topology.
- SRAM implementation.
- Global-edge policy.
- Future reconstruction-aware Scratch analysis.

## Phase 4J — SolutionTakePolicy Repairability Tradeoff

Status: COMPLETE

Key results:

- 90,000 paired repair-group instances; seed 20260910.
- Primary moderate-imbalance sweep: 8–32 faults/group, step 4; 10,000 cases
  per severity point.
- Primary sweep repairability: EARLY 78.647143%, GROUP 84.761429%, a
  +6.114286 percentage-point GROUP gain.
- Full measured corpus: `EARLY_ONLY=0`, `GROUP_ONLY=4691`; the measured-corpus
  classification is `GROUP_DOMINATES`, not a universal dominance claim.
- Paired ID, case-count, outcome-accounting, duplicate-ID, and missing-result
  integrity checks all pass.

Evidence: [PHASE4J_POLICY_TRADEOFF.md](PHASE4J_POLICY_TRADEOFF.md).

## Phase 4K — GROUP Dominance Mechanism Analysis

Status: COMPLETE

Key results:

- All 4,691 Phase 4J `GROUP_ONLY` cases were traced from the existing paired
  raw artifact; no new fault corpus was generated.
- First divergence is always at A: EARLY `CFG0` versus GROUP `CFG4`.
- GROUP's first divergent decision releases `A_ROW` in all 4,691 cases.
- EARLY failures are B/C/D = 929/1,398/2,364; no GROUP_ONLY case fails at A.
- No first-divergence case is unexplained; the `EARLY_ONLY=0` result remains
  classified `EMPIRICAL_ONLY`, not structurally proven.

Evidence: [PHASE4K_GROUP_DOMINANCE_MECHANISM.md](PHASE4K_GROUP_DOMINANCE_MECHANISM.md).

## Proposed Future Work — NOT AUTHORIZED

### Evidence and Paper Integration

Status: PROPOSED / NOT AUTHORIZED

- Final DATE manuscript integration.
- Final paper figure generation from accepted CSV artifacts.
- Submission-specific text/formatting review.

### Future Architecture Exploration

Status: PROPOSED / OUTSIDE CURRENT DATE SCOPE

- 1×4 Directional / Global-Edge policies.
- SRAM-based candidate storage.
- Reconstruction-aware Scratch architecture.

## Update rule

Phase completion and authorization follow [02_PHASE_CONTROL.md](02_PHASE_CONTROL.md).
`NEXT_PHASE_AUTHORIZED` must remain `NO` until the project owner explicitly authorizes the
next phase.
