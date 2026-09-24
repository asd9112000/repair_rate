# G2X2 N2 GROUP 7CFG Parallel-DFS Pre-Optimization Provenance Audit

```text
CASE_ID: G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg
LIFECYCLE: SUPPORTING_ABLATION (implemented; pre-synthesis)
AUDIT_STATUS: PROVENANCE_RECOVERED_AND_PORTED
RTL_MODIFIED: YES (separate ablation case only)
CANONICAL_RTL_MODIFIED: YES (separate ablation case only)
SYNTHESIS_RUN: NO
```

## Purpose

This audit began before creation; the subsequent SYN-B recovery resolved the dense-map blocker and authorized this supporting ablation.
The ablation must retain the final N2 directed G2X2 RC GROUP external repair
semantics while measuring an earlier seven-Config parallel analyzer and dense
GROUP-DFS/map implementation.  The requested dense map is explicitly
`3 x 4 x 4 x 10` (480 logical entries); its four dimension meanings may not
be invented.

## Canonical mother and immutability check

The functional mother is `rtl/G2X2_N2_RC_GROUP_reg/`, archived at
`dss_final/G2X2_N2_RC_GROUP_reg/`.  Its working and archive sources have
identical SHA-256 hashes for all nine SystemVerilog files at audit time.
The current canonical implementation uses:

- one shared combinational analyzer (`recam_shared_config_analyzer.sv`),
- sequential current-SA collection in
  `recam_dss_hyp02_static_global_core.sv`,
- an 80-bit four-SA-by-four-slot candidate store, and
- an 81-path fixed-priority static selector.

It is therefore the optimized/compact reference, not the requested dense
pre-optimization implementation.

## Historical source evidence

The historical DATE 2x2 implementation introduced at commit `3304c86`
(`DATE 2x2 direction rtl done`) remains present under
`rtl/dss_2x2/analyzer/`.

| Historical source | Proven fact | Reuse status |
|---|---|---|
| `multi_config_analyzer_bank.sv` | Generates one `config_analyzer` for each ConfigID 0 through 6.  The seven analyzers receive the same currently collected SA state and are simultaneously synthesis-visible. | Evidence for seven current-SA lanes only. |
| `dss_analyzer_top.sv` | Captures the seven results for `subarray_i` into `config_result_bank`; collection/capture remains scheduled per SA. | Evidence against inferring 4 SA x 7 = 28 simultaneous analyzers. |
| `config_result_bank.sv` | Retains `NUM_SA * NUM_CFG * PATTERN_ID_W = 4 * 7 * 4 = 112` PatternID bits. | Historical result representation, not a 480-entry map. |
| `group_selector.sv` | Iterates a `7^4` ConfigID tuple space, chooses minimum borrow then lexicographic ConfigID order, and accepts any ConfigID for every SA. | Not semantics-equivalent to final N2 directed GROUP. |

The seven-lane evidence establishes the requested analyzer-count boundary:

```text
PARALLEL_CONFIG_ANALYZERS_PER_CURRENT_SA: 7
CONFIG_TIME_MULTIPLEXING: NO
SA_SCHEDULING: PRESENT (current SA is collected/captured before the next SA)
28_ANALYZER_HISTORICAL_REQUIREMENT: NO EVIDENCE
```

No `dont_touch`, `keep`, or preservation directive was found in the cited
historical lane bank; the seven lanes are naturally live because each feeds
the captured ConfigPatternMap.

## Dense-map provenance result

The full current repository and reachable Git history were searched for the
literal requested representation and its named variants: `3x4x4x10`,
`3 x 4 x 4 x 10`, `ConfigValidMap`, `PatternMap`, and dense GROUP DFS/map
forms.  No RTL structure, commit, or design note defines a `3 x 4 x 4 x 10`
map or assigns meanings to its dimensions.

The closest historical structures do **not** establish the requested four
dimensions:

| Structure | Actual shape/behavior | Why it is not the requested map |
|---|---|---|
| `multi_config_analyzer_bank` | 7 parallel ConfigID analyzers, each with up to 10 candidate patterns | It has no `3 x 4 x 4 x 10` state or GROUP map index. |
| `config_result_bank` | 4 SAs x 7 ConfigIDs x 4-bit PatternID | It retains 112 bits, not 480 dense entries. |
| `multi_config_group_selector` | Sequential enumeration of 7^4 ConfigID tuples | It has no 3-dimensional action/role map and does not retain Pattern candidate dimension 10. |
| canonical GROUP selector | 4 SAs x 4 legal slots with 81 fixed legal tuples | It is the compressed final representation explicitly excluded by the ablation contract. |

Consequently the following required record cannot truthfully be completed:

```text
DIM0 = UNKNOWN -- no authoritative source
DIM1 = UNKNOWN -- no authoritative source
DIM2 = UNKNOWN -- no authoritative source
DIM3 = UNKNOWN -- no authoritative source
TOTAL = 480 -- requested arithmetic only; not an evidenced RTL map
```

## Semantic-equivalence result

The historical selector is unsuitable as the primary source unchanged:

```text
HISTORICAL_ROLE_LEGALITY:
  every SA may select ConfigID 0..6
FROZEN_N2_ROLE_LEGALITY:
  A/D = {0,1,2,3}
  B/C = {0,4,5,6}
SEMANTICS_EQUIVALENT_ABLATION: NO (if historical selector is reused unchanged)
```

Porting the historical lane bank alone could preserve the seven-lane current-SA
boundary, but choosing meanings for the missing 480-entry map or creating a
new dense DFS would be a new architecture, not a provenance-backed
pre-optimization reference.  It also cannot establish the requested
optimization-only semantic comparison.

## Superseding recovery and decision

The former blocker is superseded by the recovered SYN-B producer, OPT1 core, canonical GLOBAL DFS core, and atomic commit. The port uses current canonical role legality and 13-bit physical columns, so it is documented as a provenance-backed composite rather than a literal snapshot.

```text
DIM0 = effect map {valid, release, borrow}
DIM1 = SA 0..3
DIM2 = stored action {L,R,B,RB}
DIM3 = PatternOffset 0..9
TOTAL = 3 * 4 * 4 * 10 = 480 registered bits
```

## Historical decision record

```text
STATUS: IMPLEMENTED_PRE_SYNTHESIS
DENSE_MAP_PROVENANCE: RECOVERED
FUNCTIONAL_RTL_CREATED: YES (separate supporting-ablation path)
FOCUSED_VERILATOR: PASS
SYNTHESIS: NOT RUN
CANONICAL_GROUP_SYNTHESIZABLE_RTL_CHANGED: NO
```

The remaining acceptance work is full canonical lockstep and synthesis review;
no archive, records, or canonical-case index is changed beforehand.
