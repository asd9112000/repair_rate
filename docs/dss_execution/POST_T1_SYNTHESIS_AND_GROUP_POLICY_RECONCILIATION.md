# Post-T1 Synthesis and GROUP Policy Reconciliation

## 1. Purpose

This is a documentation and source-trace reconciliation only.  It freezes the
completed DATE T1 latency evidence, identifies the paused synthesis target,
and distinguishes the active 2x2 DATE GROUP policy from both EARLY and the
historical global-search software path.  No synthesis, RTL, or C++ change is
authorized or performed.

## 2. Frozen DATE latency evidence

`DATE-LAT-T1-1M` is frozen in
`DATE2026_FORMAL_EXPERIMENT_EVIDENCE_INDEX.md`.  Its authoritative primary
scenario is `E0L_DATE_2X2_LATENCY_V1`: 1,000,000 groups, Uniform 28 fixed
faults/group (7 per SA), Mixed spatial model, seed `20260914`.

| Fully hidden | Mean exposed | Median | P95 | P99 | Max | Mean hidden slack |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 99.844600% | 0.003884 cycles | 0 | 0 | 0 | 4 | 2859.315087 cycles |

The result is provisional candidate readiness only.  It excludes final ledger
resolution, final repair decision, and final solution commit.

## 3. Historical synthesis targets

Three distinct targets must remain separate:

| Target | Boundary and purpose | Historical evidence |
| --- | --- | --- |
| V2 EARLY baseline | `recam_dss_v2_early_top`: streaming first-feasible local candidate, immediate A->B->C->D commit | Phase 4E 2x2 decision/resource-management DC report |
| V2 GROUP-NoScratch | `recam_dss_v2_group_top`: candidate collection, retained history, deferred allocation | Phase 4F 2x2 decision/resource-management DC report |
| S1G-A2G retained-state overlap | retained historical collector + restartable shared analyzer + final-only EARLY-family ownership controller | no DC run; synthesis paused |

The Phase 4E/4F reports are valid historical 2x2 decision-boundary results:
TSMC018 `slow.db`, 20.0 ns, zero IO delay, `tsmc18_wl10`, DC W-2024.09-SP2,
`compile -map_effort low`, and NAND2X1 area `9.979200`.  Their measured cell
areas are 80871.437539 (EARLY) and 105393.658552 (GROUP-NoScratch).

The separately stored H5 `rs3cs3m1` results are a distinct 3,3,1 target and
are neither an S1G-A2G result nor a substitute for this 2,2 overlap comparison.

## 4. Recently paused synthesis target

The recently paused target is exactly the isolated **S1G-A2G retained-state
overlap architecture**:

```text
rtl/dss_v2/top/recam_dss_v2_retained_collector_bank.sv
rtl/dss_v2/top/recam_dss_v2_retained_analyzer_path.sv
rtl/dss_v2/top/recam_dss_v2_retained_overlap_core.sv
```

`docs/dss_execution/DEVICE_LEVEL_EXPERIMENT_ROADMAP.md` records
`S1G-A2G synthesis: PAUSED`.  The S1G-A/S1F/S1G-A2G implementation records
call it `EARLY_OVERLAP`, preserve the baseline EARLY result semantics, and
state that GROUP is unchanged.  This is therefore an **EARLY-family overlap
architecture**, not a GROUP policy implementation.

## 5. EARLY architecture status

The active baseline is `recam_dss_v2_early_top` plus
`recam_dss_v2_early_core`.  At each SA it evaluates the frozen role rank,
tests resource feasibility against the live ledger, commits the first feasible
candidate, and then advances to the next SA.  It retains no candidate map or
candidate-history bank.

The historical Phase 4E result is valid for that decision boundary.  It is not
an apples-to-apples S1G-A2G cost comparator because it lacks the latter's four
821-bit collector banks, generation state, restart control, and BIST/test-done
interface.  A later comparison must rebuild a baseline at the explicitly
matched overlap boundary; reusing its area as a direct delta is prohibited.

## 6. GROUP-NoScratch V2 architecture status

The active DATE GROUP implementation is `recam_dss_v2_group_top` plus
`recam_dss_v2_group_core`, `dss_v2_group_candidate_store`,
`dss_v2_group_priority_reader`, `dss_v2_group_slot_decode`, and the common
resource ledger.  The Phase 4F 2x2 report is valid for this dedicated GROUP
decision/resource-management boundary, but it is not S1G-A2G overlap evidence.

## 7. Current GROUP RTL trace

The production top presents the shared analyzer result as
`local_candidate_valid = solution_valid && repairable`
(`recam_dss_v2_group_top.sv:41-48`).  The core has `IDLE`, `COLLECT`, and
`ALLOCATE` states (`recam_dss_v2_group_core.sv:15`).  During `COLLECT`, the
store writes the current `[SA][slot]` result (`:35-37`) and the controller
walks slot 0..3 for A..D; only after D slot 3 does it enter `ALLOCATE` (`:50`).

The ledger `commit_i` is `accept`, and `accept` is explicitly gated by
`state_q == ALLOCATE` (`:29,43`).  Thus no SA commits during collection.  In
`ALLOCATE`, each accepted candidate writes the ledger and result fields; only
then does the controller advance A->B->C->D.  Failure after the fourth rank
ends at the current SA (`:51`).

## 8. GROUP candidate-retention semantics

Current active retained GROUP state is derived directly from the modules:

| State | Active organization | Bits |
| --- | --- | ---: |
| Candidate history | 4 SAs x 4 canonical slots x `{valid, PatternID[3:0]}` | 80 |
| Valid map | one valid bit at each packed slot's LSB | included above |
| Pattern history | four-bit PatternID adjacent to that valid bit | included above |
| Config identity | static decode from `[SA][slot]`; not stored | 0 |
| Pivot/fault/Hybrid address history | not retained in the GROUP decision core | 0 |
| Ledger state | released[3:0], borrower-valid[3:0], borrower ID[7:0], two commit-status bits | 18 |
| Core state/counters | FSM[1:0], SA[1:0], slot/rank[1:0] | 6 |
| Registered decision results/status | busy/done/group-result, commit-valid, selected ConfigID/PatternID/donor/action, failure position | 53 |
| **Total retained GROUP state** | current decision/resource-management boundary | **157** |

The 80-bit store is confirmed by `dss_v2_group_candidate_store.sv:22-25,65-68`.
Its packed offset is `(SA * 4 + slot) * 5` (`:38-40`).  The allocator reads a
single selected-SA image and its fixed priority order is slot `1,0,3,2`
(`dss_v2_group_priority_reader.sv:21-38`).

## 9. A/B/C/D decision timing

GROUP first performs 16 collection cycles: four slots for A, then B, C, then
D.  Allocation starts only after the complete collection.  It then checks the
stored candidates and commits greedily in A->B->C->D order; accepted A updates
the ledger before B is evaluated, and so forth.  `done_o` is the terminal group
decision edge after D commit or the first terminal failed SA.

## 10. GROUP vs EARLY comparison

| Property | EARLY | GROUP-NoScratch V2 |
| --- | --- | --- |
| Local analysis | per SA | per SA during 16-slot collection |
| Final config selection | immediate first feasible candidate | deferred until all A/B/C/D candidate records exist |
| Candidate history | none | 16 `{valid, PatternID}` records, 80 bits |
| Wait for all candidate collection | no | yes |
| Shared-resource decision | online during SA progression | after collection |
| Ledger order | A->B->C->D | A->B->C->D |
| Rollback | no | no |
| Final decision point | per-SA commit / terminal edge | group allocation phase / terminal edge |

## 11. GROUP vs historical global-search distinction

The active `rtl/dss_v2/` implementation contains no `analyzeGroup(...)` or
global cross-SA combinational-search path.  The historical C++ paths are
`DirectionalMultiConfigAnalyzer::analyzeGroup` and
`DynamicRepairSimulator::findCompressedGroupChoice`; they remain outside the
active production GROUP RTL.  The current policy is greedy retained-candidate
allocation, not an exhaustive globally optimal GROUP selector.

## 12. Existing synthesis evidence

Historical baseline evidence is available and must remain labelled by scope:

| Evidence | Status | Comparable use |
| --- | --- | --- |
| Phase 4E V2 EARLY, 2x2 | available; timing closed at 20 ns | historical EARLY decision-boundary baseline |
| Phase 4F V2 GROUP-NoScratch, 2x2 | available; timing closed at 20 ns | historical GROUP decision-boundary baseline |
| H5 EARLY/GROUP, 3,3,1 | available; 20-ns timing not closed | 3,3,1 target only |
| S1G-A2G retained overlap | no synthesis artifact | no cost/timing claim |

## 13. Missing synthesis evidence

For the exact question, “what hardware cost hides T1 provisional analysis
behind BIST?”, both historical 2x2 decision-only baseline points require a
matched-boundary rerun or explicitly frozen wrapper before a direct delta is
reported.  The missing target itself is S1G-A2G with all four collector banks,
the retained analyzer path, overlap controller, and its intended final-only
ledger boundary.  No area, GE, sequential area/state, critical delay, WNS, or
critical-path result exists for that target.

## 14. Recommended next synthesis target

```text
PRIMARY_NEXT_SYNTHESIS_TARGET:
S1G-A2G retained-state overlap

PRIMARY_COMPARISON:
matched-boundary EARLY baseline

SECONDARY_CONTEXT_ONLY:
GROUP-NoScratch V2
```

T1 has quantified the latency benefit; the remaining research question is the
hardware cost required for that latency-hiding behavior.  GROUP-NoScratch is
useful policy/hardware context, but it is not another name for S1G-A2G.

## 15. Exact comparison methodology required

Before a later explicit synthesis authorization, freeze one source manifest
and SHA-256 set for each point, identical 2x2 DATE geometry
(`RS=2, CS=2, SHARE_M=1`), identical top-level input boundary, and identical
collector inclusion/exclusion.  The meaningful EARLY comparator must include
the same raw-fault/retained-collector and test-done boundary, but defer
analysis until the baseline policy point; otherwise its cost excludes the
state that enables overlap.

Use one DC W-2024.09-SP2 run methodology: TSMC018 `slow.db`, slow corner,
20.0 ns clock, zero input/output delay, `tsmc18_wl10`, `compile -map_effort
low`, and NAND2X1 area `9.979200`.  For every point retain area, hierarchy
area, timing, WNS/TNS, cell counts, state accounting, source manifest/hash,
and terminal DC log.  Report total cell area and GE separately from logical
state; do not compare against H5 3,3,1 or legacy/global-search results.

## Read-only RTL review matrix

This reconciliation traced source rather than delivering modified RTL.

| Gate | State |
| --- | --- |
| compile | NOT_RUN |
| ast | NOT_RUN |
| readability | NOT_RUN |
| comment | NOT_RUN |
| naming | NOT_RUN |
| profile | NOT_RUN |
| testbench | NOT_RUN |
| toolchain | NOT_RUN |

Existing functional and DC evidence is cited above but was not re-run.  The
current project venv resolves to Python 3.8.10, while the readable-RTL strict
gate requires Python 3.9+, so no new gate artifact was attempted for this
documentation-only task.
