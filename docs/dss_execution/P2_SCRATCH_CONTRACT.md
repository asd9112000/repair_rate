# P2-SCRATCH — Normalized GLOBAL-WithScratch Storage Contract

> Date: 2026-09-18
> Scope: read-only architecture, state-lifetime, and storage-interface audit.
> Production GLOBAL-WithScratch RTL: not started.
> Decision: contract closed with a negative evidence result; no storage reuse is authorized.

## Decision

```text
P2_SCRATCH_STATUS: CLOSED
SCRATCH_CONTRACT_DECISION: BLOCKED_NOT_ENOUGH_EVIDENCE
GLOBAL_WITHSCRATCH_RTL_STARTED: NO
```

No current storage instance is proven eligible to hold the candidate/effect
history of Normalized GLOBAL-WithScratch.  In particular, capacity by itself
does not establish reuse: each candidate needs an ownership transfer, a legal
overwrite event, suitable indexed access, and a boundary connection to the
canonical GLOBAL controller.  None is frozen for an existing source.

Normalized GLOBAL-WithScratch therefore remains **not justified**.  This is
not a policy failure and does not authorize a second GLOBAL implementation.

## Frozen Normalized GLOBAL semantics

The audited source is
`rtl/dss_canonical/policy/global/recam_dss_canonical_global_noscratch_core.v`.
It remains the only policy reference: A -> B -> C -> D traversal, R -> L ->
RB -> B action priority, ascending PatternID, deterministic donor selection,
complete bounded DFS/backtracking, explicit future-donor obligations, and a
final-only committed ledger.  Storage may not alter any of these rules.

## Current NoScratch state

The core captures three 160-bit candidate arrays on accepted `start_i`:

| State class | Bits | Source evidence |
|---|---:|---|
| Candidate valid history | 160 | `candidate_valid_q` |
| Actual-release history | 160 | `candidate_release_q` |
| Actual-borrow history | 160 | `candidate_borrow_q` |
| Candidate/effect history total | 480 | three arrays above |
| DFS snapshots | 56 | four each of released[3:0], used[3:0], release_req[3:0], borrow_count[1:0] |
| DFS cursors | 24 | four 6-bit cursors |
| FSM/depth/status | 6 | state, depth, done, repairable |
| Selected tuple and final masks | 64 | selected fields plus final released/used masks |
| Other local state total | 150 | all non-history registered state |
| Registered state total | 630 | 480 + 150 |

```text
GLOBAL_NOSCRATCH_HISTORY_VALID_BITS: 160
GLOBAL_NOSCRATCH_HISTORY_RELEASE_BITS: 160
GLOBAL_NOSCRATCH_HISTORY_BORROW_BITS: 160
GLOBAL_NOSCRATCH_TOTAL_HISTORY_BITS: 480
GLOBAL_OTHER_LOCAL_STATE_BITS: 150

RS2_GLOBAL_HISTORY_BITS: 480
RS3_GLOBAL_HISTORY_BITS: NOT_FROZEN
```

There is no canonical RS3 GLOBAL core or RS3 candidate/effect encoding to
audit.  Inferring RS3 capacity from RS2 would be unsound.

## Candidate/effect minimization audit

The frozen module boundary accepts arbitrary independent valid, actual-release,
and actual-borrow bitmaps.  The controller explicitly uses the latter two
because PatternID can consume fewer resources than its action/config envelope.
No frozen function derives either effect bitmap from candidate index alone.

```text
CURRENT_HISTORY_BITS: 480
PROVABLY_DERIVABLE_BITS: 0
PROVABLY_MINIMIZED_HISTORY_BITS: 480
POTENTIAL_MINIMUM_BITS: NOT_PROVEN
MINIMIZATION_STATUS: PROVEN_FOR_CURRENT_MODULE_BOUNDARY
```

This does not preclude a future compact representation; it requires a new
proof from a frozen candidate-map producer through exact actual-effect
derivation.  P2-SCRATCH neither changes nor assumes such a proof.

## Existing storage-source audit

| Source / module | Capacity and type | Owner and lifetime | Access / latency | Classification |
|---|---|---|---|---|
| Retained collector bank, `recam_dss_v2_retained_collector_bank` | 821 bits/SA; 3,284 bits/4 SA; coherent register packing | Its collector/analyzer path owns it from accepted fault through analysis. No GLOBAL ownership handoff or release event exists. | Internal register is exposed read-only as `retained_state_o`; writes are fault transitions or clear only. Combinational reconstruction depends on it. | CAPACITY_ONLY_NOT_REUSABLE |
| Retained overlap core, `recam_dss_v2_retained_overlap_core` | four retained banks plus scheduler/analyzer state | Isolated historical retained-state path; collection and analysis can overlap. Final repair/program boundary is not an integrated canonical GLOBAL producer. | No candidate-store ports and no safe overwrite arbitration. | NOT_REUSABLE |
| Hybrid delta bank, `hybrid_delta_bank` | 2,072 bits at frozen defaults; register bank | Historical/alternate `dss_2x2` analyzer owns captured Hybrid/Must view until selected final decode. | One selected-SA capture write; wide replay outputs; no indexed GLOBAL read/write port. | NOT_REUSABLE |
| Pending repair buffer, `pending_repair_buffer` | 3,866 bits at frozen defaults; register bank | Holds selected repair data after `selection_done_o` for final programming; its contents are live after selection. | Atomic group capture and wide outputs only; no legal pre-selection overwrite. | NOT_REUSABLE |
| CAM-reuse temporary buffer, `cam_reuse_temp_buffer` | 144 bits at defaults: 5 x {valid,row,col,cfg}, occupancy, overflow | Collector-owned while faults are collected/reuse state is needed. | Append-only one write/cycle and wide replay; capacity below 480 bits. | NOT_REUSABLE |
| Historical candidate stores, `dss_v2_group_candidate_store` / `dss_v2_rs3cs3m1_group_candidate_store` | 80 / 112 bits | Historical deferred-policy candidate history, not canonical GLOBAL. | One indexed asynchronous read plus one sequential write; capacity below 480 bits. | NOT_REUSABLE |
| `sync_fifo` primitive | Parameterized, but no canonical-GLOBAL production instance or allocation | No owner, instance, width, lifetime, or arbitration contract is frozen. | FIFO order is incompatible with random candidate indexing without an adapter. | NOT_PROVEN |
| CAM/SRAM C++ models | Simulation models, not an instantiated RTL scratch macro | No canonical-GLOBAL hardware owner or ports. | Not a synthesizable storage source for this boundary. | NOT_REUSABLE |

The retained 821-bit/SA result proves collector-semantic state sufficiency,
not scratch eligibility.  Its generation-aware update rule means it is live
while BIST collection or analyzer restart remains possible.  The module has no
write interface by which GLOBAL can borrow only part of the storage, and no
event marks its bits reusable.  It must not be overwritten.

## Ownership and lifetime

No storage source is selected, therefore no reuse ownership transition is
frozen.  The audited lifetimes are:

| Interval | Retained collector | Hybrid delta / pending repair | Canonical GLOBAL candidate history |
|---|---|---|---|
| BIST fault collection | LIVE, WRITEABLE by collector | collector input/live when used | not yet captured |
| Local RECAM analysis / candidate generation | LIVE, READ_ONLY-to-analysis with generation invalidation on a fault | LIVE when captured/needed for decode | producer boundary not integrated |
| GLOBAL search | no handoff defined; reuse forbidden | no handoff defined; reuse forbidden | local NoScratch registers only |
| Solution commit | retained path ownership still not released | pending repair is LIVE for program handoff | selected tuple is live; candidate history may be discarded only in a future defined backend |
| eFuse/final repair programming | lifecycle not integrated with canonical GLOBAL | pending repair remains LIVE | no current WithScratch backend |
| Online mode | no global scratch allocation contract | no global scratch allocation contract | not applicable |

```text
SELECTED_SCRATCH_SOURCE: NONE
SCRATCH_CAPACITY_BITS: NOT_FROZEN
SCRATCH_OWNER_BEFORE_GLOBAL: NOT_APPLICABLE
SCRATCH_OWNER_DURING_GLOBAL: NOT_APPLICABLE
SCRATCH_OWNER_AFTER_GLOBAL: NOT_APPLICABLE
SCRATCH_REUSE_START_EVENT: NOT_DEFINED
SCRATCH_REUSE_END_EVENT: NOT_DEFINED
OVERWRITE_SAFE: NO
```

The closest capacity source, the four retained collector banks, is not selected
because no event proves that it has ceased to be semantically live before the
future GLOBAL search and final repair-program path.

## Required CandidateStore behavior for a future contract

This is a required interface shape, not a frozen or implemented RTL interface:

```text
capture/start: atomically accept valid[159:0], release[159:0], borrow[159:0]
search read:   request one candidate index; return its {valid,release,borrow}
clear/release: invalidate the captured generation after final result consumption
generation:    identify the captured candidate-map generation and reject stale response
```

The controller presently assumes combinational indexed reads from local
registers.  A backend with one- or two-cycle read latency requires an
`FSM_SCHEDULING_CHANGE` only: request/response tracking, a response-valid
stage, and stale-generation suppression.  It must not change search order or
what constitutes a legal branch.

```text
GLOBAL_LOGICAL_READ_PORTS_REQUIRED: 1 indexed 3-bit candidate-effect read/candidate visit
GLOBAL_LOGICAL_WRITE_PORTS_REQUIRED: 1 atomic 480-bit capture transaction/start
SCRATCH_PHYSICAL_READ_PORTS_AVAILABLE: NOT_FROZEN
SCRATCH_PHYSICAL_WRITE_PORTS_AVAILABLE: NOT_FROZEN
READ_PORT_MATCH: NOT_PROVEN
WRITE_PORT_MATCH: NOT_PROVEN
SCRATCH_READ_LATENCY: NOT_FROZEN
SCRATCH_WRITE_LATENCY: NOT_FROZEN
CURRENT_NOSCRATCH_READ_LATENCY: combinational indexed register read
SEARCH_CONTROLLER_CHANGES_REQUIRED: backend request/response scheduling only; no semantic change

CANDIDATE_STORE_INTERFACE_FROZEN: NO
LOCAL_STORE_BACKEND_DEFINED: YES (current NoScratch registers)
SCRATCH_STORE_BACKEND_DEFINED: NO
```

## Analytical latency envelope

The current core tests one candidate per SEARCH cycle.  Its measured/frozen
candidate-visit bounds are 4 best case and 2,625,640 worst case.  The following
is derived for an unprefetched backend; start/done overhead is unchanged and
not included.

| Backend | Candidate service cycles, best / worst | Added cycles per candidate read | Evidence |
|---|---:|---:|---|
| Local register backend | 4 / 2,625,640 | 0 | current RTL contract |
| One-cycle scratch read | 8 / 5,251,280 | 1 | DERIVED; needs request/response FSM |
| Two-cycle scratch read | 12 / 7,876,920 | 2 | DERIVED; needs request/response FSM |

No latency trade-off is authorized until a physical source and port contract
are selected.

## State and system constraints

```text
REUSED_EXISTING_SCRATCH_BITS_RS2: 0 (proven)
REUSED_EXISTING_SCRATCH_BITS_RS3: NOT_FROZEN
NEW_LOCAL_HISTORY_BITS_RS2: 480 (current NoScratch implementation)
NEW_LOCAL_HISTORY_BITS_RS3: NOT_FROZEN
NEW_CONTROL_STATE_BITS: 150 (current NoScratch implementation)
TOTAL_LOGICAL_STATE_RS2: 630
TOTAL_LOGICAL_STATE_RS3: NOT_FROZEN

REUSED_EXISTING_SCRATCH_BITS: 0
NEW_DSS_LOCAL_STATE_BITS: 480
NEW_ADAPTER_CONTROL_BITS: NOT_FROZEN
TOTAL_REQUIRED_LOGICAL_STATE: 630 (current NoScratch reference only)
```

Future system constraints, deliberately out of scope here: a device-wide
scratch source would require one-group-at-a-time ownership or explicit bounded
arbitration, lifetime isolation across banks/channels, and coordination with
the separately modeled online CAM pool.  P2-SCRATCH starts neither a device
sweep nor multi-group scheduling/arbitration.

## Future verification and matched synthesis plan

Any future storage backend must compare the same candidate-map input through
the same canonical controller to selected-tuple output.  Required checks are
directed witnesses including N2/F16/group172, 1,000 seeded candidate-map
vectors, exact selected action/config/PatternID/donor/effect fields, final
masks, repairability, and final committed result:

```text
GLOBAL_SCRATCH_NOSCRATCH_MISMATCHES: 0
```

The future synthesis comparison boundary is fixed as:

```text
candidate-map input -> Normalized GLOBAL controller -> selected tuple output
```

Both variants must use that exact boundary; no area number is created in P2.

## Safety record

```text
GLOBAL_SEARCH_SEMANTICS_CHANGED: NO
FUTURE_DONOR_SEMANTICS_CHANGED: NO
CANONICAL_ORDER_CHANGED: NO
NORMALIZED_GLOBAL_NOSCRATCH_MODIFIED: NO
CXX_SEMANTICS_MODIFIED: NO
FROZEN_GROUP_DATA_MODIFIED: NO
FORMAL_100K_TOUCHED: NO
DEVICE_SWEEP_STARTED: NO

NEXT_AUTHORIZED_PHASE: NONE (requires an explicit, physically owned scratch source and frozen interface contract)
```
