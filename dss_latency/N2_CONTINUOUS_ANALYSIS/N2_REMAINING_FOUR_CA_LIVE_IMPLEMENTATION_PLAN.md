# N2 remaining-four CA-LIVE implementation plan

## Scope and sequencing

This plan opens the four remaining N2 row-only DSS conversions after the
G2X2-RC CA-LIVE timing-sweep evidence commit `00c2c1c7a32a57ce2b85b56390f5152a4c5696ad`.
It is a planning record only: no remaining-four RTL, testbench, synthesis
runner, DC result, source freeze, or N3 work is created by this plan.

The target cases are:

| Case | Canonical source case | Policy/topology | CA-LIVE sibling target |
| --- | --- | --- | --- |
| G2X2 R EARLY | `rtl/G2X2_N2_R_EARLY/` | G2X2 row-only, immediate prefix commit | `rtl/G2X2_N2_R_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/` |
| G2X2 R GROUP | `rtl/G2X2_N2_R_GROUP_reg/` | G2X2 row-only, delayed static-global selection | `rtl/G2X2_N2_R_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/` |
| L1X4 R EARLY | `rtl/L1X4_N2_R_EARLY/` | directed A -> B -> C -> D, immediate prefix commit | `rtl/L1X4_N2_R_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/` |
| L1X4 R GROUP | `rtl/L1X4_N2_R_GROUP_reg/` | directed A -> B -> C -> D, delayed static-global selection | `rtl/L1X4_N2_R_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/` |

RECAM remains the canonical N2 baseline and is not a CA-LIVE conversion target.

## Frozen semantic boundary

Each sibling is an isolated policy-engine top.  It consumes the caller-owned,
serially selected live analyzer projection for one active SA; it does not
instantiate a collector or retain a four-SA analyzer-input bank.  The existing
ports total 272 bits:

```text
5 pivot-valid + 45 pivot-row + 65 pivot-column + 30 threshold
+ 7 Hybrid-valid + 21 Hybrid-pointer + 7 Hybrid-descriptor
+ 91 Hybrid-differing + 1 overflow = 272 bits
```

`state_update_i/state_sa_i` and `test_done_valid_i/test_done_sa_i` use the
same event-shaped serial scheduling contract as the closed G2X2-RC CA-LIVE
cases.  An update for `active_sa` has priority over a same-edge analyzer
result, EARLY acceptance, GROUP candidate-map write, freeze, and selection.
It resets that SA to its first canonical record; only the post-update live
projection may be evaluated afterwards.  A completed-word event may precede
or coincide with the final accepted result and is retained only for the active
SA.

The two existing R EARLY tops contain a historical comment calling their
analyzer boundary “204-bit”; the port declarations sum to 272 bits.  Future
implementation/spec work must use the declarations and update any stale
comment only as an explicitly reviewed source change; this plan makes no RTL
edit.

## Per-policy migration contract

### EARLY: G2X2 R and L1X4 R

Create a CA-LIVE controller by preserving each canonical controller's exact
rank order, configuration-ID mapping, prefix facts, and topology rule:

- G2X2 R retains the four frozen cross-edge implications represented by
  `a_released_q`, `a_borrows_q`, `b_borrows_q`, and `c_released_q`.
- L1X4 R retains only forward A -> B -> C -> D release/borrow legality and
  its endpoint slot restrictions.
- On a valid, legal result, enter HOLD rather than registering PatternID,
  selected configuration, analyzer input, or a solution payload.  With no
  update, the live projection remains authoritative; a same-active-SA update
  invalidates HOLD and restarts at the first rank.
- Preserve the existing selected-address mux and line reconstruction behavior.

The expected extra scheduling state is scan/HOLD control plus the one-bit
active-SA done-event memory.  This is control only; it must not duplicate
fault/CAM state.

### GROUP: G2X2 R and L1X4 R

Reuse the canonical `dss_v2_group_candidate_store` and
`dss_group_pivot_address_regs`; do not substitute the RC 80-bit store.
The R store is **60 bits**: four SAs times three physical canonical records,
each stored as `{PatternID[3:0], valid}`.  The action slot numbered 3 is a
selector alias of stored local slot 0.  Therefore the CA-LIVE scan restarts at
and overwrites only the three actual canonical records (0, 1, 2) for its active
SA.  It must not invent a fourth stored record, clear the map, or add a
generation tag.

For both GROUP cases:

- suppress the candidate-store write on an active-SA update edge;
- overwrite all three records, including an all-zero invalid record for an
  infeasible analyzer result, before freezing that SA;
- freeze only when the active SA has completed its latest three-record scan
  and its done event has been observed;
- retain already frozen earlier-SA map records and the required pivot
  reconstruction state;
- retain the final registered selection response rather than changing the
  canonical selection timing.

The topology-specific selectors are frozen dependencies: G2X2 R has its
fixed G2X2 priority/path relation, while L1X4 R has its directed selector
priority.  Their priority, aliases, donor metadata, and release/borrow
semantics are not shared interchangeably and will be lockstep-checked against
their own canonical case only.

## Required functional closure per case

No 20 ns synthesis starts until all relevant items pass:

1. directed canonical-order tests, including every canonical scan record;
2. canonical-versus-CA-LIVE semantic lockstep for the same stable live image;
3. fault update during scan and `FAULT_UPDATE_SAME_EDGE_AS_CONFIG_RESULT`,
   proving old-result preemption;
4. done-event-before-solution, solution-before-done, and same-edge final
   result/done ordering;
5. EARLY HOLD invalidation (EARLY only);
6. GROUP record-overwrite, no stale freeze, and final selector/reconstruction
   equivalence (GROUP only);
7. a strict readable-Verilog gate for the newly created RTL, plus the
   project-supported compile/simulation evidence; and
8. a case-specific source manifest and SHA256 freeze after functional closure.

The existing RC vector-27 directed check is RC prefix-law evidence.  It is not
automatically claimed as R-topology coverage; each R EARLY case needs its own
topology-specific directed legality vectors.

## Characterization and closure order

After a case's functional closure and exact source freeze, run one matched
20.0 ns DC characterization with the established N2 methodology only.  Record
area, NAND2X1-normalized GE, combinational/sequential area, cell counts,
WNS/TNS, critical path, and constraint warnings.  Do not run 10/15/25 ns for
a remaining case before all four 20 ns closures support the seven-case N2
table and the COMMITMENT, RESOURCE, and TOPOLOGY deltas.

Only then prepare `full_sixcase_timing_sweep/` and reuse a verified frozen 20
ns result where applicable.  N3 remains not started until the user authorizes
it after the required remaining-four functional and 20 ns closure conditions.
