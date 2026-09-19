# P3-4PT-SYNTH-REVIEW-A — Hardware-Cost Root-Cause Analysis

## Scope and status

```text
P3_4PT_SYNTH_REVIEW_A_STATUS: COMPLETE
AUTHORITATIVE_BASELINE: P3-4PT-SYNTH closure commit c81e733
SYNTHESIS_SOURCE_REVISION: 93f6384002029f97fb47106847367ad379316a59
ANALYSIS_SCOPE: RS=CS=2, m=1, 20 ns, TSMC018 slow corner, frozen DC method
RTL_MODIFIED: NO
SYNTHESIS_RERUN: NO
```

This is an RTL-to-mapped-hierarchy review, not a cross-architecture cost
comparison or an optimization authorization.  “Area” below always means the
frozen mapped total cell area, not DC's net-interconnect estimate and not a
technology-independent silicon-cost claim.

The requested readable-Verilog review class is `analyze`: no source was
written.  Its public evidence matrix is: `compile=PASS` (closure's exact-top
Verilator lint), `toolchain=PASS` (frozen DC), and `ast`, `readability`,
`comment`, `naming`, `profile`, `testbench` = `NOT_RERUN`.  The strict skill
gate remains `BLOCKED` by its Python 3.8 `dict[str, ...]` runtime issue,
already recorded by the closure; it is not RTL or DC evidence.

## Frozen quantitative decomposition

| Point | Total area | Producer area | Shared analyzer | Producer excluding analyzer | Evidence |
| --- | ---: | ---: | ---: | ---: | --- |
| SYN-B OPT1 | 395,369.256316 | 133,757.8720 | 69,668.1222 | 64,089.7498 | `SYN_B_OPT1/hierarchy_area.rpt` |
| SYN-C | 1,080,926.994646 | 953,569.1171 | 138,800.6941 | 814,768.4230 | `SYN_C/hierarchy_area.rpt` |
| SYN-D OPT1 | 1,267,025.771037 | 974,638.5353 | 141,581.5644 | 833,056.9709 | `SYN_D_OPT1/hierarchy_area.rpt` |

Thus SYN-C/SYN-D's non-analyzer producer logic is respectively **12.71x** and
**13.00x** SYN-B's.  The corresponding analyzers are only **1.99x** and
**2.03x**.  The 1x4 increase is therefore not primarily the analyzer.

| Major mapped block | SYN-C area / comb / seq | SYN-D area / comb / seq | RTL source and role |
| --- | ---: | ---: | --- |
| Top-local control and snapshot state | included in top: 35,356.3054 / 59,609.0887 | `impl`: 37,721.3758 / 63,600.7688 | streaming top or GLOBAL top; snapshots and phase FSM |
| Candidate producer excluding analyzer | 814,768.4230 / 755,721.4956 / 59,046.9274 | 833,056.9709 / 773,813.7858 / 59,243.1850 | `rtl/dss_1x4/producer/...candidate_producer.v` |
| Shared analyzer | 138,800.6941 / 138,800.6941 / 0 | 141,581.5644 / 141,581.5644 / 0 | `rtl/dss_1x4/producer/...shared_analyzer.v` |
| Streaming policy | 32,392.4834 / 26,488.1233 / 5,904.3601 | n/a | `...streaming_early_core.v` |
| GLOBAL search | n/a | 181,721.2334 / 102,419.8563 / 77,039.4250 | `...global_core.v` |
| Atomic commit | n/a | 9,343.8577 / 6,286.8961 / 3,056.9616 | `...global_atomic_group_commit.v` |

The non-analyzer producer numbers include the small mapped arithmetic children
(`add_86` / `add_86`) shown in hierarchy reports.  This avoids subtracting a
child twice.

## RTL → hardware map and retained-state inventory

The following bit counts are source-level logical state.  They are not a claim
that one bit maps to one flip-flop or that they alone explain mapped area.

| RTL structure | Module/file and source evidence | Width × depth; logical bits | Mapped hierarchy / area | Why retained / classification |
| --- | --- | ---: | --- | --- |
| Input snapshot bank | `...streaming_early_top.v:35-43,79-91`; same declarations at `...global_top.v:70-85,203-238` | 1,116 bits: pivots 360, eight threshold vectors 192, hybrid fields 560, overflow 4 | top-local; direct area not separately preserved | Holds all four SA collector results while producer serializes attempts. Must survive producer cycles; `ARCHITECTURE_INTRINSIC`. |
| Candidate-record bank | producer `:34-36,78,85-87` | 180 valid + 540 row-use + 360 col-use = **1,080 bits** | `producer`, 59,046.9274 seq (C) / 59,243.1850 seq (D) | Retains ten capacity attempts (A/D: 2; B/C: 3), 15 PatternID slots each. Persisted across producer-to-policy/search handoff. `ARCHITECTURE_INTRINSIC` for retained candidates; physical allocation of all 180 slots is `REPRESENTATION_DRIVEN`. |
| Producer scheduler | producer `:30-33,57,74-94` | state 2 + SA 2 + attempt 2 + done 1 = 7 bits | included in producer-local area | Serializes 10 requests; must survive cycles. `ARCHITECTURE_INTRINSIC`. |
| 1x4 analyzer temporary model | shared analyzer `:31-42,90-188` | 6×6 matrix = 36 bits; two 6×9 dictionaries = 108 bits; scalar temporaries | combinational `producer/shared_analyzer`, 138,800.6941 / 141,581.5644 | Elaborated as combinational logic, so it is not retained state. Six pivots plus up to ten hybrids and 2R2C/3R2C/4R2C PatternID enumeration are required by this frozen contract; `ARCHITECTURE_INTRINSIC` implementation with `REPRESENTATION_DRIVEN` loop/index realization. |
| Streaming policy state | `...streaming_early_core.v:25-45,103-141` | row allocation 24; result fields 70; state/counters ≈20; no second candidate copy | `policy`, 32,392.4834 | A→D selection, owner/donor reconstruction and persistent eight-row allocation. Mixed; `ARCHITECTURE_INTRINSIC`. |
| GLOBAL search copy and DFS state | `...global_core.v:35-63,154-247,314-316` | Candidate input copy **1,080**; ledger stack 96; cursor stack 24; borrow stack 8; used-row stack 20; result/current state ≈170 | `impl/search`, 181,721.2334 | Search owns a second candidate copy to explore and backtrack. Mixed; contract-required storage, but representation is reviewable. |
| GLOBAL commit shadow/persistent allocation | `...global_atomic_group_commit.v:24-42,97-133` | persistent allocation 24 + shadow allocation 24 + state/counters ≈8 | `impl/commit`, 9,343.8577 | Atomic all-four-SA publication; must survive cycles. `ARCHITECTURE_INTRINSIC`. |

Candidate-storage comparison: SYN-B keeps 4 SA × 4 actions × 10 PatternID
slots × {valid, release, borrow} = **480 bits** in three 160-bit banks
(`...candidate_map_producer.v:44-46,149-164`).  SYN-C/D keep 4 SA × 3 physical
attempt slots × 15 PatternIDs × {valid, used_rows[2:0], used_cols[1:0]} =
**1,080 bits**.  Only ten attempt positions are semantically scheduled in the
1x4 producer; nevertheless every 15-slot physical position is retained,
including unused high PatternIDs in lower-capacity attempts.  PatternID is
implicit in slot index; no separate PatternID bank is stored.  The 1x4 bank
holds candidate summaries, not full pivot/hybrid records, while the snapshot
bank holds the input reconstruction information during production.

## Dominant 1x4 combinational structures

Direct hierarchy evidence shows that about 92.7% (SYN-C) / 92.9% (SYN-D) of
the producer excluding analyzer is combinational.  The mapped hierarchy is
flat below producer, so individual cells cannot be attributed to a single
source statement.  The following associations are therefore source-grounded
inferences, marked as such, supported by the producer's 755.7k/773.8k local
combinational area and its critical paths.

| Suspect RTL construct | Location | Alternatives / width | Mapped evidence | Assessment |
| --- | --- | ---: | --- | --- |
| Dynamic four-SA snapshot selection | producer `:37-56` | `sa_q`: 4 alternatives; selects up to 360-bit hybrid-differing bundle plus all pivot/threshold fields | producer-local combinational area dominates; C/D critical path begins at `sa_q_reg` | **Direct source, inferred area attribution.** A wide variable part-select mux/decode cone. |
| Dynamic record-bank writes | producer `:85-87` | destination banks 180/540/360 bits; variable `sa_q*... + attempt_q*...` base; write slices 15/45/30 bits | producer local comb 755,066.1948 (C) / 773,304.8466 (D); endpoints are `candidate_used_rows_q_reg[*]` | **Strongest root-cause inference.** Decoder/enable/hold-and-select logic for wide retained vectors, not merely 1,080 FF bits. |
| Attempt-dependent PatternID table | analyzer `:44-88,168-187` | 3 attempt classes, 6/10/15 entries, 6-bit patterns | analyzer is 138.8k/141.6k comb | Direct analyzer cost, but only ~14.6% of producer. |
| Dynamic matrix/dictionary reconstruction | analyzer `:106-166` | 6×6 matrix, 2×6 dictionaries, variable pointer/index | analyzer hierarchy and analyzer DRC nets | Direct source, not the primary producer increase. |
| Candidate-table indexed selection in policies | streaming `:71-101`; GLOBAL search `:154-247` | 180 valid, 540 row, 360 col inputs; variable candidate/depth index | policy 32.4k; GLOBAL search 181.7k | Real mux/decode costs, but they cannot explain SYN-C's 0.95M producer because they lie outside that hierarchy. |
| Owner/donor reconstruction | streaming `:47-100`; GLOBAL core / commit | eight rows × 3-bit owner assignment; neighbor predicates | policy/commit/search hierarchies | Necessary policy semantics; not the main producer hotspot. |

The critical timing paths independently support this attribution: SYN-C starts
at `producer/sa_q_reg[1]` and ends at
`producer/candidate_used_rows_q_reg[333]` (19.81 ns); SYN-D starts at
`impl/producer/sa_q_reg[0]` and ends at
`impl/producer/candidate_used_rows_q_reg[350]` (19.72 ns first reported
path).  Both traverse SA selection, the shared analyzer, then producer write
logic.  SYN-B similarly runs from `candidate_map_producer/sa_q_reg[1]` to
`candidate_release_q_reg[46]` (19.88 ns).  SYN-A is different: its worst path
is `policy_core/sa_q_reg[1]` to
`policy_core/resource_ledger/borrower_valid_q_reg[0]` (19.79 ns), a
`LEDGER` path.

## Replication and search findings

The shared 1x4 analyzer uses six pivots and HYBRID_ENTRIES=10, compared with
SYN-B's five pivots and HYBRID_ENTRIES=7.  It also supports three capacity
forms (2R2C, 3R2C, 4R2C) and emits 15 candidate demand records.  Those changes
plausibly account for its observed ~2x area.  It is instantiated once, not
once per SA: producer serializes `sa_q` and `attempt_q`.  No evidence shows
four physical analyzer copies.

The large non-analyzer replication instead comes from retaining and dynamically
addressing 1,080 candidate-summary bits across ten scheduled capacity attempts,
plus wide SA-selected snapshot inputs.  It is intentional for the current
multi-cycle producer contract, but its flat-vector, dynamic-part-select
representation is the principal `REPRESENTATION_DRIVEN` cost suspect.  The
review found no evidence that PatternID is separately duplicated; it is
represented by position.

SYN-B's OPT1 search is 181,282.1494 and SYN-D's OPT1 search is
181,721.2334 (0.24% apart).  Both implement a comparable candidate-driven
GLOBAL DFS boundary.  Since their search areas are nearly equal while SYN-D's
total is 3.20x SYN-B's, GLOBAL DFS is conclusively **not** the primary 1x4
area cause.

## DRC review

| Point | DRC result | Hierarchy / signal evidence | Relation to hotspot |
| --- | --- | --- | --- |
| SYN-B | 102 max-fanout, 166 max-cap | producer `active_hybrid_differing[*]`, `active_sa_o[0]`, shared-analyzer nets; many search/core nets | Includes producer broadcast/select nets, but most violations are also search-related. |
| SYN-C | 16 max-cap | 6 producer-local unnamed nets and 10 shared-analyzer nets; each reports 0.15→0.16 | Consistent with dense producer/analyzer fanout; report lacks RTL names for the mapped internal nets. |
| SYN-D | 17 max-cap | 12 shared-analyzer nets, 3 `impl/search` nets, `active_hybrid_descriptor[1]`, and one producer net | Mostly analyzer/search, with one explicit producer broadcast bit. |

No max-fanout violations are reported for SYN-C/D.  The DRC reports do not
justify a causal claim stronger than “overlapping wide control/select cones”;
no DRC change is proposed.

## Human review shortlist (no change proposed)

| Priority | File / module | RTL construct | Area evidence | Opportunity class / semantic risk |
| ---: | --- | --- | --- | --- |
| 1 | `rtl/dss_1x4/producer/recam_dss_line1x4_rs2_cs2_m1_normalized_candidate_producer.v`, producer `:85-87` | Variable-base writes into 180/540/360-bit banks | 814.8k/833.1k producer excluding analyzer; 755.7k/773.8k comb | `REPRESENTATION_DRIVEN`; high risk to candidate identity, PatternID index, and cycle timing. |
| 2 | same producer `:37-56` | `sa_q`-selected flattened snapshot part-selects | both C/D critical paths start at `sa_q` | `REPRESENTATION_DRIVEN`; medium/high risk to serial request order and latency. |
| 3 | same producer `:34-36,78,85-87` | 1,080-bit bank with physical slots unused by 2R2C/3R2C | 59.0k seq plus its write/select cone | `POTENTIALLY_REMOVABLE_REDUNDANCY` only after proving fixed candidate identity/order; high risk to oracle equivalence. |
| 4 | `rtl/dss_1x4/producer/...shared_analyzer.v`, `:106-188` | matrix/dictionary reconstruction and 15-candidate nested loops | 138.8k/141.6k comb | `MIXED` intrinsic/representation; high risk to RECAM validity and PatternID. |
| 5 | `rtl/dss_1x4/policy/...global_core.v`, `:61-63,154-247,314-316` | second 1,080-bit candidate copy and variable candidate/depth reads | 181.7k GLOBAL search, predominantly mixed | `REPRESENTATION_DRIVEN`; high risk to policy ordering/backtracking and latency. |
| 6 | `rtl/dss_1x4/policy/...streaming_early_core.v`, `:71-101` | variable candidate lookup and eight-row owner/donor allocation | 32.4k policy | `ARCHITECTURE_INTRINSIC` with representation component; high risk to A→D and donor priority. |
| 7 | `rtl/dss_1x4/policy/...global_atomic_group_commit.v`, `:63-95` | shadow replay and owner reconstruction | 9.34k commit | `ARCHITECTURE_INTRINSIC`; high risk to atomicity/persistent mapping. |

Any future proposal must preserve candidate identity, implicit PatternID ordering,
policy ordering, C++↔RTL oracle equivalence, externally visible interface, and
documented cycle latency unless it is separately authorized to change them.

## Required conclusion

```text
SYN_B_PRODUCER_AREA: 133757.8720
SYN_C_PRODUCER_AREA: 953569.1171
SYN_D_PRODUCER_AREA: 974638.5353
SYN_B_NON_ANALYZER_PRODUCER_AREA: 64089.7498
SYN_C_NON_ANALYZER_PRODUCER_AREA: 814768.4230
SYN_D_NON_ANALYZER_PRODUCER_AREA: 833056.9709
MAIN_SYN_C_AREA_HOTSPOT: producer dynamic candidate-bank write/select logic
MAIN_SYN_D_AREA_HOTSPOT: impl/producer dynamic candidate-bank write/select logic
LARGEST_REGISTER_STRUCTURE: 1080-bit 1x4 candidate-record bank
LARGEST_COMBINATIONAL_STRUCTURE: producer-local variable-index select/write cone
MAJOR_WIDE_MUX_FOUND: YES
MAJOR_DUPLICATION_FOUND: YES (candidate summaries retained across all physical slots; no per-SA analyzer replication)
GLOBAL_SEARCH_IS_PRIMARY_1X4_AREA_CAUSE: NO
PRODUCER_AREA_CLASSIFICATION: MIXED, predominantly REPRESENTATION_DRIVEN
TOP_HUMAN_REVIEW_FILES:
1. rtl/dss_1x4/producer/recam_dss_line1x4_rs2_cs2_m1_normalized_candidate_producer.v
2. rtl/dss_1x4/producer/recam_dss_line1x4_rs2_cs2_m1_normalized_shared_analyzer.v
3. rtl/dss_1x4/policy/recam_dss_line1x4_rs2_cs2_m1_normalized_global_core.v
4. rtl/dss_1x4/policy/recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_core.v
5. rtl/dss_1x4/top/recam_dss_line1x4_rs2_cs2_m1_normalized_global_top.v
RTL_MODIFIED: NO
NEXT_PROPOSED_PHASE: HUMAN_RTL_REVIEW
```

The 1x4 producer overhead is **MIXED, predominantly
REPRESENTATION_DRIVEN**.  The ten-attempt/1x4 candidate contract, snapshot
lifetime, and 2R2C/3R2C/4R2C analysis are genuine architecture requirements.
However, the approximately 0.815–0.833M non-analyzer producer cost is almost
entirely combinational and converges on variable selection/writes of broad
flattened vectors, rather than being explained by its 1,080 retained bits or
the ~0.14M analyzer.  This is evidence for human design review, not authority
to optimize or alter the frozen semantics.
