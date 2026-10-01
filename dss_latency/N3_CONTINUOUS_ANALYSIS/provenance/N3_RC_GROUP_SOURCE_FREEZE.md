# N3 RC-GROUP CA-LIVE production source freeze

Status: `SOURCE_FREEZE: PASS` (evidence-level snapshot, not a Git commit).

## Git provenance

- Worktree: `/home/asd9112000/repair_rate_date2026_n3`
- Branch: `integration/date2026-n3-ca-live-scaling`
- Base HEAD: `a2a61b96280275019ab06f9e120c4951466b9777`
- Initial status: untracked `dss_latency/N3_CONTINUOUS_ANALYSIS/`, `rtl/G2X2_N3_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/`, and `tb/n3_continuous_analysis/`.
- Production source count: 7 SV files.  Exact paths, roles, and hashes are in [N3_RC_GROUP_SYNTHESIS_SOURCE_MANIFEST.txt](N3_RC_GROUP_SYNTHESIS_SOURCE_MANIFEST.txt) and [N3_RC_GROUP_SOURCE_SHA256.txt](N3_RC_GROUP_SOURCE_SHA256.txt).

## Structural audit

`PRE_FREEZE_STRUCTURAL_AUDIT: PASS`

The frozen top is `recam_dss_n3_rc_group_live_state_top` in `rtl/G2X2_N3_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_n3_rc_group_live_state_top.sv`.  It instantiates exactly one `recam_n3_rc_fixed_mask_analyzer`, one CA-LIVE controller, one candidate store, one selector, one slot decoder, and one retained-pivot reconstruction block.

| Contract item | Evidence in frozen source | Result |
|---|---|---|
| Topology / sharing | G2X2 directional; RS=3, CS=3, SHARE_M=1 slot/config map | PASS |
| Physical addresses | top defaults `ROW_ADDR_W=9`, `PHYS_COL_ADDR_W=13`, `HYBRID_LINE_ADDR_W=13` | PASS |
| N3 sizing | seven pivots, `HYBRID_ENTRIES=17`, 6-bit PatternID | PASS |
| Per-SA policy | four scan slots/config evaluations; 4-SA GROUP decision | PASS |
| Analyzer payload | `7 + 63 + 91 + 28 + 28 + 17 + 51 + 17 + 221 + 1 = 524` live-input bits | PASS |
| Retention | 112-bit candidate store; 656-bit reconstruction retained state; 616-bit private pivot capture contract | PASS |
| Forbidden state | no runtime nth-pattern search, duplicate analyzer state bank, or generation tags in the production source set | PASS |

The analyzer payload accounting is: pivot-valid `7`, pivot-row addresses `7*9=63`, pivot-column addresses `7*13=91`, row `gt1..gt4` `4*7=28`, column `gt1..gt4` `4*7=28`, hybrid-valid `17`, hybrid pointers `17*3=51`, hybrid descriptors `17`, hybrid differing addresses `17*13=221`, and overflow `1`.

## Synthesis boundary and source lock

The synthesis source set excludes C++ shared oracles, all testbenches, the latency harness/vector generator, documentation, historical RS3 reference RTL, and CA-SB prototypes.  Production RTL must remain byte-identical from this freeze through the post-DC hash check.  Any RTL change invalidates this freeze.

The matched runner is [run_n3_rc_group_ca_live_20ns.tcl](run_n3_rc_group_ca_live_20ns.tcl).  It uses the canonical N2 CA-LIVE methodology: DC W-2024.09-SP2, TSMC018 `slow.db`, slow operating condition, 20.0 ns clock, zero input/output delay, `set_load 0.05`, wire-load model `tsmc18_wl10`, and `compile -map_effort low`.

## Hash gates and executed run

- `SYNTHESIS_SOURCE_HASH_MATCH: PASS` immediately before the successful DC invocation.
- `POST_DC_SOURCE_HASH_MATCH: PASS` after DC exited.  The seven production RTL hashes equal the pre-freeze snapshot.
- Runner SHA-256: `6373fbe67175b4fb5632ec9f07db5282084a5fcbb5957b05028facb0f61033c8`.
- DC: W-2024.09-SP2; accepted requested command `compile -map_effort low` (DC OPT-1303 reports effective medium effort).
- Raw results: `results/date2026/n3_rc_group_ca_live_20ns/synthesis/by_period/20ns/`.
- The first invocation stopped before elaboration because the new TCL had two incorrect core/top filenames.  It produced no synthesis evidence.  The runner-only path correction above was made, the source hash gate was repeated, and the second invocation completed.  No production RTL changed.
