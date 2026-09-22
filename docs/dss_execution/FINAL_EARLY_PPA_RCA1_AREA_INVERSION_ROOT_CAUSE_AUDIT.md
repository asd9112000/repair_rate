# FINAL-EARLY-PPA-RCA1 — Area Inversion Root-Cause Audit

```text
AUDIT_SCOPE: READ_ONLY_FORENSIC
RTL_MODIFIED: NO
SIMULATOR_SEMANTICS_MODIFIED: NO
SYNTHESIS_RUN: NO
LATENCY_EXPERIMENT: NO
DSS_FINAL_MODIFIED: NO
```

## Executive conclusion

The apparent `EARLY > GROUP` inversion is **not** evidence that the four-token
EARLY resource policy is intrinsically more expensive than final GROUP/GLOBAL.
The synthesized designs are not semantically matched: E is a corrected
Model-B2, four-per-SA, 260-bit/11-Hybrid-entry decision boundary; G is the
archived single 204-bit/7-Hybrid-entry GROUP boundary.  The existing
Model-B2 compatibility audit found 259 differences in 1,000 deterministic
vectors, including 41 Model-B2-only repairable cases.

The frozen DC hierarchy reports explain the complete cell-area delta without
inventing a new synthesis run:

| Matched report row / local logic | E (µm²) | G (µm²) | E − G (µm²) | Interpretation |
| --- | ---: | ---: | ---: | --- |
| Shared analyzer specialization | 168,362.4111 | 73,144.2102 | **+95,218.2009** | Same analyzer source, but E elaborates `HYBRID_ENTRIES=11`; G elaborates 7. |
| Top-local logic | 26,205.3794 | 16.6320 | **+26,188.7474** | E selects one 260-bit summary from four banks before the analyzer; G directly wires one 204-bit summary. |
| Controller/core | 6,256.9585 | 33,180.8404 | **−26,923.8819** | E removes G's candidate store, static 81-path selector, and deferred atomic controller. |
| **Total cell area** | **200,824.7490** | **106,341.6826** | **+94,483.0664** | Exact sum of the three differences above. |

Thus the dominant cause is the Model-B2 semantic/input-boundary expansion,
not accidental replication.  E has exactly one elaborated shared analyzer,
not four.  Its 4-bit persistent availability state is small; it is hidden by
a much larger combinational Model-B2 front end.

The thesis-safe recommendation is **Path C**: retain the existing Final
GROUP/GLOBAL semantic boundary and derive a new, semantically matched minimal
EARLY from that mother before making an EARLY-versus-GROUP policy-area claim.
This does not invalidate the archived Model-B2 EARLY result; it preserves it
as evidence for the different Model-B2 boundary.

## Scope, provenance, and comparability

| Item | Design E — final Model-B2 EARLY | Design G — archived GROUP/GLOBAL |
| --- | --- | --- |
| Archive | `dss_final/recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch/` | `dss_final/recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch/` |
| Top | `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top` | `recam_dss_hyp02_static_global_top` |
| RTL provenance | `7fdaf0ad86700d40588f31735d4eb2513b77478f` | `35a60728642e73f5d2c428d2a345ef8c6817bdaa` |
| Analyzer specialization | 11 Hybrid entries / 260 bits per SA | 7 Hybrid entries / 204 bits |
| Summary boundary | four independently retained SA summaries = 1,040 input bits | one summary = 204 input bits |
| Control policy | streaming `A,B,C,D`; `R,L,RB,B`; immediate prefix commits | collect all 16 SA/slot candidates, fixed 81-path selection, atomic commit |
| Total cell area | 200,824.749004 µm² | 106,341.682630 µm² |
| Timing condition | DC W-2024.09-SP2, `slow.db`, 20 ns, zero I/O; WNS/TNS 0/0 | same tool/library/period/I/O condition; WNS/TNS 0/0 |

`GROUP_MODEL_B2_COMPATIBILITY_AUDIT.md` is decisive on semantic matching:

```text
GROUP_MODEL_B2_COMPATIBILITY: FAIL
MISMATCHES_1000: 259
MODEL_B2_ONLY_REPAIRABLE: 41
VECTOR216_GROUP_DIFFERENCE: YES
C1R16_GROUP_DIFFERENCES: 13 / 16
ARE_THE_TWO_SYNTHESIZED_DESIGNS_SEMANTICALLY_MATCHED: NO
```

No conclusion below treats the 94,483.066374 µm² difference as a pure
EARLY-versus-GROUP policy cost.

## Source-manifest diff

The following covers every source in the two frozen manifests.  A primary
class is shown first; tags make the Model-B2/pre-Model-B2 boundary explicit.
`SAME_ROLE_DIFFERENT_IMPLEMENTATION` does not mean an accidental rewrite: the
source audit identifies the corresponding intentional policy or input-boundary
change.

| Source/module | E | G | Classification | Audit finding |
| --- | --- | --- | --- | --- |
| `recam_shared_config_analyzer.sv` / `recam_shared_config_analyzer` | yes, SHA256 `b4aa0711...d5b6c` | yes, same SHA256 | `IDENTICAL_HASH` | Same analyzer algorithm and ports.  E changes only elaboration parameter 7 → 11. |
| E top / `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top` | yes | corresponding G top differs | `SAME_ROLE_DIFFERENT_IMPLEMENTATION`; `MODEL_B2_ONLY` | Replaces G's one direct 204-bit summary input with a four-bank 1,040-bit Model-B2 boundary and a selected-summary mux. |
| G top / `recam_dss_hyp02_static_global_top` | no | yes | `GROUP_ONLY`; `LEGACY/PRE_MODEL_B2_ONLY` | Directly wires one 7-entry summary into the analyzer and exposes G's atomic-result interface. |
| E core / `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core` | yes | corresponding G core differs | `SAME_ROLE_DIFFERENT_IMPLEMENTATION`; `MODEL_B2_ONLY` | Streaming `ABCD`/`R,L,RB,B` controller and four persistent common-spare tokens; no candidate store or global selector. |
| G core / `recam_dss_hyp02_static_global_core` | no | yes | `GROUP_ONLY`; `LEGACY/PRE_MODEL_B2_ONLY` | 16-cycle candidate collection, deferred decision, and atomic output publication. |
| `dss_v2_group_candidate_store.sv` | no | yes | `GROUP_ONLY`; `LEGACY/PRE_MODEL_B2_ONLY` | 80-bit history required by the G collect-then-select policy. |
| `recam_dss_hyp02_static_selector.sv` | no | yes | `GROUP_ONLY`; `LEGACY/PRE_MODEL_B2_ONLY` | Fixed priority across 81 legal complete tuples. |
| `dss_v2_group_slot_decode.sv` | no standalone instance | yes | `GROUP_ONLY`; `LEGACY/PRE_MODEL_B2_ONLY` | G's collection decoder.  E retains its fixed SA/slot meaning as an audited constant table in its core, rather than reusing the module. |
| `dss_v2_params_pkg.sv` | no | yes | `GROUP_ONLY`; `LEGACY/PRE_MODEL_B2_ONLY` | Types/constants needed by G-only packaged modules; not needed by E's standalone closure. |
| `dss_v2_types_pkg.sv` | no | yes | `GROUP_ONLY`; `LEGACY/PRE_MODEL_B2_ONLY` | Same reason as the parameter package. |

There is no E-only collector RTL source in the frozen manifest.  The E
collector summaries are deliberately retained outside the synthesized
decision/resource-management boundary and arrive through the top input.

## Elaborated hierarchy and functional roles

```text
E: recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top
   ├─ external retained collector state: 4 × 260-bit summaries [not instantiated]
   ├─ 4:1 × 260-bit selected-summary mux (top-local combinational logic)
   ├─ analyzer: recam_shared_config_analyzer (one instance; H=11)
   │  ├─ config-specific canonical projection
   │  ├─ dictionary reconstruction and 5×5 matrix construction
   │  └─ ten-pattern candidate evaluation / first PatternID production
   └─ core: streaming EARLY resource controller
      ├─ fixed SA/slot → ConfigID / claim-mask decode
      ├─ four common-spare availability tokens
      └─ incremental commit and result storage

G: recam_dss_hyp02_static_global_top
   ├─ external retained collector state: one 204-bit summary [not instantiated]
   ├─ analyzer: recam_shared_config_analyzer (one instance; H=7)
   │  ├─ config-specific canonical projection
   │  ├─ dictionary reconstruction and 5×5 matrix construction
   │  └─ ten-pattern candidate evaluation / first PatternID production
   └─ core: GROUP/GLOBAL controller
      ├─ candidate_store: 80-bit registered candidate history
      ├─ collect_slot_decode
      ├─ static_selector: 81 legal fixed paths
      └─ deferred atomic commit and result storage
```

There are no standalone projector, matrix-builder, or candidate-analyzer
modules in either design: those roles are folded into the one shared analyzer.
There is no internal collector instance in either frozen top.

### Mandatory analyzer / front-end instance check

| Function | E exact count | G exact count | Evidence and conclusion |
| --- | ---: | ---: | --- |
| Internal collector | 0 | 0 | Neither top instantiates a collector; both accept collector metadata as inputs. |
| External retained summaries | 4 × 260 bits | 1 × 204 bits | Input-boundary state, not mapped registers inside these tops. |
| Selected-summary projector/mux | 1 four-way 260-bit mux | 0 separate mux | E `case(current_sa)` selects one bank; G wires its one summary directly. |
| Standalone matrix builder | 0 | 0 | Folded into the analyzer in both cases. |
| Config analyzer | 1 | 1 | One `analyzer` instance in each top and one corresponding DC hierarchy row. |
| Pattern/candidate analyzer | 1 role, folded into the above instance | 1 role, folded into the above instance | The common source creates candidate-valid and first PatternID outputs. |
| Candidate store | 0 | 1 | G-only, 80-bit module. |
| Static selector | 0 | 1 | G-only, 81-path module. |

```text
SHARED_ANALYZER_INSTANCE_COUNT_E: 1
SHARED_ANALYZER_INSTANCE_COUNT_G: 1
COLLECTOR_INSTANCE_COUNT_E: 0
COLLECTOR_INSTANCE_COUNT_G: 0
PROJECTOR_INSTANCE_COUNT_E: 1 top-local summary mux
PROJECTOR_INSTANCE_COUNT_G: 0 (direct wire; no separate projector)
MATRIX_BUILDER_INSTANCE_COUNT_E/G: 0 standalone; 1 folded analyzer each
CONFIG_ANALYZER_INSTANCE_COUNT_E/G: 1 / 1
CANDIDATE_ANALYZER_INSTANCE_COUNT_E/G: 1 folded role / 1 folded role
ACCIDENTAL_FOUR_ANALYZER_REPLICATION: NO
```

## What Model-B2 added at this boundary

“Present” here distinguishes external metadata retained at the top interface
from synthesized sequential state.  It must not be read as a claim that E
internally registers 1,040 collector bits.

| Structure | Present in E | Present in G | Bits / width / instances | Area if available | Semantic reason |
| --- | --- | --- | --- | --- | --- |
| Raw retained collector metadata | yes, external | yes, external | E: 4 × 260 = 1,040 input bits; G: 1 × 204 input bits | no separate mapped collector area | E must preserve one full Model-B2 summary for each SA until that SA is analyzed. G uses one archived, truncated summary boundary. |
| Hybrid retention capacity | yes | yes | E 11 entries/SA; G 7 entries | included in analyzer specialization | The extra four records per E SA preserve C1R3-required reachable information. |
| Model-B2 extra field capacity | yes | no | +56 bits/SA: 4 valid, 12 pointer, 4 descriptor, 36 differing-address bits; +224 bits across E's four SA inputs | no separate state cell area | Restoring seven entries is disproved by C1R3, Vector216, and analogous directed cases. |
| Multiple per-SA retained states | yes, external | no | E four independent summary banks; G one input | E top-local logic is 26,205.3794 µm², but this is not a mux-only measurement | Needed by E's per-SA streaming analysis. |
| Post-Must/config projection | yes | yes | one shared analyzer each | E 168,362.4111; G 73,144.2102 for complete analyzer blocks | Common algorithm; E's larger H=11 specialization is part of the Model-B2 envelope. |
| Dictionary reconstruction / 5×5 matrix | yes | yes | one folded analyzer each | included above | Same source; hybrid loop is elaborated to 11 or 7 entries. |
| Additional analyzer instances | no | no | 1 / 1 | n/a | No accidental replication. |
| Generation/version tracking | no | no | 0 / 0 bits | n/a | No such signal/module is in either frozen source manifest. |
| Additional repair-decode payload | no | G has more | E outputs no ledger/donor/release fields; G additionally has 12 ledger + 8 donor + 4 release output bits | included in cores | E does not gain a large output payload; G carries more deferred GROUP metadata. |

The increased H parameter alone is not a linear per-entry cell-area estimate.
The analyzer contains dependency reconstruction, dictionary matching, and
ten candidate cones; DC optimization, the changed selected-summary driver,
and the 20 ns timing target make an exact per-entry attribution unavailable
without a new controlled experiment, which is out of scope here.

## Area by hierarchy

The existing reports have only 3 E and 6 G hierarchy rows, including the root.
They do **not** contain fifteen independent hierarchy/module contributors.
No new synthesis or hierarchy report was generated; therefore a fabricated
“top 15” is deliberately not supplied.  The complete available ranked rows
are below.  Root rows are totals, not additive child contributors.

| Rank | E hierarchy row | Area (µm²) | % total E | G hierarchy row | Area (µm²) | % total G |
| ---: | --- | ---: | ---: | --- | ---: | ---: |
| 1 | root top (total) | 200,824.7490 | 100.0 | root top (total) | 106,341.6826 | 100.0 |
| 2 | `analyzer` | 168,362.4111 | 83.8 | `analyzer` | 73,144.2102 | 68.8 |
| 3 | top-local logic | 26,205.3794 | 13.0 | `core` | 33,180.8404 | 31.2 |
| 4 | `core` | 6,256.9585 | 3.1 | `core/candidate_store` | 21,771.2883 | 20.5 |
| 5 | — | — | — | `core/static_selector` | 5,701.4497 | 5.4 |
| 6 | — | — | — | `core/collect_slot_decode` | 405.8208 | 0.4 |
| 7–15 | unavailable in frozen report | — | — | unavailable in frozen report | — | — |

Additional report-wide evidence is consistent with the hierarchy split:

| Report metric | E | G | E − G |
| --- | ---: | ---: | ---: |
| Combinational area (µm²) | 198,107.0802 | 98,498.0313 | +99,609.0489 |
| Sequential area (µm²) | 2,717.6688 | 7,843.6514 | −5,125.9825 |
| Buf/inverter area (µm²) | 40,921.3735 | 15,667.3443 | +25,254.0292 |
| Buf/inverter count | 3,067 | 1,530 | +1,537 |
| Logic levels on reported critical path | 102 | 77 | +25 |

The added buffer/inverter area and deeper E path are consistent with the wide
summary-selection and expanded analyzer cone.  They do not establish an RTL
bug by themselves.

## Sequential-state audit

| Category | E architectural bits | G architectural bits | Notes |
| --- | ---: | ---: | --- |
| Internal collector state | 0 | 0 | Collector metadata is external at both top boundaries. |
| Sharing-policy state | 4 | 0 | E `common_spare_available_o[3:0]`; G performs static atomic selection rather than token-by-token commit. |
| Candidate-history state | 0 | 80 | G `store_q`: 16 candidates × (valid + PatternID[3:0]). |
| Repair-result payload | 39 | 65 | E: repairable/commit/config/pattern/borrow/failure. G additionally carries ledger, donor, release, and atomic result fields. |
| FSM/counters/protocol | 6 | 6 | E: SA/rank plus busy/done. G: state plus SA/slot collection counters. |
| Generation/version state | 0 | 0 | Absent in both. |
| **Total logical architectural state** | **49** | **151** | G's 151-bit figure is the archived state-accounting value. |
| **Mapped sequential cells** | **49** | **141** | From the respective DC area reports. |
| **Mapped sequential area** | **2,717.668842 µm²** | **7,843.651356 µm²** | From the respective DC area reports. |

The difference between G's logical 151 bits and 141 mapped sequential cells
is a synthesis optimization/mapping outcome; the DC count, not a hand-derived
bit count, is used for its area statement.  E's source-level 49 state bits
match its 49 mapped sequential cells.

```text
IS_THE_4_BIT_EARLY_SHARING_CONTROLLER_ACTUALLY_SMALL_BUT_HIDDEN_BY_A_LARGER_MODEL_B2_FRONTEND: YES
E_CORE_AREA: 6,256.9585 µm² (3.1% of E)
E_ANALYZER_PLUS_TOP_LOCAL_AREA: 194,567.7905 µm² (96.9% of E)
```

The four availability bits are only one component of E's 49-bit core state;
the 6,256.9585 µm² core also contains protocol and committed-result registers.

## Combinational representation audit

| Finding | E-specific? | Classification | Evidence / consequence |
| --- | --- | --- | --- |
| Four-way 260-bit selected-summary mux | yes | `SEMANTICALLY_REQUIRED` at the frozen E boundary | The top selects one of four per-SA retained Model-B2 summaries with an explicit `case(current_sa)`.  It is not four analyzer instances.  A different upstream banking/pipeline contract might optimize it, but that would require a separate equivalence and timing review. |
| 11-entry hybrid reconstruction loop | parameter-expanded in E | `SEMANTICALLY_REQUIRED` | Same analyzer source as G, elaborated for 11 rather than 7 records.  C1R3 establishes that 7-entry truncation is not valid for Model-B2. |
| Dictionary/matrix procedural arrays and data-dependent dictionary insertion | shared | `LIKELY_OPTIMIZABLE` | The common analyzer uses procedural arrays and data-dependent dictionary indices.  Synthesis realizes these as combinational selection/update logic.  This is a representation candidate, but not an E-only defect and no equivalent replacement has been proven. |
| Fixed ten candidate cones | shared | `SEMANTICALLY_REQUIRED` for this combinational analyzer contract | The analyzer evaluates the legal pattern candidates and produces the first valid PatternID/candidate visibility.  E does not evaluate all ConfigIDs at once: it drives one `current_config_id_o` per cycle. |
| Simultaneous per-SA analyzer evaluation | no | `SEMANTICALLY_REQUIRED` absence | E serializes SA selection through one analyzer and one selected summary; it does not evaluate four SAs in parallel. |
| Dynamic runtime Config table / dynamic resource indexing in E core | no | `SEMANTICALLY_REQUIRED` absence | E uses explicit fixed `case` decode for SA and priority rank; C3 records no dynamic resource indexing. |
| Duplicated candidate banks | no | `SEMANTICALLY_REQUIRED` absence | E has no candidate-store module or replicated analyzer. |
| G candidate-store variable-base read/write | G-only | `REPRESENTATION_ARTIFACT` / `LIKELY_OPTIMIZABLE` | `store_q[write_offset +: 5]` and dynamic read offsets can infer wide write/read selection logic.  This is a real G cost (21,771.2883 µm² block), but its removal makes E smaller at the core, not larger. |
| G 81-path static selector | G-only | `SEMANTICALLY_REQUIRED` for archived G policy | 81 legality terms and fixed priority implement G's atomic complete-tuple choice; its 5,701.4497 µm² is absent from E. |

No evidence supports an accidental full-width crossbar, four independent
analyzers, duplicated candidate banks, simultaneous all-Config evaluation, or
simultaneous all-SA evaluation in E.  The audit does identify future
optimization candidates in the shared analyzer and at the E summary-boundary
transport, but cannot attribute the 94.5k µm² inversion to an unproven
representation defect.

## GROUP-mother derivation audit

```text
WAS_FINAL_EARLY_IMPLEMENTED_AS_FINAL_GROUP_MINUS_GROUP_ONLY_STRUCTURES_PLUS_MINIMAL_EARLY_CONTROLLER: PARTIAL
```

It is a valid lineage adaptation, not a clean-sheet analyzer rewrite:

- The analyzer source hash is exactly identical, and ConfigID, PatternID,
  topology, and candidate interpretation are retained.
- E correctly removes G's 80-bit candidate store, 81-path selector, deferred
  collection/decision mechanism, and atomic commit path.
- E replaces the G core/top with the intended streaming controller and fixed
  decode.  That replacement is why E's core is 26,923.8819 µm² smaller.

It is only **partial** as a minimal *area-comparison* derivation because its
front-end boundary is not the G front end: E changes one 204-bit/7-entry input
to four 260-bit/11-entry inputs.  The added capacity is semantically justified
for Model-B2, but it prevents a direct mother-minus-policy comparison.  No
G structure that should have been reused unchanged was reimplemented in a way
that explains the inversion; the shared analyzer was reused unchanged.  The
G slot decoder was intentionally folded into E's fixed 16-entry table.

## Counterfactuals (structural only; no invented area)

### Normalized, final-GROUP front end plus minimal EARLY

Starting with exactly G's one 204-bit/7-entry analyzer front end and the same
candidate interpretation, then removing the 80-bit candidate store, the
81-path selector, and deferred GROUP decision and adding the small streaming
EARLY controller would remove G-specific hardware and would not add the E
four-bank mux or H=11 expansion.

```text
NORMALIZED_GROUP_FRONTEND_EARLY_EXPECTED_RELATIVE_AREA: EARLY < GROUP
```

This is a structural expectation, not an area number or a synthesized claim.
It is the comparison needed for a thesis statement about policy overhead at
the archived Final GROUP semantic boundary.

### If GROUP were upgraded to Model-B2

| Category | Required structural content |
| --- | --- |
| Model-B2 common front end | 11-entry/260-bit per-SA retained summaries; per-SA preservation or a semantically equivalent serialized-bank mechanism; selected-summary transport; H=11 analyzer specialization; dictionary/matrix/candidate logic. |
| EARLY-only | fixed `ABCD`/`R,L,RB,B` streaming controller, four tokens, immediate prefix commits, no candidate history. |
| GROUP-only | 16 candidate history entries/80-bit store (or an equivalent Model-B2 candidate history), complete-tuple legality/selection, deferred atomic commit, ledger/donor/release result fields. |

```text
MODEL_B2_GROUP_WOULD_REQUIRE_SAME_LARGE_FRONTEND: YES
WOULD_THE_200_8K_VS_106_3K_INVERSION_LIKELY_DISAPPEAR_AFTER_G_IS_MODEL_B2: YES, LIKELY
```

That likelihood follows structurally: a Model-B2 GROUP would inherit the
dominant H=11 front end and either the same multi-SA bank/mux transport or an
equivalent retained-state organization, then add GROUP-only storage/selection.
It is not a numerical PPA prediction and must not be promoted to one without
a separately authorized matched synthesis.

## Semantic-boundary decision support

| Evidence | Semantic boundary | Reuse consequence |
| --- | --- | --- |
| Final GROUP/GLOBAL RTL | `PRE_MODEL_B2 / FINAL-GROUP-COMPATIBLE` | Frozen 204-bit/7-entry source and fixed static-GLOBAL behavior. |
| Existing GROUP synthesis | `PRE_MODEL_B2 / FINAL-GROUP-COMPATIBLE` | Directly reusable only for that same archived G boundary. |
| Formal repair-rate corpus / frozen group evidence | `PRE_MODEL_B2 / FINAL-GROUP-COMPATIBLE` | The formal group-policy corpus predates C1R3's 260-bit/11-entry collector correction.  It should not be claimed as a Model-B2 result. |
| Historical Directional EARLY repair-rate results | `PRE_MODEL_B2` unless explicitly rerun after C1R3/C1R4 | Earlier EARLY evidence does not establish the corrected Model-B2 collector semantics. |
| C1R3 / C1R4 | `MODEL_B2` | Directed/shared-collector and fixed-action correctness evidence; expressly not a formal repair-rate sweep. |
| New EARLY RTL and 1,000-vector lockstep | `MODEL_B2` | Corrected E controller evidence, with 4 × 260-bit summaries and H=11. |
| GROUP Model-B2 compatibility audit | boundary-crossing evidence | Fails compatibility: old G cannot be relabeled as Model-B2. |

```text
FINAL_GROUP_SEMANTICS_REUSE_EXISTING_REPAIR_RATE: YES
FINAL_GROUP_SEMANTICS_REUSE_GROUP_PPA: YES
FINAL_GROUP_SEMANTICS_REUSE_OLD_EARLY_FUNCTIONAL_EVIDENCE: PARTIAL
FORMAL_REPAIR_RATE_SEMANTIC_BOUNDARY: PRE_MODEL_B2
```

The `PARTIAL` old-EARLY answer is deliberate: static topology/config naming
and some policy evidence can guide a new matched implementation, but a
pre-Model-B2 result must not be represented as verification of the corrected
260-bit/11-entry Model-B2 collector.

## Ranked root-cause attribution

```text
CAUSE_1_NOT_MINIMAL_GROUP_DERIVATION: MINOR
CAUSE_2_HARDWARE_UNFRIENDLY_RTL: MINOR
CAUSE_3_MODEL_B2_SEMANTIC_EXPANSION: DOMINANT
CAUSE_4_ACCIDENTAL_DUPLICATION: NONE
```

1. **Model-B2 semantic expansion — dominant.** E's richer analyzer costs
   95,218.2009 µm² more, and its multi-SA input selection appears in a top
   with 26,188.7474 µm² more local area.  The expansion is justified by C1R3
   evidence, but it makes the PPA comparison unmatched.
2. **Not-minimal-as-a-comparison derivation — minor.** The shared analyzer is
   bit-identical, and E removes the appropriate G-only policy blocks.  The
   non-minimal part is the necessary changed front-end boundary, not an
   unrelated replacement.
3. **Hardware-unfriendly representation — minor.** The shared analyzer's
   generic procedural construction and E's wide bank mux are plausible
   optimization targets.  There is no equivalent optimized RTL or matched
   synthesis proving they dominate; G itself has the stronger dynamic
   candidate-store representation cost that E removes.
4. **Accidental duplication — none.** RTL and DC hierarchy both show one
   analyzer in each design and zero internal collector instances.

## Decision tree and recommendation

| Path | Fairness / evidence status | Relative time to thesis area | Assessment |
| --- | --- | --- | --- |
| A. Keep Model-B2 and optimize E only | Not a fair E-vs-current-G comparison; G remains pre-Model-B2 | MEDIUM | Not recommended.  It could improve E, but cannot repair the semantic mismatch. |
| B. Keep Model-B2 and upgrade G, then resynthesize both | Fair after full Model-B2 GROUP RTL/equivalence/PPA closure | SLOWEST | Technically appropriate if Model-B2 is the thesis-final architecture. |
| C. Freeze thesis/DATE semantics to Final GROUP boundary and derive matched minimal EARLY | Uses the largest already validated functional, repair-rate, and GROUP-PPA evidence set | FASTEST | **Recommended for the stated thesis deadline.** |

```text
RECOMMENDED_PATH: C
FASTEST_FAIR_PATH_TO_THESIS_AREA: C
```

Path C is a decision-support recommendation, not authorization to change an
archive.  The new Model-B2 EARLY archive remains intact evidence; any decision
to select a different thesis-final boundary requires human approval.

## Audit limits and verification discipline

- Existing DC reports were used as requested.  Their hierarchy depth is
  insufficient for a top-15 leaf-block table, and no synthesis was run to
  manufacture one.
- No numeric counterfactual was estimated.
- The RTL review was source and report based.  The readable-Verilog review
  workflow was used for hierarchy/interface/representation inspection; it did
  not authorize a source change or a new generated hardware claim.
- The initial worktree was not clean because `results/final_early/` was
  already untracked raw DC output.  It was preserved as evidence.  This audit
  adds this requested report only and does not alter that directory.

## Final return

```text
FINAL_EARLY_PPA_RCA1_STATUS: COMPLETE
SEMANTICALLY_MATCHED_E_VS_G: NO

CAUSE_1_NOT_MINIMAL_GROUP_DERIVATION: MINOR
CAUSE_2_HARDWARE_UNFRIENDLY_RTL: MINOR
CAUSE_3_MODEL_B2_SEMANTIC_EXPANSION: DOMINANT
CAUSE_4_ACCIDENTAL_DUPLICATION: NONE

EARLY_SHARED_ANALYZER_INSTANCES: 1
GROUP_SHARED_ANALYZER_INSTANCES: 1
EARLY_COLLECTOR_INSTANCES: 0 internal; 4 external 260-bit summaries
GROUP_COLLECTOR_INSTANCES: 0 internal; 1 external 204-bit summary

EARLY_SEQUENTIAL_BITS: 49 logical / 49 mapped sequential cells
GROUP_SEQUENTIAL_BITS: 151 logical / 141 mapped sequential cells

TOP_AREA_DELTA_BLOCKS:
  ANALYZER_H11_MINUS_H7: +95,218.2009 um2
  TOP_LOCAL_MULTI_SA_SUMMARY_PATH: +26,188.7474 um2
  EARLY_CORE_MINUS_GROUP_CORE: -26,923.8819 um2
  TOTAL_E_MINUS_G: +94,483.0664 um2

FOUR_BIT_CONTROLLER_ITSELF_SMALL: YES
FINAL_EARLY_ACTUALLY_DERIVED_MINIMALLY_FROM_GROUP: PARTIAL
NORMALIZED_GROUP_FRONTEND_EARLY_EXPECTED_RELATIVE_AREA: <
MODEL_B2_GROUP_WOULD_REQUIRE_SAME_LARGE_FRONTEND: YES
FORMAL_REPAIR_RATE_SEMANTIC_BOUNDARY: PRE_MODEL_B2
RECOMMENDED_PATH: C
FASTEST_FAIR_PATH_TO_THESIS_AREA: C
WORKTREE_CLEAN: NO (pre-existing untracked results/final_early/; this new report is also uncommitted)
NEXT_ACTION: STOP FOR HUMAN DECISION.
```
