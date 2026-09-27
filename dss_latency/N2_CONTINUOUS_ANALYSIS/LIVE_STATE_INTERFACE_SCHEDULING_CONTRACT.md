# N2 CA-LIVE Interface and Scheduling Contract

## Scope and boundary

This contract defines the policy-engine boundary for the future isolated
`EARLY-CA-LIVE` and `GROUP-CA-LIVE` variants.  It does not integrate a raw
fault producer, modify RTL, or change the canonical and CA-SB evidence.

The authoritative per-SA collector/CAM state remains upstream and outside the
policy-engine synthesis top.  The policy top consumes one current
active-SA projection and never stores a four-SA history of that projection.
Consequently the CA-LIVE boundary matches the old canonical policy boundary:
the old tops expose the same analyzer inputs as caller-owned ports and contain
no collector instance.

## Live analyzer input projection

The active-SA projection is 272 bits, matching the existing N2 canonical
analyzer interface with `ROW_ADDR_W=9`, `PHYS_COL_ADDR_W=13`, and
`HYBRID_ENTRIES=7`.

| Signal | Width | Meaning | Upstream source | Form |
| --- | ---: | --- | --- | --- |
| `pivot_valid_i` | 5 | valid pivot prefix | Address-CAM state | registered state projection |
| `pivot_rows_flat_i` | 45 | five physical row addresses | Address-CAM state | registered state projection |
| `pivot_cols_flat_i` | 65 | five physical column addresses | Address-CAM state | registered state projection |
| `row_gt1_i`, `row_gt2_i`, `row_gt3_i` | 15 | row threshold relations | collector counters and pivots | combinational projection of persistent state |
| `col_gt1_i`, `col_gt2_i`, `col_gt3_i` | 15 | column threshold relations | collector counters and pivots | combinational projection of persistent state |
| `hybrid_valid_i` | 7 | valid Hybrid records | Hybrid-CAM state | registered state projection |
| `hybrid_pointer_flat_i` | 21 | pivot pointers | Hybrid-CAM state | registered state projection |
| `hybrid_descriptor_i` | 7 | row/column relation descriptor | Hybrid-CAM state | registered state projection |
| `hybrid_differing_flat_i` | 91 | differing physical addresses | Hybrid-CAM state | registered state projection |
| `conventional_overflow_i` | 1 | collector overflow qualification | collector status | sticky registered state |
| **Total** | **272** | current active-SA analyzer input | upstream collector/CAM | — |

No field is missing from the existing analyzer contract.  A physical
collector-to-this-272-bit projection is an upstream integration responsibility
and is excluded from CA-LIVE policy synthesis; this document does not claim
that an already-integrated raw-fault producer exists.

## Serial active-SA schedule

The N2 timing contract uses serial BIST order `A -> B -> C -> D`.  CA-LIVE
therefore services exactly one `active_sa` at a time.

1. `active_sa` begins at A and advances only after that SA has its latest
   result accepted/frozen.
2. A fault-state update belongs to the active SA; its post-update live
   projection is the analyzer input for the next cycle.
3. A configuration scan can restart on every active-SA update.
4. A completed earlier GROUP SA retains only its four existing candidate-map
   entries and its existing pivot-reconstruction record; it retains no
   analyzer-input payload.

The current DATE BIST schedule exposes `SA_TEST_DONE` as a completed-word
event, not a sticky hardware level.  CA-LIVE therefore uses the event-shaped
contract `test_done_valid_i` plus `test_done_sa_i`.  Because service is
serial, `test_done_seen_q` is one bit for the active SA.  It is set by the
event and cleared when `active_sa` advances.  A same-cycle fourth-config and
test-done event must be accepted without an extra wait edge.

## Fault-update priority

`fault_state_update_i` has priority over config completion, candidate-map
write, EARLY acceptance, and GROUP advancement.  On an update edge:

1. suppress the old-generation candidate-map write;
2. suppress old EARLY acceptance or GROUP completion;
3. clear the active result-complete condition;
4. set the current canonical slot/rank to the first entry; and
5. evaluate only the post-update live projection on the subsequent scan edge.

This applies equally when an old candidate is combinationally ready on the
same clock edge.  The required future directed test is
`FAULT_UPDATE_SAME_EDGE_AS_CONFIG_RESULT` and must prove no old write, no old
commit, and first-config restart.

## EARLY-CA-LIVE

EARLY scans the unchanged role-local priority and accepts the first analyzer
valid **and** prefix-compatible result.  It then enters HOLD, retaining the
current configuration/rank and the existing prefix facts while the live state
is stable.  The analyzer output is the pending solution; no selected
PatternID/configuration payload register is required.  An active-SA update in
HOLD invalidates readiness and restarts at the first rank.

Relative to the canonical EARLY controller, the conservative minimum new
control is two bits:

| New state | Bits | Reason |
| --- | ---: | --- |
| SCAN/HOLD state expansion | 1 | distinguish existing busy scan from HOLD |
| `test_done_seen_q` | 1 | remember an active-SA completed-word event before HOLD |
| **Total new state** | **2** | no analyzer-input or solution payload state |

`active_sa`, rank, prefix facts, and terminal outputs reuse the canonical
controller roles.  The actual encoding may reuse equivalent existing bits but
must not exceed this purpose without documented reason.

## GROUP-CA-LIVE

The canonical 80-bit candidate store remains the sole Config-result
persistence structure: `4 SA x 4 slots x {PatternID[3:0], valid}`.  Each
active-SA scan overwrites CFG1 through CFG4; an infeasible configuration
writes the existing all-zero invalid record.  Clearing the map is unnecessary
because result-complete is represented by scheduler state and every one of
the four slots is overwritten before freeze.

Freeze an SA only after both its latest four-slot scan is complete and its
`test_done_seen_q` is true.  Earlier SAs keep their canonical map records.
For the final SA, the unchanged 81-path selector consumes the completed map,
then the existing registered result supplies the fifth response cycle.  The
440-bit pivot retention remains unchanged because the map contains no physical
pivot addresses for final line reconstruction.

Relative to the canonical GROUP controller, the conservative minimum new
control is two bits:

| New state | Bits | Reason |
| --- | ---: | --- |
| scan-complete/wait-done state expansion | 1 | distinguish scan, wait, and final selection states |
| `test_done_seen_q` | 1 | remember the active-SA completed-word event |
| **Total new state** | **2** | no generation tag, map clear, or per-SA payload state |

## Non-negotiable preserved logic

- one unchanged shared analyzer;
- unchanged canonical Config and PatternID priority;
- unchanged EARLY prefix legality, including vector 27;
- unchanged GROUP 80-bit candidate map and 81-path selector;
- unchanged 440-bit pivot reconstruction retention;
- no CA-SB 4 x 272-bit state bank and no wide four-way state-selection cone.

## Future verification plan

EARLY must run canonical lockstep, vector-27 prefix legality, update during
SCAN, update during HOLD, same-edge update/result preemption, BIST done before
solution, and solution before BIST done.

GROUP must run canonical lockstep, same-edge write suppression, updates during
each of four slots, four-slot overwrite proof, BIST done during an incomplete
scan, earlier-SA freeze/no replay, final-SA five-cycle response, and final
81-path equivalence.

## Closure

```text
CA_LIVE_SYNTHESIS_BOUNDARY_MATCHES_OLD_CANONICAL: YES
LIVE_ANALYZER_INPUT_TOTAL_WIDTH: 272
ALL_INPUTS_AVAILABLE_FROM_UPSTREAM_STATE: YES (at this declared boundary)
N2_CA_LIVE_ANALYZER_SERVICE: SERIAL_ONE_ACTIVE_SA
BIST_DONE_SEMANTICS: PULSE
BIST_DONE_EXTRA_STATE_REQUIRED: 1 bit
EARLY_CA_LIVE_NEW_STATE_BITS: 2
GROUP_CA_LIVE_NEW_STATE_BITS: 2
CA_SB_PAYLOAD_BITS_REMOVED_PER_POLICY_TOP: 1088
```
