# DSS Thesis Execution Phase Control

> 文件狀態：Current
> 適用範圍：DSS thesis execution phase authorization and entry gates
> 建立時間：2026-09-13T06:28:23+08:00
> 最後修改時間：2026-09-13T22:20:00+08:00

Only explicit project-owner authorization changes a phase from `NOT_STARTED` to
`AUTHORIZED`.  Controlled status vocabulary is:

```text
NOT_STARTED, PROPOSED, AUTHORIZED, IN_PROGRESS, BLOCKED, COMPLETE,
COMPLETE_NEGATIVE_RESULT, N/A_PROVEN_UNREACHABLE, FAILED, DEFERRED
```

## Dependency graph

```text
DSS_X0 -> H0 and S0 (parallel only after explicit authorization)
H0 -> H1 -> H2 -> H3 -> H4 -> H5
                              \-> S0R -> S1 -> E0 -> E1 -> D0 -> D1 -> D2 -> D3 -> P0
H5O -> H4R -> H5 (only if the authorized final original-RTL attempt reaches
                  its hard timeout)
E1 -> A0 -> (A1 only if justified) -> P0
```

`H3` additionally requires a completed reuse audit and explicit `ADVANCE TO
H3`.  `S1` requires verified actual new RTL geometry.  `E0` requires a frozen
simulator contract, calibrated geometry, clean functional regression, and
hardware-characterization provenance.  `A0` is nonblocking for `E1`; `A1`
exists only when `A0` finds implementation architecturally justified.  `P0`
freezes only claims supported by accepted evidence.

## Standard phase record

Every record below explicitly states Status, Research Question, Scope, Excluded
Scope, Frozen Inputs, Entry Conditions, Required Reuse Audit, Tasks, Required
Tests, Metrics, Required Artifacts, Completion Gate, Final State, Next Proposed
Phase, and `NEXT_PHASE_AUTHORIZED`.  No numeric result is implied by an
unstarted record.

### DSS_X0 — Execution governance / authority / freeze bootstrap

```text
Status: COMPLETE
Research Question: Can the new execution be governed without changing frozen evidence?
Scope: documentation/control bootstrap
Excluded Scope: RTL, simulator, synthesis, experiments
Frozen Inputs: DATE_2026_DSS_V2 Phase 4K
Entry Conditions: Phase-4K and current authorities exist
Required Reuse Audit: N/A; establishes the audit rule
Tasks: create isolated control layer and conflict register
Required Tests: documentation/link/diff validation
Metrics: none
Required Artifacts: docs/dss_execution control set
Completion Gate: all DSS_X0 gates accepted
Final State: DSS_X0 complete; next phases remain unauthorized
Next Proposed Phase: H0,S0
NEXT_PHASE_AUTHORIZED: NONE
```

### H0 — Pre-Implementation Reuse and Parameterization Audit

```text
Status: NOT_STARTED
Research Question: Which existing RTL/verification/topology/ledger/analyzer/synthesis resources safely support 3,3,1, and which assumptions require change?
Scope: read-only reuse and parameter audit of rtl/dss_v2, rtl/recam, rtl/dss_2x2/analyzer, tb, tests, tests/golden, scripts, and authority/evidence
Excluded Scope: RTL modification and implementation
Frozen Inputs: 2,2,1 V2 evidence; DSS_X0 audit rule
Entry Conditions: explicit H0 authorization; frozen baseline and authorities available
Required Reuse Audit: this phase is the mandatory audit
Tasks: classify every relevant block as PARAMETERIZED_AND_VERIFIED, PARAMETERIZED_BUT_UNVERIFIED_FOR_TARGET, PARTIALLY_PARAMETERIZED, FIXED_TO_2_2_1, FIXED_CONFIG_TABLE, REUSABLE_REFERENCE_ONLY, REQUIRES_EXTENSION, or REQUIRES_REDESIGN
Required Tests: source/interface and regression-availability inspection only
Metrics: reuse coverage, fixed-assumption inventory, regression risk
Required Artifacts: completed 02 audit instance and evidence pointers
Completion Gate: all mandatory audit questions answered with one fixed recommendation
Final State: RTL_IMPLEMENTATION_AUTHORIZED remains NO until later phase gate
Next Proposed Phase: H1
NEXT_PHASE_AUTHORIZED: NONE
```

### H1 — 3,3,1 legal configuration-space freeze

```text
Status: NOT_STARTED
Research Question: What legal role-specific physical configuration envelope follows from 2x2 Directional RS=CS=3 SHARE_M=1?
Scope: derive legal configurations, R_max/C_max/K_max, C(K,R), maximum candidates, PatternID width, transpose opportunities, and ConfigID strategy
Excluded Scope: copying old seven IDs manually; RTL implementation
Frozen Inputs: H0 evidence; V2 policy semantics
Entry Conditions: H0 COMPLETE and explicit H1 authorization
Required Reuse Audit: H0 result must be cited
Tasks: derive rather than assume target configuration space
Required Tests: derivation consistency and reference-model review
Metrics: legal envelope and candidate cardinalities
Required Artifacts: signed configuration-space specification
Completion Gate: all enumerated H1 outputs are frozen
Final State: target configuration representation defined, not implemented
Next Proposed Phase: H2
NEXT_PHASE_AUTHORIZED: NONE
```

### H2 — Analyzer/CAM/ledger sizing architecture

```text
Status: NOT_STARTED
Research Question: What physical analyzer, CAM, candidate, and ledger representation is required by the H1 envelope?
Scope: Address/Hybrid entries and widths, matrix, candidate vector, PatternID/descriptor, scan state, resource action, ledger, donor/borrow encoding
Excluded Scope: treating frozen MAX_K=5/MAX_ADDRESS_ENTRIES=5/MAX_HYBRID_ENTRIES=7 as target answers; RTL implementation
Frozen Inputs: H0 and H1 evidence
Entry Conditions: H0 COMPLETE, H1 COMPLETE, explicit H2 authorization
Required Reuse Audit: H0 result and H1 ConfigID strategy
Tasks: distinguish general RECAM geometry, DSS physical envelope, and RTL storage representation
Required Tests: sizing/width and transpose consistency review
Metrics: required capacities and state widths
Required Artifacts: sizing architecture and interface delta record
Completion Gate: all required physical representations are explicit
Final State: implementation inputs frozen, not implemented
Next Proposed Phase: H3
NEXT_PHASE_AUTHORIZED: NONE
```

### H3 — 3,3,1 V2-derived RTL

```text
Status: NOT_STARTED
Research Question: Can V2-derived EARLY and GROUP-NoScratch implement the frozen 3,3,1 envelope?
Scope: 2x2 Directional CAM RS=CS=3 SHARE_M=1 RTL for EARLY and GROUP-NoScratch
Excluded Scope: legacy Phase-3 ConfigPatternMap as primary architecture; Scratch; policy reinvention
Frozen Inputs: H0/H1/H2 accepted outputs and V2 policy semantics
Entry Conditions: H0/H1/H2 COMPLETE, reuse audit complete, explicit ADVANCE TO H3
Required Reuse Audit: completed H0 recommendation and regression plan
Tasks: minimal architecture delta with baseline protection
Required Tests: compile/lint and baseline-preservation tests
Metrics: interface/state delta only
Required Artifacts: RTL, manifests, and validation records if authorized
Completion Gate: implementation and baseline protection criteria accepted
Final State: functional candidate ready for H4
Next Proposed Phase: H4
NEXT_PHASE_AUTHORIZED: NONE
```

### H4 — Functional verification

```text
Status: NOT_STARTED
Research Question: Does new RTL preserve frozen policy semantics at the new geometry?
Scope: directed, randomized, golden, ledger, first-failure/no-rollback verification
Excluded Scope: synthesis and experiment sweep
Frozen Inputs: H3 RTL and H1/H2 semantics
Entry Conditions: H3 COMPLETE and explicit H4 authorization
Required Reuse Audit: reusable verification identified by H0
Tasks: execute required functional comparison and baseline regressions
Required Tests: directed/random per-policy golden equivalence (EARLY RTL versus
EARLY golden; GROUP-NoScratch RTL versus GROUP golden), four-class paired
outcome accounting (BOTH_PASS, EARLY_ONLY, GROUP_ONLY, BOTH_FAIL), universal
resource/ledger invariants, and frozen baseline regression. EARLY_ONLY is not
itself a functional failure. EARLY success => GROUP success is a historical
stale requirement, not a new-execution invariant.
Metrics: mismatch and invariant counts
Required Artifacts: reproducible logs and evidence
Completion Gate: clean functional correctness and baseline preservation
Final State: functionally verified RTL
Next Proposed Phase: H5
NEXT_PHASE_AUTHORIZED: NONE
```

### H5 — Hardware characterization

```text
Status: COMPLETE — original H4-verified RTL mapped; target 20-ns timing is not closed
Research Question: What equivalent-boundary hardware cost and timing characterize verified 3,3,1 RTL?
Scope: accepted V2-equivalent synthesis characterization
Excluded Scope: old Phase-2 structural reports as comparison baseline
Frozen Inputs: H4 functional result and accepted Phase-4 provenance
Entry Conditions: H4 COMPLETE and explicit H5 authorization
Required Reuse Audit: H0 synthesis-flow reuse and H5 provenance audit
Tasks: audit reproduction command, library/corner/constraint, metadata, and report path
Required Tests: synthesis checks plus report/provenance validation
Metrics: area, GE, timing, sequential state at equivalent boundary
Required Artifacts: results provenance and characterization report
Completion Gate: accepted equivalent-boundary hardware evidence
Final State: hardware-calibrated new geometry
Next Proposed Phase: S1
NEXT_PHASE_AUTHORIZED: NONE
```

### H5O — Synthesis-representation recovery (conditional)

```text
Status: NOT_NEEDED
Research Question: Can the fixed 3,3,1 analyzer be represented in a
semantics-preserving form that completes accepted-methodology synthesis?
Entry Condition: the one authorized 14400-second original-H4-RTL run receives
the runner SIGTERM while legitimately mapping/optimizing
Scope: mandatory reuse audit, target-only constant/table/fixed-width rewrite,
and H4R equivalence before any optimized synthesis
Excluded Scope: policy, candidate order, PatternID, matrix/Must/transpose,
latency, Scratch, and 2,2,1 RTL changes
NEXT_PHASE_AUTHORIZED: NONE
```

### S0 — Simulator schema / experiment contract

```text
Status: NOT_STARTED
Research Question: What frozen group-level simulator contract is required for a later paired DSS study?
Scope: group schema, provenance, paired corpus, output and comparison contract
Excluded Scope: simulator algorithm modification, RTL execution, registered workflow
Frozen Inputs: docs authority and DSS_X0 scope boundaries
Entry Conditions: explicit S0 authorization; no dependence on H0 for contract definition
Required Reuse Audit: existing DynamicSpareSharing/report/workflow inspection
Tasks: define contract without creating a new executable workflow
Required Tests: schema and scope review
Metrics: required metadata and paired-observation completeness
Required Artifacts: accepted contract proposal
Completion Gate: simulator contract frozen without scope mixing
Final State: ready to calibrate after actual RTL geometry exists
Next Proposed Phase: S1
NEXT_PHASE_AUTHORIZED: NONE
```

### S0R — Simulator policy semantic resolution

```text
Status: PROPOSED
Authorization: NO
Research Question: Can the DynamicSpareSharing policy path be calibrated to
the frozen V2 GROUP-NoScratch greedy semantics without relabeling enumerative
GroupCompressed behavior?
Scope: audit/calibration plan only
Excluded Scope: experiments, device integration, or semantic substitution
Blocking Conflict: H0S0-CL-004
```

### S1 — RTL-calibrated simulator hardware metadata

```text
Status: NOT_STARTED
Research Question: Can group simulation metadata cite verified actual RTL geometry and hardware provenance?
Scope: calibrated metadata and contract integration
Excluded Scope: substituting device repair rates or analytical proxy values for RTL data
Frozen Inputs: S0 contract and H5 accepted evidence
Entry Conditions: S0 COMPLETE, H5 COMPLETE, verified actual new RTL geometry, explicit S1 authorization
Required Reuse Audit: report/parser/metadata path from H0 and S0
Tasks: bind provenance without altering numeric meaning
Required Tests: metadata completeness and scope validation
Metrics: provenance-field completeness
Required Artifacts: calibrated schema/evidence record
Completion Gate: every imported RTL datum carries required provenance
Final State: simulation is hardware-calibrated
Next Proposed Phase: E0
NEXT_PHASE_AUTHORIZED: NONE
```

### E0 — Paired experiment preflight

```text
Status: NOT_STARTED
Research Question: Is the paired group experiment reproducible and scope-correct before full execution?
Scope: preflight corpus, policy contract, metadata, and workflow readiness
Excluded Scope: full study claims and Scratch decision
Frozen Inputs: S0 contract, S1 calibration, H4 functional and H5 hardware evidence
Entry Conditions: frozen simulator contract, calibrated geometry, clean functional regression, hardware provenance, explicit E0 authorization
Required Reuse Audit: workflow/manifest/report reuse results
Tasks: establish preflight evidence
Required Tests: deterministic paired preflight
Metrics: corpus pairing and contract compliance
Required Artifacts: preflight manifest and results
Completion Gate: reproducible scope-correct preflight
Final State: full experiment authorized only by later instruction
Next Proposed Phase: E1
NEXT_PHASE_AUTHORIZED: NONE
```

### E1 — Full DSS experiment / mechanism analysis

```text
Status: NOT_STARTED
Research Question: What repairability, imbalance, and greedy-loss tradeoffs occur under the frozen paired group contract?
Scope: group-level EARLY/GROUP-NoScratch experiment and mechanism analysis
Excluded Scope: group/device aggregation and unsupported causal claims
Frozen Inputs: E0 preflight, S1 metadata, accepted policy semantics
Entry Conditions: E0 COMPLETE and explicit E1 authorization
Required Reuse Audit: approved workflow and analysis reuse
Tasks: execute only registered contract/workflow
Required Tests: reproducibility and invariant checks
Metrics: paired outcome classes and approved study metrics
Required Artifacts: run artifacts, source CSV, analysis evidence
Completion Gate: accepted reproducible experiment evidence
Final State: primary 3,3,1 experiment complete
Next Proposed Phase: A0
NEXT_PHASE_AUTHORIZED: NONE
```

### D0 — Device Integration Audit

```text
Status: PROPOSED
Authorization: NO
Research Question: Can frozen V2 DSS group semantics enter the existing
HierarchicalRECAM device path without changing group policy, spare ownership,
or global-CAM ownership?
Scope: audit only
Excluded Scope: simulator implementation or device experiments
Required Audit: group invocation, current selector semantics, Tier-1 reset
state, persistent Tier-2 state, global-pool ownership, and existing occupancy/
overflow tests
Completion Gate: a scope-safe integration plan; implementation remains
unauthorized
```

### D1 — DSS Impact on Tier-2 CAM Demand

```text
Status: PROPOSED
Authorization: NO
Research Question: Does Tier-1 DSS reduce residual Tier-2 demand and persistent
device-wide CAM occupancy on identical device fault corpora?
Scope: future paired No-Sharing/EARLY/GROUP-NoScratch device study
Excluded Scope: group-rate/device-rate aggregation
Required Metrics: Tier-2 requests, novel/deduplicated entries, final/peak
occupancy, overflow, and device success
```

### D2 — Finite Global CAM Capacity Sweep

```text
Status: PROPOSED
Authorization: NO
Research Question: How does Tier-1 DSS benefit vary with one finite persistent
device-wide online CAM capacity?
Scope: future paired capacity sweep
Excluded Scope: per-group or reset CAM interpretations
```

### D3 — Device-Level Mechanism Analysis

```text
Status: PROPOSED
Authorization: NO
Research Question: Which Tier-1 actions avoid or reduce persistent Tier-2 CAM
allocations, and where does device occupancy diverge?
Scope: future paired traces and mechanism classes
Excluded Scope: unsupported repairability claims
```

The intended progression is first a `2,2,1` policy/device mechanism study and
then a `3,3,1` device scalability confirmation.  D0–D3 remain unauthorized.
`H0S0-CL-004` remains open, so S1/E0/E1 and any device integration depending
on frozen group-policy calibration remain blocked.

### A0 — Scratch Applicability Audit

```text
Status: NOT_STARTED
Research Question: Does Scratch provide a meaningful state-lifetime or retained-information advantage for EARLY-WithScratch or GROUP-WithScratch?
Scope: architectural applicability audit after primary line evidence
Excluded Scope: Scratch RTL implementation by default
Frozen Inputs: E1 evidence and primary architecture state analysis
Entry Conditions: E1 COMPLETE and explicit A0 authorization
Required Reuse Audit: state lifetime, storage, input-stability, and reconstruction boundary audit
Tasks: determine IMPLEMENTATION_JUSTIFIED or COMPLETE_NEGATIVE_RESULT
Required Tests: analytical/state evidence review; no required RTL
Metrics: state/lifetime benefit versus added contract/complexity
Required Artifacts: A0 audit and recommendation
Completion Gate: explicit justified or negative-result conclusion
Final State: A1 proposed only if implementation is justified
Next Proposed Phase: A1 or P0
NEXT_PHASE_AUTHORIZED: NONE
```

### A1 — Scratch RTL

```text
Status: NOT_STARTED
Research Question: If A0 justifies it, can Scratch RTL preserve frozen semantics with a measured benefit?
Scope: only the A0-justified Scratch architecture
Excluded Scope: speculative matrix completion
Frozen Inputs: A0 implementation-justified conclusion and primary-line evidence
Entry Conditions: A0 concludes IMPLEMENTATION_JUSTIFIED and explicit A1 authorization
Required Reuse Audit: A0 completed audit plus implementation-specific reuse audit
Tasks: minimal justified Scratch implementation and comparison
Required Tests: functional/golden/baseline and applicable hardware checks
Metrics: justified state/area/latency benefit
Required Artifacts: RTL/evidence only if authorized
Completion Gate: accepted semantics and benefit evidence
Final State: Scratch evidence accepted or failure documented
Next Proposed Phase: P0
NEXT_PHASE_AUTHORIZED: NONE
```

### P0 — Thesis evidence / claims freeze

```text
Status: NOT_STARTED
Research Question: Which claims are supported by accepted evidence only?
Scope: evidence consolidation, figures/tables, and claim boundary
Excluded Scope: new architecture, RTL, experiments, or reinterpretation
Frozen Inputs: accepted H/S/E/A evidence
Entry Conditions: relevant evidence complete and explicit P0 authorization
Required Reuse Audit: evidence/provenance traceability review
Tasks: freeze supported claims and cite sources
Required Tests: evidence-to-claim audit
Metrics: claim provenance completeness
Required Artifacts: thesis evidence and claims register
Completion Gate: no unsupported extrapolation
Final State: thesis evidence frozen
Next Proposed Phase: NONE
NEXT_PHASE_AUTHORIZED: NONE
```
