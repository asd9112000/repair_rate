# S1G-A2R — Historical Collector Minimal-State Audit

`S1GA2R_STATUS: COMPLETE`

## Executive conclusion

The historical collector and the 204-bit V2 analyzer boundary are different
abstractions.  The historical `multi_config_analyzer_bank` consumes a
per-Config logical view; the V2 analyzer accepts one membership-free logical
Hybrid list.  Therefore the current 204-bit input cannot express every
historical per-Config state without changing its interface or defining a
strictly narrower source contract.

No minimum complete retained width is proven.  In particular, the historical
source default has 14 Hybrid entries (`shared_fault_collector` and
`dss_analyzer_top`), while the frozen V2 boundary has seven.  The audit must
not silently choose either value as the complete historical contract.

## Collector state inventory

| Element | Historical representation | Update/order | Config dependence | Retention result |
| --- | --- | --- | --- | --- |
| Pivot payload | compact prefix, valid + row + column, five entries | First mutually distinct faults; append only | `cfg_pivot_valid` is derived from prefix index and `k` | MUST STORE; order matters |
| Pivot occupancy | `ceil(log2(6))=3` bits | Increment on pivot write | No | DERIVABLE from compact valid prefix |
| Pivot cfg view | 5 x 7 bits | Continuous `entry < k(cfg)` decode | Yes | DERIVABLE from prefix and fixed Config table |
| Row counters | 12 x (valid + row address + 4-bit count) = 168 bits at DATE widths | Every accepted stored fault; first-free insertion order | No | Not proven redundant for future transitions |
| Column counters | 12 x (valid + ColumnWord + 4-bit count) = 120 bits | Every accepted stored fault; first-free insertion order | No | Not proven redundant for future transitions |
| Must view | 7 x 5 row + 7 x 5 column bits | Combinational count/Config threshold decode | Yes | DERIVABLE only when counters/pivots remain available |
| Hybrid records | default 14 entries; DATE field form is valid + fault-ref + row + column + descriptor + ptr + cfg mask | Related fault; append order | Yes | MUST STORE or losslessly derive |
| Hybrid `cfg_valid` | 7 bits per record | `relation_ptr < k(cfg)` at insertion | Yes | MUST STORE; direct analyzer consumer |
| CAM-reuse records | 12 x (valid + row + column + cfg mask) = 264 bits at DATE widths | Additional pivot relevant to some Config | Yes | MUST STORE for complete collector semantics |
| Overflow flags | counter, Hybrid, reuse; pivot-full is internally guarded | Capacity event, sticky until clear | Some effects per view | Distinctions not proven derivable from one bit |
| Fault count / Hybrid fault-ref | 4 bits each at `MAX_FAULTS=12` | Accepted fault order | No | Required for exact historical store, not consumed by candidate bank |

All stores clear on `clear_i`; insertion is append-at-occupancy, so compact
prefix and entry order are architectural transition state.  No deletion exists
inside these collector stores; logical Hybrid visibility can shrink through
Must filtering, without changing its stored `cfg_valid` mask.

## Hybrid membership proof

The collector sets a Hybrid membership bit when:

```text
relation_found && relation_ptr < k(ConfigID)
```

with `k={4,3,5,4,3,5,4}` for ConfigIDs 0..6.  `multi_config_analyzer_bank`
then requires that bit before presenting the record to that Config, and also
filters records retired by the relevant Must condition.

Directed transition: accept distinct pivots `(0,0),(1,1),(2,2),(3,3)`, then
accept `(3,4)`.  The last fault relates to pivot index 3.  Its historical
membership is visible to ConfigIDs `{0,2,3,5,6}` (`k>=4`) and hidden from
`{1,4}` (`k=3`).  A membership-free 204-bit Hybrid projection exposes the
same record to all Configs.  This can alter its dictionary insertion/full
path, Config feasibility, and representative PatternID.  It is a reachable
semantic counterexample to lossless 204-bit projection.

## Dictionary, counter, overflow, and ordering findings

The fixed Config dimensions are: C0 `2R2C`, C1 `2R1C`, C2 `3R2C`, C3
`3R1C`, C4 `1R2C`, C5 `2R3C`, C6 `1R3C`; their pivot/dictionary capacity is
their `k=R+C` value.  A physical Hybrid counts only in Configs selected by its
membership and not retired by that Config's Must view.  Thus one physical
entry can be ignored by C1/C4 and counted by C0/C2/C3/C5/C6.

Physical store full, per-Config dictionary full, collector overflow, and
candidate infeasibility are distinct.  The counter tables use first-free
address insertion and exact counts; no source proof establishes that they can
be removed while preserving all future collector transitions.  The CAM-reuse
and Hybrid stores likewise preserve append order.  Equivalent unordered sets
are therefore not proven equivalent: Hybrid processing order can change
dictionary extension and PatternID.

## Compatibility and counterexamples

| Historical state | 204-bit summary | Previous 264-bit reuse stub | Existing V2 analyzer consumes it | Required for future transition |
| --- | --- | --- | --- | --- |
| Pivot payload/order | Present | Present | Yes | Yes |
| Must threshold view | Present, condensed | Present | Yes | Yes |
| Exact counters | Absent | Absent | Indirectly | Not proven derivable |
| Hybrid payload/order | Present only as logical 7-entry form | Present | Yes | Yes |
| Hybrid cfg membership | Absent | Absent | Historical only | Yes |
| CAM reuse | Absent | Present | No | Yes for complete collector |
| Per-Config dictionary occupancy | Derived only in historical view | Absent | Historical only | Yes / not reconstructible from 204 |
| Overflow distinctions | Collapsed to one bit | Collapsed | Partly | Not proven derivable |

CE-1 is the pivot-3 Hybrid trace above.  CE-2 is the same record included for
one `k=3` view versus masked from it: its dictionary occupancy diverges.
CE-3 remains unproved as an output divergence, but append order is consumed
by the historical Hybrid traversal.  CE-4 is a membership-hidden state with
the same 204 projection; a future related fault can extend different
per-Config dictionaries.  CE-5 is the fourth distinct pivot, which creates
CAM reuse for `k=3` Configs while retaining a similar visible pivot prefix.

## Required architecture decision

```text
S1GA2R_STATUS: COMPLETE
HISTORICAL_STATE_INVENTORY_COMPLETE: YES
HYBRID_CFG_VALID_SEMANTICS_AUDITED: YES
PER_CONFIG_DICTIONARY_SEMANTICS_AUDITED: YES
COUNTER_SEMANTICS_AUDITED: YES
OVERFLOW_DISTINCTIONS_AUDITED: YES
ENTRY_ORDERING_AUDITED: YES
204BIT_ANALYZER_PROJECTION_LOSSLESS: NO
MINIMUM_PROVEN_RETAINED_STATE_BITS_PER_SA: NOT_PROVEN
HYBRID_CFG_VALID_REQUIRED: YES
PER_CONFIG_COUNTER_STATE_REQUIRED: NOT_PROVEN (not proven derivable)
EXPLICIT_OVERFLOW_STATE_REQUIRED: NOT_PROVEN
ORDERING_STATE_REQUIRED: YES
COUNTEREXAMPLE_COUNT: 5
LOSSLESSNESS_PROOF: FAIL for the existing 204-bit projection
V2_ANALYZER_CHANGED: NO
BASELINE_EARLY_CHANGED: NO
GROUP_CHANGED: NO
S1GB_RESUMED: NO
AUDIT_DOCUMENT: docs/dss_execution/S1GA2R_HISTORICAL_COLLECTOR_MINIMAL_STATE_AUDIT.md
```

No retained-state implementation is authorized until a new contract resolves
the 14-versus-7 Hybrid-capacity boundary and carries the Config-specific
Hybrid view or an equivalent lossless projection.
