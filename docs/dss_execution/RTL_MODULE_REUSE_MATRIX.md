# Canonical Directional RTL Module Reuse Matrix

> Status: Gate A complete; implementation authorization is limited to
> `NORMALIZED_STREAMING_EARLY`.  GLOBAL remains stopped on the two contract
> conflicts recorded below.

## Decision

The canonical root is `rtl/dss_canonical/`.  Historical trees remain in place.
The RS2 canonical EARLY path reuses the analyzer, configuration decode,
directional topology/feasibility, registered resource ledger, and diagnostic
adapter.  Its only new functional logic is a streaming controller with the
frozen action order `R,L,RB,B` and widened common PatternID trace.

No existing RTL implements normalized GLOBAL.  The historical GROUP modules
are sequential first-legal allocators and must not be renamed GLOBAL.

## Reuse matrix

| Function | Existing module | New consumer | Reuse mode | Semantic change | Reason |
| -------- | --------------- | ------------ | ---------- | --------------- | ------ |
| RS2 candidate analysis | `rtl/recam/recam_shared_config_analyzer.sv` | `recam_dss_canonical_rs2_streaming_early_top` | DIRECT | None | Verified ConfigID/PatternID universe and lowest-valid PatternID are retained. |
| RS2 slot/ConfigID/descriptor decode | `rtl/dss_v2/group/dss_v2_group_slot_decode.sv` | canonical EARLY core | DIRECT | None | Slot identity remains separate from numeric ConfigID. |
| RS2 topology and donor arbitration | `rtl/dss_v2/topology/dss_topology_2x2_directional.sv` plus `dss_v2_resource_feasibility.sv` | canonical EARLY core | DIRECT | None | Frozen release resource and donor priority are already correct. |
| Physical committed ledger | `rtl/dss_v2/resource/dss_v2_resource_ledger.sv` | canonical EARLY core | DIRECT | None | Preserves atomic commit, one borrower per resource, and registered ownership. |
| Ledger trace packing | `rtl/dss_v2/adapter/dss_v2_legacy_ledger_diagnostic_adapter.sv` | canonical EARLY core | DIRECT | None | Diagnostic compatibility only. |
| Historical streaming controller | `rtl/dss_v2/top/recam_dss_v2_early_core.sv` | canonical EARLY core | SPECIALIZED_COPY_WITH_REASON | Action priority only | Its streaming structure is reused conceptually, but B/C priority is noncanonical and the source is preserved as provenance. |
| Historical GROUP controller | `rtl/dss_v2/top/recam_dss_v2_group_core.sv` | architecture/test reference | REFERENCE_ONLY | None | Its `R,L,RB,B` allocation is normalized EARLY at the audited boundary, but its 80-bit history is unnecessary for streaming EARLY and it is not GLOBAL. |
| Historical priority reader | `rtl/dss_v2/group/dss_v2_group_priority_reader.sv` | canonical EARLY controller | WRAPPED | None | The verified rank-to-slot permutation `1,0,3,2` is embedded at a four-value static control boundary; the 80-bit store interface is not imported. |
| Historical candidate store | `rtl/dss_v2/group/dss_v2_group_candidate_store.sv` | GLOBAL planning only | REFERENCE_ONLY | Inadequate for GLOBAL | It stores only `{valid, first PatternID}`; complete GLOBAL needs every valid PatternID bitmap. |
| RS3 table | `rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_config_table.sv` | canonical EARLY core | DIRECT | None | Provides frozen RS3 role-slot identity and action bits. |
| RS3 topology | `rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_topology.sv` | canonical EARLY core | DIRECT | None | Four-resource m=1 ownership and donor order match RS2. |
| RS3 analyzer | `rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_shared_config_analyzer.sv` | future canonical RS3 analyzer top | REFERENCE_ONLY | Required representation change, not semantic change | Current checkout contains runtime `nth_pattern`; the task forbids it in production. The retained fixed-mask P0 source is not present at a repository path with proven provenance. |
| Retained analyzer/collector | `recam_dss_v2_retained_analyzer_path.sv`, `recam_dss_v2_retained_collector_bank.sv` | GLOBAL scratch feasibility audit | REFERENCE_ONLY | None | These expose retained state and analyzer views, not a candidate-history memory port. |
| Retained overlap controller | `recam_dss_v2_retained_overlap_core.sv` | GLOBAL scratch feasibility audit | REFERENCE_ONLY | None | It owns per-SA collector lifetime; it has no spare 1R/1W GLOBAL store contract. |
| Legacy RECAM scheduler/control | `recam_role_aware_config_scheduler.sv`, `recam_role_aware_config_control.sv` | architecture reference | REFERENCE_ONLY | None | Historical ConfigID scheduling must not override semantic action priority. |
| Legacy analyzer | `recam_shared_config_analyzer.sv` | canonical RS2 top | DIRECT | None | Same authoritative RS2 analyzer listed above. |
| Legacy ledger checker | `recam_physical_resource_ledger_checker.sv` | regression oracle only | REFERENCE_ONLY | None | Production canonical EARLY uses the V2 registered ledger. |
| `rtl/dss_2x2/` equivalents | candidate analyzers, stores, selectors, resource tracker | no production consumer | REFERENCE_ONLY | None | Duplicate/reference family with a different integration boundary; importing it would duplicate verified V2 functions. |
| Canonical streaming policy | none | `recam_dss_canonical_streaming_early_core` | NEW | Frozen `R,L,RB,B` for all roles | Smallest new logic that corrects historical V2 EARLY without candidate history. |
| Canonical RS2 integration | none | `recam_dss_canonical_rs2_streaming_early_top` | NEW | None | Thin binding of the unchanged analyzer to the canonical controller. |
| GLOBAL speculative ledger/search | none | future GLOBAL core | NEW | True backtracking required | No existing audited RTL has reversible speculative ledger snapshots or complete tuple traversal. |

`REFERENCE_ONLY` is an audit disposition, not a requested reuse mode for new
production code.  It means the file was inspected but is not instantiated by
the current canonical deliverable.

## GLOBAL stop-condition evidence

### Software traversal conflict

The new frozen contract requires action traversal `1,0,3,2` (`R,L,RB,B`).
The current `findDirectionalV2GroupGlobalChoice()` builds each SA vector using
`compressedPlansForSubarray()`, which preserves attempt-vector order and then
ascending PatternID.  The implementation and its trace explicitly identify
that order as role slots `0,1,2,3` (`L,R,B,RB`).  Therefore exact selected
action/ConfigID/PatternID equivalence cannot be claimed even if repairability
occasionally agrees.

### Software-versus-RTL resource conflict

`PhysicalResourceLedger::allocateSequential()` allows an earlier SA to borrow
an unassigned shareable line owned by a later, not-yet-selected SA.  The RTL
feasibility boundary instead requires `resource_released_i[donor]` to have
been set by a prior committed/speculative RELEASE action.  The retained
N2/F16/group172 software witness selects A `BORROW_ONLY` from B before B's
candidate is selected; the RTL live-ledger contract cannot make that A
candidate legal at depth A.

This is not a controller implementation detail.  It is a candidate-legality
and donor-lifetime mismatch, so Gate C stops before GLOBAL RTL is written.

## Preliminary scratch feasibility audit

| Item | RS2 | RS3 | Finding |
|---|---:|---:|---|
| Complete candidate bitmap history | 16 × 10 = 160 bits | 16 × 35 = 560 bits | Required if GLOBAL preserves every PatternID. |
| Pattern cursor width per depth | 4 bits | 6 bits | Four DFS depths also need action rank and resume state. |
| Speculative ledger | 16 bits/state | 16 bits/state | At least one reversible snapshot per active depth. |
| Existing complete retained collector state | 821 bits/SA | no frozen RS3 equivalent | Capacity alone does not prove reuse. |
| Candidate-history read/write ports | NOT_DEFINED | NOT_DEFINED | No explicit shared CandidateStore interface exists. |
| Safe overwrite point before final reconstruction | NOT_PROVEN | NOT_PROVEN | Collector/reconstruction state may still be live while tuple search runs. |

Consequently:

```text
GLOBAL_REQUIRED_RETAINED_BITS: NOT_FROZEN (candidate-history lower bound: RS2 160, RS3 560)
GLOBAL_REUSABLE_SCRATCH_BITS: 0 PROVEN
GLOBAL_NEW_LOCAL_BITS_WITH_SCRATCH: NOT_FROZEN
GLOBAL_NEW_LOCAL_BITS_NOSCRATCH: NOT_FROZEN
SCRATCH_IMPLEMENTATION_AUTHORIZED: NO
```

An implementation may proceed only after the software oracle adopts the same
action/resource contract and a physical scratch owner freezes capacity,
lifetime, ports, latency, and overwrite rules.
