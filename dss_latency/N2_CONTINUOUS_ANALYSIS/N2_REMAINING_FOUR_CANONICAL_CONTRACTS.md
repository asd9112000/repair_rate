# N2 remaining-four canonical CA-LIVE contracts

This extraction is taken from the frozen canonical RTL before any CA-LIVE
sibling is written.  It is the implementation authority for the scheduling
transformation; values are not imported from the G2X2-RC cases.

## G2X2 N2 R EARLY

| Field | Frozen contract |
| --- | --- |
| Topology | G2X2 row-only; cross-edge implications `C.borrow -> A.release`, `B.borrow -> D.release`, `A.borrow -> B.release`, and `D.borrow -> C.release` |
| Action slots | local=0, release=1, borrow=2, release-borrow=3 |
| Dense analyzer configs | 3 (`slot 0/3 -> config 0`, `slot 1 -> config 4`, `slot 2 -> config 2`) |
| External actions | 4; release-borrow retains action identity although it shares config 0 with local |
| Candidate store / reconstruction | none / none |
| Pattern ID | 4 bits |
| Config priority | slots `1, 0, 3, 2` (release, local, release-borrow, borrow) |
| Old controller schedule | scan ranks for one SA; first valid prefix-compatible result commits immediately, then advance A -> B -> C -> D |

Sources: `recam_dss_g2x2_r_static_early_core.sv` and
`recam_dss_g2x2_r_static_early_top.sv`.

## G2X2 N2 R GROUP

| Field | Frozen contract |
| --- | --- |
| Topology | G2X2 row-only static-global |
| Action slots | local=0, release=1, borrow=2, release-borrow=3 |
| Dense analyzer configs | 3 (`record 0 -> config 0`, `record 1 -> config 4`, `record 2 -> config 2`) |
| External actions | 4; slot 3 is an alias of stored local record 0, not a fourth record |
| Candidate store | 60 bits = 4 SA x 3 records x `{PatternID[3:0], valid}` |
| Pattern ID | 4 bits |
| Static path count / priority | 81; explicit `path_valid[0]` through `path_valid[80]`, lowest numbered valid path wins |
| Reconstruction retention | 440 bits (`dss_group_pivot_address_regs`) |
| Old controller schedule | scan records 0,1,2 for each SA A -> B -> C -> D; then one DECIDE response |

Sources: `recam_dss_g2x2_r_static_global_core.sv`,
`recam_dss_g2x2_r_static_selector.sv`, and `dss_v2_group_candidate_store.sv`.

## L1X4 N2 R EARLY

| Field | Frozen contract |
| --- | --- |
| Topology | directed A -> B -> C -> D, single-hop row sharing only |
| Action slots | local=0, release=1, borrow=2, release-borrow=3 |
| Role-legal slots | A: local/release; B and C: all; D: local/borrow |
| Dense analyzer configs | 3 (`slot 0/3 -> config 0`, `slot 1 -> config 4`, `slot 2 -> config 2`) |
| External actions | 4; release-borrow shares config 0 with local |
| Candidate store / reconstruction | none / none |
| Pattern ID | 4 bits |
| Config priority | slots `1, 0, 3, 2`; endpoint masks reject illegal actions |
| Old controller schedule | four priority ranks are serviced per SA; first valid, slot-allowed, prefix-compatible result commits immediately in A -> B -> C -> D order |

Sources: `recam_dss_l1x4_r_static_early_core.sv` and
`recam_dss_l1x4_r_static_early_top.sv`.

## L1X4 N2 R GROUP

| Field | Frozen contract |
| --- | --- |
| Topology | directed A -> B -> C -> D row-only static-global |
| Action slots | local=0, release=1, borrow=2, release-borrow=3 |
| Dense analyzer configs | 3 (`record 0 -> config 0`, `record 1 -> config 4`, `record 2 -> config 2`) |
| External actions | 4; slot 3 aliases stored local record 0 |
| Candidate store | 60 bits = 4 SA x 3 records x `{PatternID[3:0], valid}` |
| Pattern ID | 4 bits |
| Static path count / priority | 256 action tuples; nested fixed priority is `d`, then `c`, then `b`, then `a`, with the earliest valid tuple retained |
| Reconstruction retention | 440 bits (`dss_group_pivot_address_regs`) |
| Old controller schedule | scan records 0,1,2 for each SA A -> B -> C -> D; then one DECIDE response |

Sources: `recam_dss_l1x4_r_static_global_core.sv`,
`recam_dss_l1x4_r_static_selector.sv`, and `dss_v2_group_candidate_store.sv`.

## Shared CA-LIVE boundary

All four canonical wrappers expose the same 272-bit analyzer projection:
`5 + 45 + 65 + 30 + 7 + 21 + 7 + 91 + 1`.  CA-LIVE adds only the active-SA
fault-update and done-event scheduling ports.  It retains one analyzer,
excludes upstream fault/CAM storage from synthesis, and must not introduce a
four-SA copied-state bank, a map clear, or generation tags.
