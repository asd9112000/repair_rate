# P3-BL-IF — four-point RTL readiness

`EXISTS` means the implementation is present at the frozen integrated boundary.
`PARTIAL` means useful RTL exists but stops before that boundary. `MISSING`
means no synthesizable implementation of that role exists.

| Point | Existing RTL top | Candidate source | Policy RTL | Topology RTL | Ledger / commit | Oracle | Synth-ready |
|---|---|---|---|---|---|---|---|
| SYN-A 2x2 STREAMING_EARLY | **EXISTS** — `rtl/dss_canonical/top/recam_dss_canonical_rs2_streaming_early_top.v`, `recam_dss_canonical_rs2_streaming_early_top` | **EXISTS** — `rtl/recam/recam_shared_config_analyzer.sv`, `recam_shared_config_analyzer` | **EXISTS** — `rtl/dss_canonical/policy/early/recam_dss_canonical_streaming_early_core.v` | **EXISTS** — `rtl/dss_v2/topology/dss_topology_2x2_directional.sv` via `dss_v2_resource_feasibility` | **EXISTS** — `dss_v2_resource_ledger` in EARLY core | **PARTIAL** — prior focused RTL regressions; no P3 rerun | **EXISTS**, reference integrated boundary; no P3 synthesis run |
| SYN-B 2x2 GROUP_GLOBAL | **MISSING** — no analyzer-to-ledger top | **PARTIAL** — external `candidate_valid/release/borrow[159:0]` maps; no integrated producer | **EXISTS** — canonical GLOBAL no-scratch core; OPT1 wrapper exists separately | **PARTIAL** — fixed topology encoded inside core, not isolated adapter | **MISSING** — selected tuple has no atomic group commit to persistent ledger | **PARTIAL** — directed plus 1,000 seeded-map search equivalence, not top-level RTL | **NO** |
| SYN-C 1x4 STREAMING_EARLY | **MISSING** | **MISSING** | **MISSING** | **MISSING** | **MISSING** | **PARTIAL** — C++ `OneByFourSingleHopEarlyV1` and neighbor ledger | **NO** |
| SYN-D 1x4 GROUP_GLOBAL | **MISSING** | **MISSING** | **MISSING** | **MISSING** | **MISSING** | **PARTIAL** — C++ `OneByFourSingleHopGlobalV1`, recorded 1,000 randomized oracle evidence | **NO** |

## Dependencies behind the table

SYN-A wires `current_config_id_o` to `recam_shared_config_analyzer` and feeds
its lowest valid PatternID to the canonical EARLY core. The EARLY core uses
`dss_v2_group_slot_decode`, `dss_v2_resource_feasibility`,
`dss_topology_2x2_directional`, and `dss_v2_resource_ledger`.

SYN-B's existing core address is `SA*40 + action*10 + PatternID-1`; action
order is R, L, RB, B, followed by ascending PatternID. Its 480-bit history is
candidate/effect capture, not collector/analyzer integration. It outputs a
tuple and final search masks, but not a physical ledger commit.

No `rtl/dss_1x4/` family, 1x4 candidate producer, 1x4 topology legality
module, or 1x4 integrated top is present. C++ provenance must not be counted
as an RTL top, cycle contract, or synthesis evidence.

```text
SYN_A_CONTRACT_READY: YES
SYN_B_CONTRACT_READY: YES  (frozen implementation contract; RTL missing)
SYN_C_CONTRACT_READY: YES  (frozen implementation contract; RTL missing)
SYN_D_CONTRACT_READY: YES  (frozen implementation contract; RTL missing)
ALL_FOUR_IMPLEMENTATIONS_EXIST: NO
ALL_FOUR_SYNTHESIS_READY_TODAY: NO
```
