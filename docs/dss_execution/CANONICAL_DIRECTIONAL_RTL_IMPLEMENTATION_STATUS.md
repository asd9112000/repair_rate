# Canonical Directional RTL Implementation Status

> **Current closure update (2026-09-18):** group C++ and the canonical policy
> contract are frozen. Canonical Streaming EARLY and GLOBAL-NoScratch are
> functionally closed. Both authorized RS2 synthesis runs completed with DC
> W-2024.09-SP2 using the accepted TSMC018 slow / 20 ns / zero-I/O /
> low-map-effort methodology. GLOBAL-WithScratch was not started.
>
> ```text
> GROUP_CPP_SIMULATOR: CLOSED
> CANONICAL_POLICY_CONTRACT: FROZEN
> STREAMING_EARLY: CLOSED
> GLOBAL_NOSCRATCH: CLOSED
> DC_ACCESS: AVAILABLE
> RS2_EARLY_SYNTHESIS: COMPLETE
> RS2_EARLY_AREA: 83831.933613
> RS2_EARLY_GE: 8400.67
> RS2_EARLY_WNS: +0.01 ns
> RS2_GLOBAL_NOSCRATCH_SYNTHESIS: COMPLETE
> RS2_GLOBAL_NOSCRATCH_AREA: 61751.290432
> RS2_GLOBAL_NOSCRATCH_GE: 6188.00
> RS2_GLOBAL_NOSCRATCH_WNS: +0.02 ns
> GLOBAL_WITHSCRATCH_STARTED: NO
> FORMAL_100K_TOUCHED: NO
> FROZEN_GROUP_CORPUS_CHANGED: NO
> DEVICE_SWEEP_STARTED: NO
> ```
>
> Evidence directories:
> `results/dss_canonical/dc_tsmc018_slow/rs2_streaming_early_20ns` and
> `results/dss_canonical/dc_tsmc018_slow/rs2_global_noscratch_20ns`.

> **Superseded status (2026-09-18):** this file records the earlier stop
> condition and is retained for provenance. The later task froze `R,L,RB,B`
> plus future-donor release obligations; current GLOBAL-NoScratch status is in
> `CANONICAL_DIRECTIONAL_GLOBAL_RTL_IMPLEMENTATION_STATUS.md`. Current result:
> RS2 GLOBAL-NoScratch functional gate CLOSED (6 directed + 1,000 seeded,
> zero mismatches), 630 state bits, and RS2 synthesis BLOCKED by DC `DCSH-1`.

## Executive result

```text
STATUS: PARTIAL_COMPLETE_STOP_CONDITION

CANONICAL_ROOT: rtl/dss_canonical/

# EARLY
STREAMING_EARLY_IMPLEMENTED: YES
STREAMING_EARLY_IMPLEMENTATION_BOUNDARY: candidate/analyzer-result through committed physical ledger; RS2 analyzer-integrated top also present
STREAMING_EARLY_BASE: historical V2 streaming controller structure plus direct V2 table/topology/ledger reuse
STREAMING_EARLY_SEMANTIC_CHANGES: every SA uses RELEASE_ONLY, LOCAL, RELEASE_AND_BORROW, BORROW_ONLY
STREAMING_EARLY_RANDOM_VECTORS: RS2 1000; RS3 1000 (seed 0x20260918 + resource point)
STREAMING_EARLY_DIRECTED_VECTORS: RS2 6; RS3 6 (E1-E5 plus N2/F16/group172 reconstructed candidate-map witness)
STREAMING_EARLY_MISMATCHES: 0
STREAMING_EARLY_SCRATCH_SPLIT_REQUIRED: NO

# GLOBAL
GLOBAL_NOSCRATCH_IMPLEMENTED: NO
GLOBAL_WITHSCRATCH_IMPLEMENTED: NO
GLOBAL_SEARCH_IS_TRUE_BACKTRACKING: NO RTL EMITTED
GLOBAL_RANDOM_VECTORS: 0
GLOBAL_SOFTWARE_MISMATCHES: NOT_RUN; pre-implementation contract mismatch proven
GLOBAL_SCRATCH_NOSCRATCH_MISMATCHES: NOT_RUN

# Storage
GLOBAL_REQUIRED_RETAINED_BITS: NOT_FROZEN; candidate-history lower bound RS2=160 bits, RS3=560 bits, excluding DFS/speculative-ledger state
GLOBAL_NOSCRATCH_LOCAL_BITS: NOT_FROZEN
GLOBAL_WITHSCRATCH_LOCAL_BITS: NOT_FROZEN
GLOBAL_REUSED_SCRATCH_BITS: 0 PROVEN

# Reuse
RESOURCE_LEDGER_REUSED: YES (directly by canonical EARLY)
TOPOLOGY_REUSED: YES
ANALYZER_REUSED: RS2 DIRECT; RS3 candidate-map boundary only because checked-in analyzer uses forbidden runtime nth_pattern
RS2_RS3_SPECIALIZATION: common policy core; distinct frozen config/topology modules; analyzer remains resource-point specialized

# Synthesis
RS2_SYNTHESIS_COMPLETE: NO
RS3_SYNTHESIS_COMPLETE: NO
STREAMING_EARLY_AREA: NOT_RUN
GLOBAL_NOSCRATCH_AREA: NOT_IMPLEMENTED
GLOBAL_WITHSCRATCH_AREA: NOT_IMPLEMENTED
GLOBAL_SCRATCH_AREA_REDUCTION: NOT_APPLICABLE
BEST_20NS_WNS: NOT_MEASURED for canonical RTL; best retained correct-policy diagnostic is historical -12.52 ns

# Optimization evidence
HARDWARE_OPTIMIZATION_LEDGER_UPDATED: YES

# Safety
HISTORICAL_RTL_MOVED: NO
FORMAL_GROUP_DATA_TOUCHED: NO
DEVICE_SWEEP_STARTED: NO
```

## Implemented files

- `rtl/dss_canonical/policy/early/recam_dss_canonical_streaming_early_core.v`
  is a Verilog-2001 policy/resource core.  It uses a common six-bit PatternID
  trace and elaboration-time RS2/RS3 binding.
- `rtl/dss_canonical/top/recam_dss_canonical_rs2_streaming_early_top.v`
  binds the unchanged RS2 shared analyzer to the canonical controller.
- Each RTL module has a same-name `_spec.md` companion with a timing diagram.
- `tb/dss_canonical/recam_dss_canonical_streaming_early_core_test.cpp`
  contains an independent frozen-contract golden model and compares every
  selected field plus the physical ledger after each accepted SA.
- `make test_canonical_directional_early` is the repeatable RS2/RS3 gate.

The test boundary is completed candidate maps.  It does not claim raw-fault
collector or final repair-address reconstruction equivalence.  The W1 map is
reconstructed from the retained N2/F16/group172 trace; the formal normalized
dataset itself was not read-modified-written.

## Directed coverage

| Case | Required behavior | Result |
|---|---|---|
| E1 | RELEASE_ONLY and LOCAL legal; choose RELEASE_ONLY | PASS RS2/RS3 |
| E2 | RELEASE_ONLY illegal, LOCAL legal | PASS RS2/RS3 |
| E3 | RB and B locally valid with a donor; choose RB | PASS RS2/RS3 |
| E4 | later failure preserves committed prefix; no rollback/backtracking | PASS RS2/RS3 |
| E5 | multiple donors; frozen primary donor wins | PASS RS2/RS3 |
| W1 | N2/F16/group172 EARLY prefix: A local, B release, C fails | PASS at the reconstructed candidate-map boundary |

## Verification evidence

```text
make test_canonical_directional_early
  RS2: DIRECTED_VECTORS=6 RANDOM_VECTORS=1000 MISMATCHES=0
  RS3: DIRECTED_VECTORS=6 RANDOM_VECTORS=1000 MISMATCHES=0

verilator --lint-only ... recam_dss_canonical_rs2_streaming_early_top
  PASS, exit 0, no warnings

scripts/simulation/run_recam_phase3b_functional.sh
  PHASE3B_FUNCTIONAL_REGRESSION PASS

bash scripts/simulation/run_recam_phase4e3_random.sh
  historical EARLY 50 + 1000 vectors, all mismatch categories 0
  shared config/feasibility/ledger tests PASS
  Phase 3H/I directed regression PASS
```

The historical Phase 4E EARLY policy is not relabeled canonical; that
regression protects the directly reused modules only.

## GLOBAL stop conditions

Gate C is stopped before RTL creation for two independent reasons.

1. The task freezes GLOBAL action traversal as `R,L,RB,B`.  The current
   production `findDirectionalV2GroupGlobalChoice()` receives candidates from
   `compressedPlansForSubarray()` in attempt/slot order `L,R,B,RB`, then
   PatternID.  Exact first-tuple identity therefore differs.
2. The software `allocateSequential()` may give an earlier SA an unused
   shareable line owned by a future SA without a prior explicit release.  RTL
   feasibility requires the donor's released bit.  The retained
   N2/F16/group172 GLOBAL witness actually selects A BORROW_ONLY from B before
   selecting B, demonstrating the mismatch rather than merely exposing a
   hypothetical corner.

Changing the RTL to emulate the current oracle would violate the frozen
explicit-release ledger.  Changing the simulator would alter normalized
policy semantics outside this RTL task.  The instruction's
`GLOBAL candidate identity differs from software oracle` / ambiguous-behavior
stop condition therefore applies.

Gate D is independently stopped because the retained BIRA collector has no
frozen CandidateStore capacity/port/lifetime/overwrite contract.  Its nominal
retained bits cannot be counted as reusable GLOBAL scratch while final
reconstruction may still require them.  A WithScratch implementation would be
fictional at this point.

## Why synthesis did not start

The required order authorizes RS2 synthesis only after Gates B, C, and D all
close functionally.  B is closed at the stated boundary; C and D are blocked.
Starting DC would violate the phase gate, so there are no new area/timing
numbers and no long synthesis campaign was launched.

## Readable-Verilog public gate matrix

The `readable-verilog-generator` workflow influenced the use of Verilog-2001,
same-name spec documents, explicit reset/cycle contracts, and the eight-gate
report below.  Its bundled CLI cannot initialize under the installed Python
3.8.10 because it evaluates Python 3.9 `dict[...]` annotations during import.
No dependency was installed or upgraded.

| Gate | State | Evidence / limitation |
|---|---|---|
| compile | PASS | Verilator built and executed both RESOURCE_POINT elaborations; RS2 integrated top lint passed. |
| ast | BLOCKED | Bundled formatter-AST CLI fails at import under Python 3.8.10. |
| readability | MANUAL_PASS / AUTOMATED_BLOCKED | Source review completed; strict automated gate unavailable. |
| comment | MANUAL_PASS / AUTOMATED_BLOCKED | Ancestry, semantic contract, and non-obvious state transitions are documented. |
| naming | MANUAL_PASS / AUTOMATED_BLOCKED | Canonical policy names and functional filenames used; strict checker unavailable. |
| profile | BLOCKED | `erie_strict` gate did not initialize. |
| testbench | PASS | E1-E5, W1, and 2,000 seeded random vectors total; 0 mismatches. |
| toolchain | PARTIAL_PASS | Local Verilator compile/simulation/lint passed; synthesis intentionally NOT_RUN by phase gate. |

## Required next decisions

Before Gate C resumes, freeze one GLOBAL resource contract and update its
software oracle/golden accordingly:

- whether a borrower may reserve a future donor's line before that donor's
  action is selected, or only consume an already released resource; and
- whether first-complete-tuple ordering is `R,L,RB,B` as specified here or
  historical software slot order `L,R,B,RB`.

Before Gate D resumes, freeze a concrete scratch interface and prove that it
preserves candidate history through search and final reconstruction.
