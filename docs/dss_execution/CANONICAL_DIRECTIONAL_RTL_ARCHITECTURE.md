# Canonical Directional RTL Architecture

> Superseding note (2026-09-18): the later execution task froze canonical
> `R,L,RB,B` ordering and future-donor release obligations. GLOBAL-NoScratch is
> now implemented and documented in
> `CANONICAL_DIRECTIONAL_GLOBAL_RTL_IMPLEMENTATION_STATUS.md`; the historical
> GLOBAL stop-condition discussion below is retained only as provenance.

> Status: Streaming EARLY implemented and functionally closed at the
> candidate/ledger boundary.  RS2 analyzer integration is compiled.  GLOBAL
> architecture is specified but implementation is stopped pending contract
> reconciliation.

## Architecture identity

Every result must name three independent dimensions:

```text
RESOURCE POINT: RS2 or RS3
POLICY: NORMALIZED_STREAMING_EARLY or NORMALIZED_GLOBAL
STORAGE: STREAMING, NOSCRATCH, or WITHSCRATCH
```

The canonical source root is `rtl/dss_canonical/`.  Historical `rtl/recam/`,
`rtl/dss_2x2/`, and `rtl/dss_v2/` remain unchanged.

## NORMALIZED_STREAMING_EARLY dataflow

```text
current SA/action request
        |
        v
existing static configuration table ---> existing shared analyzer
        |                                  | solution-valid + PatternID
        +------------------+---------------+
                           v
             canonical streaming controller
                           |
                    topology/feasibility
                           |
                  first legal R,L,RB,B
                           |
                           v
             existing registered live ledger
                           |
                 irreversible atomic commit
                           |
                         next SA
```

The traversal is A, B, C, D.  Every SA scans semantic actions
RELEASE_ONLY, LOCAL, RELEASE_AND_BORROW, BORROW_ONLY.  PatternID order remains
the analyzer's canonical one-based ascending order.  Donor order remains the
directional topology order.  A failure at a later SA neither rolls back nor
revisits an earlier decision.

The policy core has no candidate-history store.  Its state is the controller,
the reused 16-bit physical ledger, and selected trace/result fields.  A
separate EARLY Scratch/NoScratch top would therefore be a false storage split.

## Resource-point specialization

The controller interface and trace widths are shared.  Timing-/identity-
critical tables remain specialized:

| Point | Config/action table | Analyzer | PatternID |
|---|---|---|---:|
| RS2 | `dss_v2_group_slot_decode` | `recam_shared_config_analyzer` | 1..10, carried in 6 bits by canonical trace |
| RS3 | `dss_v2_rs3cs3m1_config_table` | fixed-mask production analyzer required | 1..35, 6 bits |

The canonical core is verified for both tables/topologies using completed
candidate maps.  The RS2 top binds the unchanged analyzer.  No canonical RS3
top is emitted from the current `nth_pattern` source because production RTL is
required to use a fixed/elaboration-time representation and the proven P0
fixed-mask source is not present in this worktree with repository provenance.

## Intended NORMALIZED_GLOBAL dataflow

The frozen architectural intent is:

```text
same analyzer and candidate universe
              |
              v
 CandidateStore: every valid PatternID per SA/action
              |
              v
 deterministic DFS A -> B -> C -> D
 action R -> L -> RB -> B, PatternID ascending
              |
              v
 speculative ledger snapshots only
       | branch fails: restore
       | complete tuple: commit
              v
 committed final ledger and selected tuple
```

The intended candidate record is deliberately small:

```text
address = {SA[1:0], action[1:0]}
payload = candidate_valid_bitmap[MAX_PATTERN-1:0]

derived statically from address:
ConfigID, release identity, borrow requirement, donor order

derived from selected bit:
PatternID = bit_index + 1
```

This avoids storing redundant ConfigID/action/descriptor fields while
preserving every canonical PatternID.  It also explains why the historical
80-/112-bit `{valid, first PatternID}` GROUP stores cannot implement true
GLOBAL.

## Committed versus speculative ledger

Streaming EARLY uses the registered V2 ledger directly; each accepted SA
transaction is immediately committed and visible to the next SA.

GLOBAL must instead keep an immutable committed ledger while searching and a
reversible speculative snapshot at each DFS depth.  Only a complete legal
four-SA tuple may replace the committed ledger.  A failed branch restores its
parent snapshot.  Reusing the existing ledger without a snapshot/load
boundary would leak speculative state and is prohibited.

## Storage backends

The future common interface should provide one candidate-bitmap write during
collection and one candidate-bitmap read during search:

```text
write_valid, write_address, write_bitmap
read_address, read_bitmap
```

`GLOBAL-NoScratch` would implement that interface with local DSS state.
`GLOBAL-WithScratch` may use BIRA scratch only after the owner freezes:

- retained lifetime through complete tuple search and reconstruction;
- entry count and bit capacity;
- read/write ports and read latency;
- whether next-SA analysis overwrites earlier candidate history;
- exact pre-existing resource accounting.

The current retained collector exposes no such port contract.  Capacity
figures alone are insufficient; therefore WithScratch is not implemented and
zero reusable bits are currently proven.

## GLOBAL implementation stop

The current software oracle is not an exact implementation of the frozen
hardware traversal/resource contract:

1. it traverses slot/attempt order `0,1,2,3`, not `1,0,3,2`;
2. its sequential allocator may lend an unused future donor line before an
   explicit RELEASE action exists, while RTL requires a released bit.

The N2/F16/group172 witness exercises the second difference by choosing A
BORROW_ONLY from B before B is selected.  Implementing either behavior in new
RTL would silently choose one of two conflicting contracts.  Per the task's
stop rule, no GLOBAL controller or storage backend is emitted until the
software oracle and hardware ledger legality are reconciled.

## Module ownership

| Module | Owner/function |
|---|---|
| `recam_dss_canonical_streaming_early_core` | canonical policy sequencing and trace; direct ledger consumer |
| `recam_dss_canonical_rs2_streaming_early_top` | unchanged RS2 analyzer integration |
| historical V2 config/topology/ledger modules | authoritative frozen resource-point functions |
| future GLOBAL controller | deterministic DFS and speculative state only |
| future CandidateStore backend | storage placement only; never policy ordering |

## Verification boundary

The completed EARLY regression injects completed candidate maps, as do the
established Phase 4/H4 policy regressions.  It compares selected action,
ConfigID, PatternID, donor, release/borrow, ledger after every commit, failure
position, and group result.  Raw fault-stream replay through all collector and
reconstruction logic is not claimed by this phase.
