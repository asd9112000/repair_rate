# FINAL-EARLY-B0 — Commit Timing and Minimal EARLY State Audit

## Scope

```text
FINAL_EARLY_B0_STATUS: COMPLETE
MODE: READ ONLY
CURRENT_HEAD: 1158bea22ba2d45c062fec1856c4411895482324
CURRENT_BRANCH: integration/date2026-directional-canonical-v1
FUNCTIONAL_RTL_MODIFIED: NO
SIMULATOR_MODIFIED: NO
SYNTHESIS_RUN: NO
```

The requested commit equals `HEAD`. The only pre-existing worktree change is
the untracked, documentation-only FINAL-EARLY-A audit.

The readable-verilog-generator analyze workflow and ASIC review rules were
used. Its CLI is blocked before AST analysis by Python 3.8 evaluating
`dict[str, ...]`; the skill package was not changed.

## Implementations and commit behavior

| Object | Call path | Commit behavior |
| --- | --- | --- |
| Repair-rate EARLY | `DynamicRepairSimulator::run` → `findV2GroupNoScratchChoice` | A→B→C→D sequential prefix |
| Repair-rate GROUP | `DynamicRepairSimulator::run` → `findDirectionalV2GroupGlobalCanonicalChoice` | DFS first chooses a full legal tuple |
| SYN-A EARLY RTL | `recam_dss_canonical_rs2_streaming_early_top` → analyzer + policy core + ledger | accepted SA updates result and ledger on one edge |
| HYP02 GROUP RTL | `recam_dss_hyp02_static_global_top` → core → selector | 16 captures, then one 81-path publish edge |

The simulator's `DirectionalV2Early` uses historical V2 slot priority
`L,R,B,RB`; SYN-A RTL uses frozen `R,L,RB,B`. Both are immediate-commit
EARLY, but they are not candidate-priority equivalent.

| Property | Current SYN-A EARLY | Current HYP02 GROUP |
| --- | --- | --- |
| Decision | current SA, first legal candidate | complete four-SA static path |
| Resource update | accepted SA edge | group allocation edge |
| Config/Pattern visibility | each field is registered at its SA edge | all four fields register together |
| Failure after A/B prefix | prefix remains; no rollback | no partial result publish |
| Completion | terminal `done_o` | `done_o` with `sa_commit_valid_o=4'hf` |

SYN-A writes `sa_commit_valid_q[sa_q]`, ConfigID, PatternID, donor and effect
fields when `candidate_accept` is true, while its resource ledger updates on
that same rising edge. The field slices are A=`[2:0]/[5:0]`, B=`[5:3]/[11:6]`,
C=`[8:6]/[17:12]`, and D=`[11:9]/[23:18]` for ConfigID/PatternID. They remain
visible through later commits and later failure.

HYP02 retains an 80-bit candidate image through collection. Its selector
chooses the lowest valid one of 81 legal paths. On its allocation edge it
writes all final ConfigID/PatternID/metadata registers and either commits all
four SAs or none. The archived core/top regressions and 65,536-map proof
verify this atomic behavior.

## Simulator timing distinction

The repair-rate simulator returns a completed `GroupRepairResult`; it does not
emit commit-cycle, per-SA output-valid, or group output-valid events.
`DssPostBistLatency` and `DssTimelineCorrelation` are separate timing models:
they consume supplied plans/traces and do not derive policy events from
`DynamicRepairSimulator` or drive an RTL interface.

```text
SIMULATOR_CYCLE_ACCURATE_COMMIT_TIMING: NO
SIMULATOR_TRACKS_PER_SA_EARLY_COMMIT_EVENT: NO
SIMULATOR_TRACKS_GROUP_FINAL_COMMIT_EVENT: NO
SEMANTIC_ORDER: IMPLEMENTED
CYCLE_ACCURATE_OUTPUT_TIMING: NOT MODELED BY REPAIR-RATE POLICY
```

## Minimal inter-SA state

The resource order is `{A_ROW,D_ROW,B_COL,C_COL}`. A resource's historical
ledger state is unreleased, released/free, or released/borrowed. Four monotonic
tokens can encode future availability only with the fixed A→B→C→D controller:
the combinational legality table must reject a donor whose owner is still
future. Otherwise `4'b1111` would incorrectly allow A to borrow B/C.

| SA | Action | Required free token | Tokens consumed on commit |
| --- | --- | --- | --- |
| A | L | none | A_ROW |
| A | R | none | none |
| A | B/RB | no prior-owner donor | illegal |
| B | L/R | none | B_COL / none |
| B | B/RB | A_ROW | B_COL+A_ROW / A_ROW |
| C | L/R | none | C_COL / none |
| C | B/RB | A_ROW | C_COL+A_ROW / A_ROW |
| D | L/R | none | D_ROW / none |
| D | B/RB | C_COL first, else B_COL | D_ROW+donor / donor |

For legal borrow actions, required mask equals the donor portion of the
consumed mask. BORROW_ONLY also consumes the current owner's resource. D needs
a fixed C-then-B two-input priority mux, so a single static D donor mask is
not exact, though generic resource-ID lookup is unnecessary.

No later legality depends on prior ConfigID, PatternID, or borrower identity;
it depends on fixed SA/rank control, owner-progress gating, and availability.
Final repair ConfigID/PatternID storage is a destination/interface concern, not
inter-SA policy state. The current 4 released + 4 borrower-valid + 8 borrower
ID bits can therefore reduce to four policy tokens only after exact fixed-table
equivalence is proven.

In the C-failure witness, A/B remain committed and visible, C fails, D is not
visited, and no rollback occurs. A streaming repair-destination interface is
semantically feasible; a group-wide result bank is not required for EARLY.
Borrower ID and 12-bit legacy ledger outputs are diagnostics/compatibility,
not future-legality inputs; their interface removal is separately authorized.

## Verdict

```text
EARLY_SIMULATOR_POLICY_SOURCE: DynamicRepairSimulator::run -> findV2GroupNoScratchChoice
GROUP_SIMULATOR_POLICY_SOURCE: DynamicRepairSimulator::run -> findDirectionalV2GroupGlobalCanonicalChoice
EARLY_IMMEDIATE_COMMIT: YES
EARLY_NO_ROLLBACK: YES
GROUP_JOINT_DECISION: YES
GROUP_FINAL_ATOMIC_DECISION: YES
SIMULATOR_SEMANTICALLY_DISTINGUISHES_EARLY_GROUP_COMMIT: YES
CURRENT_EARLY_RTL_PER_SA_COMMIT: YES
CURRENT_GROUP_RTL_ATOMIC_COMMIT: YES
EARLY_CONTRACT_ALREADY_MATCHES: PARTIAL
GROUP_CONTRACT_ALREADY_MATCHES: YES
HISTORICAL_CONFIGID_REQUIRED_FOR_INTER_SA_POLICY: NO
HISTORICAL_PATTERNID_REQUIRED_FOR_INTER_SA_POLICY: NO
CONSUMER_ID_REQUIRED_FOR_FUTURE_LEGALITY: NO
MINIMUM_INTER_SA_RESOURCE_STATE_BITS: 4, with fixed owner-order gating
INTER_SA_CONFIGID_STATE_BITS: 0
INTER_SA_PATTERNID_STATE_BITS: 0
BORROWER_ID_POLICY_BITS: 0
FOUR_COMMON_SPARE_MODEL_EXACT: PARTIAL
PER_SA_STREAMING_OUTPUT_FEASIBLE: YES
GROUP_WIDE_RESULT_BANK_REQUIRED_FOR_EARLY: NO
PARTIAL_COMMIT_BEFORE_LATER_FAILURE_ALLOWED: YES
ROLLBACK_REQUIRED: NO
LEGACY_DIAGNOSTICS_REQUIRED_IN_DSS_FINAL: PARTIAL
CANONICAL_SHARED_ANALYZER_CHANGE_REQUIRED: NO
FINAL_EARLY_ARCHITECTURE_HYPOTHESIS: PARTIAL
IMPLEMENTATION_READY: NO
NEXT_ACTION: STOP FOR HUMAN FINAL EARLY ARCHITECTURE REVIEW
```

Before implementation, freeze one candidate-priority contract and approve the
token table's future-owner exclusion and D C-then-B donor priority.
