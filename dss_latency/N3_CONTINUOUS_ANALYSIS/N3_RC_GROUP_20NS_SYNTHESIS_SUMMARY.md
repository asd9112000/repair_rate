# N3 RC-GROUP CA-LIVE — matched 20 ns synthesis

## Result

`20NS_SETUP: FAILED`

The frozen N3 production design was synthesized once at 20.0 ns.  The final raw DC reports establish a 44.23 ns critical path with −24.41 ns slack; no RTL, constraint, or methodology adjustment was made after this result.

| Metric | Final result |
|---|---:|
| Total cell area | 685591.004318 um² |
| GE (`area / 9.979200`) | 68702.0006 |
| Combinational area | 640132.420814 um² |
| Sequential area | 45458.583504 um² |
| Total cells (`report_area`) | 27448 |
| Leaf cells (`report_qor`) | 27419 |
| Combinational / sequential cells | 26617 / 802 |
| DESIGN_WNS_NS | −24.41 |
| TNS_NS | −2736.25 |
| Timing-violating paths | 119 |
| CRITICAL_PATH_NS | 44.23 |
| CRITICAL_PATH_SLACK_NS | −24.41 |
| MAX_CAP_VIOLATIONS | 251 |
| MAX_TRANSITION_VIOLATIONS | 0 |
| MAX_FANOUT_VIOLATIONS | 155 |
| Nets with any design-rule violation | 268 |

`report_qor` prints Design WNS as the unsigned magnitude `24.41`; this summary records the signed setup value from the timing/constraint report: `−24.41 ns`.  Design WNS and critical-path slack are therefore separately named even though they have the same signed value for this failing worst path.

## Critical path

- Startpoint: `core/active_sa_q_reg[1]`
- Endpoint: `core/candidate_store/store_q_reg[12]`
- Classification: `mixed`
- Rationale: the path crosses controller state (`core`), `analyzer_config`, the `analyzer`, then terminates in the candidate store.  The hierarchical report attributes 81.3% of total cell area to the analyzer, but that area fact does not by itself reclassify this mixed timing path as analyzer-only.

## Hierarchical area attribution

`AREA_ATTRIBUTION: PROVEN` from `report_area -hierarchy`; entries overlap at parent/child levels and must not be summed.

| Hierarchical block | Global cell area (um²) |
|---|---:|
| analyzer | 557601.1095 |
| core (controller parent) | 61624.8867 |
| core/candidate_store | 48399.1201 |
| core/static_selector | 5831.1793 |
| pivot_address_regs (reconstruction) | 65280.6017 |

## Provenance

- DC: Synopsys Design Compiler W-2024.09-SP2
- Library/corner: TSMC018 `slow.db` / `slow`
- Clock: 20.0 ns; I/O delay: 0.0 ns input and output
- Other accepted N2 CA-LIVE mechanics: `set_load 0.05`, `tsmc18_wl10`, max transition 3.0, max capacitance 0.15, max fanout 10
- Compile command: `compile -map_effort low` (DC OPT-1303 effective medium)
- Runner: `provenance/run_n3_rc_group_ca_live_20ns.tcl`, SHA-256 `6373fbe67175b4fb5632ec9f07db5282084a5fcbb5957b05028facb0f61033c8`
- Frozen source manifest: `provenance/N3_RC_GROUP_SYNTHESIS_SOURCE_MANIFEST.txt`
- Hash gates: `SYNTHESIS_SOURCE_HASH_MATCH: PASS`; `POST_DC_SOURCE_HASH_MATCH: PASS`
- Raw reports: `../../results/date2026/n3_rc_group_ca_live_20ns/synthesis/by_period/20ns/{area,qos,timing,constraints,hierarchy_area}.rpt`

The source freeze is evidence-level only; it is not a Git commit.  No commit, other clock period, N3 R-GROUP implementation, or priority study was created in this phase.
