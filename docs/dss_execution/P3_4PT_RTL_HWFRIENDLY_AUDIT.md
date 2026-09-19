# P3-4PT RTL Hardware-Friendliness Audit

## Scope and evidence order

```text
P3_4PT_RTL_HWFRIENDLY_AUDIT_STATUS: COMPLETE
MODE: READ ONLY
FROZEN BASELINE: c81e733 / source revision 93f6384
RTL_MODIFIED: NO
MANIFEST_MODIFIED: NO
TESTBENCH_MODIFIED: NO
SYNTHESIS_RERUN: NO
```

This review does not judge frozen DSS semantics by their area. Evidence is
ranked as: frozen architecture/policy, functional verification, actual DC
reports, RTL source, then skill guidance. A skill rule alone is not a PPA
finding.

## Skill used

```text
VERILOG_GENERATOR_SKILL_STATUS: AVAILABLE
VERILOG_GENERATOR_SKILL: readable-verilog-generator v2.0.0
SKILL_SOURCE: Eriemon/verilog-generator
SKILL_PATH: /home/asd9112000/.codex/skills/readable-verilog-generator
SKILL_DOCUMENTATION_READ: YES
ASIC_REFERENCE_READ: YES
ASIC_REFERENCE: references/rules/asic-verilog-quality.md
FPGA_SPECIFIC_GUIDANCE_USED_AS_ASIC_EVIDENCE: NO
```

Read documents: `SKILL.md`, `references/workflows/verilog_dispatcher.md`,
`references/rules/asic-verilog-quality.md`, and
`references/rules/verilog-quality-gates.md`. Applied ASIC guidance: preserve
contract, use complete combinational assignments, make control/enable cones
reviewable, keep widths explicit, and treat high-fanout control/dynamic packed
selection as review targets. The skill's public read-only CLI was invoked on
the candidate producer, but its Python 3.8 runtime stopped before AST analysis
at `dict[str, ...]`; that is a known skill-runtime limitation, not an RTL
failure. Vivado/xsim/Vitis/XPM/UNISIM and all FPGA resource advice were
excluded.

The skill's packed dynamic-lookup rule is useful context, not a violation
claim: its configured threshold is 1024 bits, while the individual 1×4 packed
banks are 180, 540, and 360 bits. Their aggregate is 1080 bits and their
combined dynamic write cone is still an ASIC review concern; no formal skill
gate result is fabricated.

## Files reviewed

- `rtl/dss_1x4/producer/recam_dss_line1x4_rs2_cs2_m1_normalized_candidate_producer.v`
- `rtl/dss_1x4/producer/recam_dss_line1x4_rs2_cs2_m1_normalized_shared_analyzer.v`
- `rtl/dss_1x4/policy/recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_core.v`
- `rtl/dss_1x4/policy/recam_dss_line1x4_rs2_cs2_m1_normalized_global_core.v`
- `rtl/dss_canonical/policy/global/recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_map_producer.v`
- frozen SYN-A/B/C/D hierarchy, timing, constraints, and closure reports.

## DC correlation baseline

| Point | Total cell area | Producer | Analyzer in producer | Producer excluding analyzer | Critical-path classification |
| --- | ---: | ---: | ---: | ---: | --- |
| SYN-B OPT1 | 395,369.256316 | 133,757.8720 | 69,668.1222 | 64,089.7498 | producer `sa_q` to candidate-release bank |
| SYN-C | 1,080,926.994646 | 953,569.1171 | 138,800.6941 | 814,768.4230 | producer `sa_q` to `candidate_used_rows_q` |
| SYN-D OPT1 | 1,267,025.771037 | 974,638.5353 | 141,581.5644 | 833,056.9709 | producer `sa_q` to `candidate_used_rows_q` |

SYN-C producer-local area is 755,066.1948 combinational plus 59,046.9274
sequential; SYN-D is 773,304.8466 plus 59,243.1850. Therefore the observed
1×4 excess is not principally a flip-flop count or GLOBAL DFS claim.

## Findings

### HF-01

```text
FILE: rtl/dss_1x4/producer/...candidate_producer.v
MODULE: recam_dss_line1x4_rs2_cs2_m1_normalized_candidate_producer
RTL LOCATION: lines 85-87
RTL CONSTRUCT: variable-base procedural writes into 180-, 540-, and 360-bit packed registers
SKILL GUIDANCE: dynamic packed selection requires resource-aware review; visible enable/data cones
CLASSIFICATION: HF-HIGH-RISK
ISSUE TYPE: REPRESENTATION
INFERRED HARDWARE: decoded write-position control plus data/hold muxing for 15-, 45-, and 30-bit slices; implementation may use per-slice or per-bit enables and D-input muxes
LIKELY PPA EFFECT: very large combinational area, select/control fanout, and endpoint timing cone
EXISTING DC EVIDENCE: direct: 755.1k/773.3k producer-local comb area and both critical paths end in candidate_used_rows_q
ARCHITECTURE REQUIRED: NO for this flattened variable-slice representation; retaining candidate summaries is required
CONFIDENCE: HIGH
POTENTIAL ISOLATED EXPERIMENT: synthesis-only equivalence-preserving representation A/B, after separate authorization
```

### HF-02

```text
FILE: rtl/dss_1x4/producer/...candidate_producer.v
MODULE: same
RTL LOCATION: lines 37-56
RTL CONSTRUCT: sa_q-based variable part-selects of four flattened collector snapshots
SKILL GUIDANCE: inspect dynamic selectors as mux/decode hardware, not merely legal syntax
CLASSIFICATION: HF-HIGH-RISK
ISSUE TYPE: REPRESENTATION
INFERRED HARDWARE: four-way selection across pivot, threshold, hybrid, and overflow bundles; widest selected field is 360-bit hybrid_differing
LIKELY PPA EFFECT: broad mux/input fanout before analyzer; contributes to sa_q-originating critical cone
EXISTING DC EVIDENCE: direct timing endpoint path starts at sa_q; hierarchy localizes the hotspot to producer, not analyzer alone
ARCHITECTURE REQUIRED: serial reuse of one analyzer is required; this packed-bundle selector representation is not proven required
CONFIDENCE: HIGH
POTENTIAL ISOLATED EXPERIMENT: compare explicit four-way selection versus current packed slices only under full timing/equivalence proof
```

### HF-03

```text
FILE: rtl/dss_1x4/producer/...candidate_producer.v
MODULE: same
RTL LOCATION: lines 34-36, 78, 82, 85-87
RTL CONSTRUCT: regular 180-slot candidate bank = 180 valid + 540 row-demand + 360 column-demand bits
SKILL GUIDANCE: separate retained state necessity from packed dynamic-access cost
CLASSIFICATION: HF-SUSPICIOUS
ISSUE TYPE: REPRESENTATION
INFERRED HARDWARE: 1080 retained bits, reset/start-clear fanout, and the HF-01 write network
LIKELY PPA EFFECT: measured ~59k sequential producer area plus unquantified portion of its large write cone
EXISTING DC EVIDENCE: direct sequential hierarchy area; 94 positions are architecturally usable, while the physical regular bank contains 180 positions and A/D attempt-2 plus lower-capacity tail slots remain zero
ARCHITECTURE REQUIRED: candidate retention across producer-to-policy/search handoff is YES; all physical slot allocation is not proven required
CONFIDENCE: HIGH
POTENTIAL ISOLATED EXPERIMENT: retain candidate identity/order but compare a semantically exact sparse/partitioned representation; not authorized here
```

### HF-04

```text
FILE: rtl/dss_1x4/producer/...shared_analyzer.v
MODULE: recam_dss_line1x4_rs2_cs2_m1_normalized_shared_analyzer
RTL LOCATION: lines 106-188
RTL CONSTRUCT: fixed-bound matrix, dictionary, hybrid, candidate, and nested 6x6 loops
SKILL GUIDANCE: analyze loop bounds and replicated operations; do not flag a loop by syntax alone
CLASSIFICATION: ARCHITECTURE-INTRINSIC
ISSUE TYPE: ARCHITECTURAL_REQUIREMENT
INFERRED HARDWARE: elaboration-time unrolled parallel comparison/matrix predicates; no runtime loop controller and no divider from loop indices
LIKELY PPA EFFECT: substantial combinational analyzer logic and reductions
EXISTING DC EVIDENCE: direct analyzer area 138.8k/141.6k, all combinational; it is only about 15% of 1x4 producer area
ARCHITECTURE REQUIRED: YES: six pivots, ten hybrids, and 2R2C/3R2C/4R2C PatternID evaluation are frozen
CONFIDENCE: HIGH
POTENTIAL ISOLATED EXPERIMENT: none before an analyzer-contract review; this is not a style-only optimization target
```

### HF-05

```text
FILE: rtl/dss_1x4/producer/...shared_analyzer.v
MODULE: same
RTL LOCATION: lines 44-88, 127-187
RTL CONSTRUCT: attempt_i-selected PatternID table and candidate-count decode, then 15 output lanes
SKILL GUIDANCE: runtime case/decode is a mux; static loop instances are parallel logic
CLASSIFICATION: ARCHITECTURE-INTRINSIC
ISSUE TYPE: ARCHITECTURAL_REQUIREMENT
INFERRED HARDWARE: attempt-class mux/decode feeding fixed candidate lanes and validity gating, not a runtime software-style loop
LIKELY PPA EFFECT: muxing and replicated candidate predicates; no evidence of accidental 32-bit candidate arithmetic
EXISTING DC EVIDENCE: analyzer is roughly 2x SYN-B's because 1x4 expands from five/seven to six/ten pivot/hybrid bounds and from <=10 to <=15 candidates
ARCHITECTURE REQUIRED: YES
CONFIDENCE: HIGH
POTENTIAL ISOLATED EXPERIMENT: only a contract-preserving encoding experiment after human review
```

### HF-06

```text
FILE: rtl/dss_1x4/policy/...streaming_early_core.v
MODULE: recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_core
RTL LOCATION: lines 71-100
RTL CONSTRUCT: 32-bit integer candidate_index and variable reads from 180/540/360-bit candidate inputs; two fixed eight-row scans
SKILL GUIDANCE: make widths explicit and inspect dynamic lookup/mux cones; fixed loops are unrolled
CLASSIFICATION: HF-SUSPICIOUS
ISSUE TYPE: REPRESENTATION
INFERRED HARDWARE: candidate-index muxes plus fixed eight-row own/donor priority chains. `line % 2` is evaluated with a constant unrolled loop index, so it does not imply a runtime modulo divider.
LIKELY PPA EFFECT: moderate candidate read mux and priority logic; current coding uses 32-bit intermediate arithmetic but DC may reduce it
EXISTING DC EVIDENCE: policy is 32,392.4834 total (26,488.1233 comb), far below producer; directed and 1000-case oracle evidence report zero mismatches
ARCHITECTURE REQUIRED: sequential first-legal scan and donor priority YES; 32-bit index/packed lookup encoding NO
CONFIDENCE: MEDIUM
POTENTIAL ISOLATED EXPERIMENT: report-only elaboration/netlist width query before any rewrite
```

### HF-07

```text
FILE: rtl/dss_1x4/policy/...global_core.v
MODULE: recam_dss_line1x4_rs2_cs2_m1_normalized_global_core
RTL LOCATION: lines 61-63, 153-247, 284-316
RTL CONSTRUCT: second 1080-bit candidate copy, variable depth/cursor reads, and OPT1 45-entry prior-candidate class comparison
SKILL GUIDANCE: state ownership and dynamic lookup must be separated from required DFS semantics
CLASSIFICATION: ARCHITECTURE-INTRINSIC
ISSUE TYPE: ARCHITECTURAL_REQUIREMENT
INFERRED HARDWARE: retained candidate handoff copy, four-depth ledger/cursor state, dynamic candidate read muxes, and when OPT1=1 a fixed 45-way compare/reduction cone
LIKELY PPA EFFECT: mixed sequential/combinational GLOBAL search cost
EXISTING DC EVIDENCE: SYN-D search is 181,721.2334 (102,419.8563 comb, 77,039.4250 seq), almost equal to SYN-B search 181,282.1494; it is not the 1x4 area explosion
ARCHITECTURE REQUIRED: DFS state and stable candidate lifetime YES; exact copy representation needs future interface/lifetime proof before classification as removable
CONFIDENCE: HIGH
POTENTIAL ISOLATED EXPERIMENT: none until a separate candidate-store ownership contract is authorized
```

### HF-08

```text
FILE: rtl/dss_1x4/policy/...global_core.v
MODULE: same
RTL LOCATION: lines 65-68, 166, 183-200, 211-247
RTL CONSTRUCT: integer indices, variable packed slices, and nested fixed scans
SKILL GUIDANCE: check signedness/width and actual elaboration; do not infer 32-bit hardware from an integer declaration alone
CLASSIFICATION: NEEDS-SYNTHESIS-CONFIRMATION
ISSUE TYPE: CODING_STYLE
INFERRED HARDWARE: likely constant-multiplier/add/index decode reduction, but exact surviving widths and cells are not reported below the search hierarchy
LIKELY PPA EFFECT: unknown incremental effect
EXISTING DC EVIDENCE: search hierarchy is healthy at 20 ns but lacks sub-cone attribution
ARCHITECTURE REQUIRED: NO for integer representation; YES for indexed DFS behavior
CONFIDENCE: LOW
POTENTIAL ISOLATED EXPERIMENT: report-only `report_timing`/netlist cone or an equivalence-preserving width-specialization A/B
```

### HF-09

```text
FILE: rtl/dss_canonical/policy/global/...candidate_map_producer.v
MODULE: recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_map_producer
RTL LOCATION: lines 82-95 and 162-164
RTL CONSTRUCT: same dynamic SA input selection and variable bank slices, but 3x160-bit maps over fixed 4 actions x 10 candidates
SKILL GUIDANCE: dynamic slices are legal but must be assessed from actual storage, decode, and timing evidence
CLASSIFICATION: HF-OK
ISSUE TYPE: REPRESENTATION
INFERRED HARDWARE: bounded snapshot mux plus map-write decode/hold muxes
LIKELY PPA EFFECT: real but contained control/datapath cost
EXISTING DC EVIDENCE: SYN-B non-analyzer producer is 64,089.7498, ~13x smaller than SYN-C/D; its critical path is analogous but area remains bounded
ARCHITECTURE REQUIRED: map retention YES; exact encoding is architecture-specific
CONFIDENCE: HIGH
POTENTIAL ISOLATED EXPERIMENT: none; this is a control comparison, not a copy target for 1x4
```

### HF-10

```text
FILE: 1x4 candidate producer and GLOBAL core
MODULE: producer / global_core
RTL LOCATION: producer lines 76-82; global core lines 258-316
RTL CONSTRUCT: full-bank reset and start-time clear of 1080-bit candidate state
SKILL GUIDANCE: keep reset/enable fanout visible; preserve confirmed reset and lifecycle contract
CLASSIFICATION: NEEDS-SYNTHESIS-CONFIRMATION
ISSUE TYPE: REPRESENTATION
INFERRED HARDWARE: reset/clear distribution and wide D-input zero-versus-data selection; the reset is synchronous because it is inside `always @(posedge clk_i)`
LIKELY PPA EFFECT: possible area/fanout contribution beyond the measured flip-flop area
EXISTING DC EVIDENCE: producer sequential area is ~59k and SYN-C/D have max-cap but no named reset-net attribution; no direct reset-cone report
ARCHITECTURE REQUIRED: clearing stale candidates between operations YES; implementation cost attribution UNKNOWN
CONFIDENCE: MEDIUM
POTENTIAL ISOLATED EXPERIMENT: report-only reset/enable fanout inspection first
```

## Ranked summary

| ID | RTL construct | Classification | Inferred hardware | DC correlation | PPA risk | Architecture-required? | Confidence |
| --- | --- | --- | --- | --- | --- | --- | --- |
| HF-01 | producer variable bank writes | HF-HIGH-RISK | write decode + hold/data mux network | exact producer comb hotspot and endpoint | Very high | No, representation only | High |
| HF-02 | `sa_q` snapshot selection | HF-HIGH-RISK | wide four-way mux/fanout | exact critical-path start and producer hierarchy | High | serial reuse yes; packing no | High |
| HF-03 | 180-slot candidate bank | HF-SUSPICIOUS | 1080 FF bits plus clear/write structures | ~59k seq, supports HF-01 cone | High | retention yes; physical slots uncertain | High |
| HF-04 | analyzer fixed loops | ARCHITECTURE-INTRINSIC | unrolled matrix/comparator predicates | 138.8k/141.6k analyzer | Medium | Yes | High |
| HF-07 | GLOBAL candidate copy/DFS | ARCHITECTURE-INTRINSIC | second bank, lookup muxes, 45-way OPT1 check | 181.7k, near 2x2 GLOBAL | Medium | Yes, pending store proof | High |
| HF-06 | EARLY candidate lookup/row scan | HF-SUSPICIOUS | lookup mux + fixed priority scan | 32.4k policy | Medium | scan yes; index encoding no | Medium |
| HF-10 | full reset/start clear | NEEDS-SYNTHESIS-CONFIRMATION | clear fanout and D muxes | no reset-cone report | Medium | clear semantics yes | Medium |
| HF-08 | GLOBAL integer/index widths | NEEDS-SYNTHESIS-CONFIRMATION | implementation-dependent | no sub-cone report | Low/unknown | behavior yes; type no | Low |
| HF-09 | 2x2 dynamic map bank | HF-OK | bounded mux/decode | 64.1k non-analyzer producer | Low comparative | yes | High |

## 2×2 versus 1×4 representation

The shared syntactic patterns—dynamic SA selection and packed slice writes—do
not by themselves prove poor RTL: SYN-B uses them with a 480-bit map and costs
64.1k excluding analyzer.  1×4 must support ten capacity attempts, six pivots,
ten hybrids, local R/C demand metadata, and neighbor-owner state; these are
architecture-intrinsic. The confirmed representation-driven concern is how the
1×4 producer realizes its 1080-bit regular candidate bank and broad dynamic
read/write/select cones, not its required candidate semantics.

## RTL correctness and non-findings

No new RTL correctness contradiction was found. Existing focused regressions,
1000-case C++↔RTL corpora, and exact-top Verilator lint are the relevant
functional evidence; this review does not replace them.

- Analyzer loops have fixed elaboration bounds (`6`, `15`, or fixed
  `HYBRID_ENTRIES=10`) and synthesize as replicated combinational logic; they
  are not sequential runtime loops.
- The fixed eight-row EARLY/GLOBAL scans are unrolled priority/allocation
  logic. They preserve frozen donor priority; they are not runtime `%` logic.
- No raw gated clock, delay, simulation task, or multiple-clock-domain issue
  was found in reviewed files.
- No claim is made that `integer` creates a 32-bit hardware multiplier; exact
  post-optimization width needs a report-only synthesis/netlist query.

## Top hardware-friendliness concerns

1. **HF-01 dynamic candidate-bank writes** — high confidence; direct DC area
   and timing correlation; safe only as a separately authorized,
   equivalence-proven representation experiment.
2. **HF-02 `sa_q`-selected flattened snapshots** — high confidence; directly
   aligned with critical-path start; independently testable only with exact
   cycle/semantic equivalence.
3. **HF-03 regular 180-slot / 1080-bit candidate bank** — high confidence;
   direct sequential-area and lifecycle evidence; slot-layout alternatives
   require identity/order and latency preservation proof.
4. **HF-04 unrolled 1×4 analyzer** — high confidence and DC-supported, but
   architecture-intrinsic; not safe to reduce without reauthorizing the
   capacity/analyzer contract.
5. **HF-07 GLOBAL copy/indexed DFS** — high confidence but not primary cause;
   safe experimentation is blocked until candidate-store ownership/lifetime is
   separately frozen.

```text
REPRESENTATION_DRIVEN_COST: CONFIRMED
ARCHITECTURE_INTRINSIC_COST_IDENTIFIED: six-pivot/ten-hybrid analyzer, ten capacity requests, required candidate lifetime, neighbor ledger, and GLOBAL DFS semantics
FIRST_RECOMMENDED_ISOLATED_HYPOTHESIS: HYP-01 — retain exact 1x4 candidate identity, PatternID order, ten-cycle producer schedule, and 1080-bit externally observable payload; compare only an alternative producer bank write/select representation, with full C++↔RTL and DC A/B evidence
```

HYP-01 is a future human-reviewed experiment, not a recommendation to modify
this baseline.

## Explicit non-actions

No RTL, testbench, source manifest, constraint, synthesis script, existing
result, interface, latency, PatternID ordering, EARLY ordering, GLOBAL
objective, topology, or candidate semantics was changed. No automatic
refactoring, DC rerun, or optimization phase was started.

```text
CHECKS_RUN: startup repository guard; source/hierarchy/timing/constraint inspection; readable-verilog-generator read-only CLI attempted (blocked by Python 3.8 type-annotation incompatibility); git diff --check pending final document check
MISMATCHES: none discovered; no functional corpus rerun in this read-only phase
NEXT_ACTION: STOP. Return findings for human review. Do not begin RTL optimization.
```
