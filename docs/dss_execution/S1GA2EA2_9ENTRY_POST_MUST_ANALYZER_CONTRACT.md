# S1G-A2E-A2 — 9-Entry Post-Must Analyzer Contract

## Scope and terminology

This contract covers only the isolated Option-A analyzer boundary. It does not
define retained collector storage, overlap-control state, EARLY, GROUP, BIST,
or S1G-B work.

```text
PHYSICAL_HYBRID_SET       = H0..H13 historical physical Hybrid records
MEMBERSHIP_VISIBLE_SET(C) = physical-valid records with cfg_valid[H][C]
POST_MUST_ANALYZER_VIEW(C)= membership-visible records surviving final Must

PHYSICAL_HYBRID_ENTRIES = 14
NUM_CONFIGS             = 7
POST_MUST_VIEW_ENTRIES  = 9
POST_MUST_MAX           = [8,3,9,8,3,9,8]
```

The physical store has 14 allocated entries. Membership-visible capacity can
reach 11 for every ConfigID. Only the final post-Must analyzer-visible view
has the common nine-entry bound.

## Exact projector predicate

For selected Config `C` and physical entry `Hi`, emit `Hi` iff:

```text
physical_hybrid_valid[Hi]
&& physical_hybrid_cfg_valid[Hi][C]
&& (physical_hybrid_pointer[Hi] < MAX_K)
&& !(physical_hybrid_descriptor[Hi]
     ? col_must_by_cfg[C][physical_hybrid_pointer[Hi]]
     : row_must_by_cfg[C][physical_hybrid_pointer[Hi]])
```

`pointer < MAX_K` exactly matches the historical consumer's legal-pointer
check. Config-local membership is the separate stored `cfg_valid` bit; it is
not replaced with a different numeric pointer bound.

## Stable compaction and overflow

The projector scans H0 through H13 in ascending physical order. It copies each
survivor to the next compact output slot, creating an ordered prefix in slots
0 through 8 and invalid-fill tail slots. It never sorts, deduplicates, or
repacks by address, pointer, descriptor, ConfigID, or fault type.

`projected_count_o` counts every survivor. A count greater than nine sets
`projection_overflow_o` and triggers a simulation-only assertion. This is a
proof contradiction for a legal input, not a permitted truncation behavior.

## Analyzer consumer and widths

The isolated wrapper feeds the compact outputs to unchanged
`recam_shared_config_analyzer` with `HYBRID_ENTRIES=9`. Pivot inputs,
threshold inputs, candidate table, candidate order, PatternID width, and
overflow sideband remain unchanged.

```text
base fields = pivot-valid5 + pivot-rows45 + pivot-columns25
            + row_gt15 + col_gt15 + overflow1 = 106

old Hybrid payload = 7 × (valid1 + pointer3 + descriptor1 + differing9) = 98
new Hybrid payload = 9 × (valid1 + pointer3 + descriptor1 + differing9) = 126

OLD_ANALYZER_INPUT_BITS = 106 + 98  = 204
NEW_ANALYZER_INPUT_BITS = 106 + 126 = 232
```

The projected count is four bits, exposing the illegal 10-through-14 range.
It is not a retained-state counter. Config structures remain seven-wide.

## Semantic preservation and verification

ConfigID remains the existing seven-value encoding and role mapping. PatternID
remains four bits; the candidate set, priority, scan order, and spare-index
allocation meaning do not change. Final Must vectors remain matrix inputs;
only Must-retired Hybrid payload records are removed from traversal.

Verification compares injected historical physical views to an independent
historical predicate/oracle. For compact views of at most seven entries it also
requires exact equality with an actual frozen seven-slot analyzer instance.
Coverage includes row/column blockers, descriptor-selected retirement,
membership holes, H-order packing, all seven capacity witnesses, and random
reachable states.
