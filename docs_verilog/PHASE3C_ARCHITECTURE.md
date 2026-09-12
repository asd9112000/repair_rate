# Phase 3C Role-Aware Config Scheduler and ConfigPatternMap

## Authorized boundary

Phase 3C adds sequential control around the frozen, fully combinational Phase
3B `recam_shared_config_analyzer`.  It enumerates and records candidate repair
results only.  It does not add EARLY, GROUP, PhysicalResourceLedger, ownership
arbitration, repair allocation, or CAM-state snapshot hardware.

The implementation is `recam_role_aware_config_scheduler`.  Its only Phase 3B
instance remains combinational and ConfigID-controlled; no analyzer pipeline,
candidate serialization, or analyzer semantic/datapath change is introduced.

## SA role and universal ConfigID contract

`sa_role_i` is latched when `start_i` is accepted:

| `sa_role_i` | Physical SA role | Fixed scan order |
|---:|---|---|
| 0 | A/D | CFG0 (2R2C), CFG4 (1R2C), CFG5 (2R3C), CFG6 (1R3C) |
| 1 | B/C | CFG0 (2R2C), CFG2 (3R2C), CFG1 (2R1C), CFG3 (3R1C) |

The universal three-bit encoding remains frozen: CFG0=2R2C, CFG1=2R1C,
CFG2=3R2C, CFG3=3R1C, CFG4=1R2C, CFG5=2R3C, CFG6=1R3C, and CFG7 is invalid.
Each SA records precisely four physically reachable configurations; it neither
scans nor allocates a seven-entry map.

## Controller behavior and timing

The controller uses one `busy` bit and a two-bit scan counter, rather than a
one-hot FSM.  Its conceptual states are IDLE, SCAN0, SCAN1, SCAN2, SCAN3, and
the one-cycle DONE indication.

- Reset (`rst_ni=0`): clears `busy_o`, `done_o`, counter, role latch, and all
  ConfigPatternMap valid bits/data.
- Idle start: `start_i=1` clears the prior map, latches the role, and begins
  SCAN0.
- Start while busy: ignored; neither role, counter, nor captured map entry is
  changed.
- Each SCAN interval presents one ConfigID to Phase 3B combinational logic;
  the result is captured at its ending rising edge.
- The fourth capture clears `busy_o` and asserts `done_o` for one cycle.

There are exactly four analyzer evaluation cycles per SA.  With this interface,
the accepting start edge is E0 and result captures occur at E1, E2, E3, and
E4; `done_o` is asserted after E4.  Thus request acceptance to done is four
clock intervals (five named edges including E0), with no hidden setup/evaluate
cycle beyond those four intervals.

## ConfigPatternMap

`config_pattern_map_o` holds four 20-bit entries (80 bits total), with entry
0 in bits `[19:0]`.  Every capture sets the entry valid bit, including an
unrepairable result.

| Entry bits | Field | Bits |
|---|---|---:|
| `[0]` | valid | 1 |
| `[3:1]` | universal ConfigID | 3 |
| `[4]` | solution_valid | 1 |
| `[5]` | repairable | 1 |
| `[9:6]` | PatternID | 4 |
| `[19:10]` | full candidate_valid bitmap | 10 |

The current combinational Phase 3B result is also exposed through
`current_config_id_o`, `current_candidate_valid_o`, `current_pattern_id_o`,
`current_solution_valid_o`, and `current_repairable_o`.  These are observation
ports for integration/verification; the registered map is the transaction
result.  Unused candidate slots remain zero exactly as Phase 3B defines them.

## Input ownership and stability contract

**The collector state is held stable while the role-aware scheduler evaluates
the four configurations of one subarray. No analyzer-state snapshot is
required.** The collector owns Pivot/Hybrid/Must/overflow state during fault
collection.  After `collection_done` for an SA, it is read-only through E4;
after `done_o`, it may clear, reuse, or advance for the next SA.

```text
FAULT COLLECTION → collector-state freeze → SCAN0 → SCAN1 → SCAN2 → SCAN3
                                                                    ↓
                                                          Phase 3C done
                                                                    ↓
                                                collector may clear/reuse/advance
```

This is a frozen interface contract, not a temporary workaround.  Phase 3C
excludes full Pivot/Hybrid snapshots, double buffering, and shadow CAM: these
are unnecessary under the frozen-state contract and would artificially inflate
area.  They are reconsidered only if collector/analyzer concurrency is later
authorized. If upstream cannot provide the guarantee, integration must stop
and define an explicit snapshot ownership boundary first.

Phase 3C has intentional sequential cells in its scheduler/map.  The frozen
Phase 3B analyzer remains independently required to contain zero sequential
cells.  Phase 3C synthesis is not authorized in this phase.
