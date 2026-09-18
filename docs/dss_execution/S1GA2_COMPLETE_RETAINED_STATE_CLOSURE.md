# S1G-A2 — Complete Retained-State Contract Extension

`S1GA2_STATUS: BLOCKED`

## Stop-condition result

The required source re-derivation disproves the proposed 468-bit complete
contract.  The 264-bit CAM-reuse extension alone cannot make the 204-bit V2
summary lossless for the historical collector, because the summary also omits
per-Hybrid ConfigID membership.

`tagged_hybrid_store` retains `cfg_valid[6:0]` for each of seven entries.
`shared_fault_collector` creates that mask from `relation_ptr < k(config)`.
For a relation to pivot index 3 or 4, the mask excludes the `k=3`
configurations.  The 204-bit summary retains only:

```text
valid + pointer[2:0] + descriptor + differing_address[8:0]
```

and its V2 analyzer treats every valid Hybrid record as applicable.  CAM reuse
does not encode this missing 49-bit (`7 entries x 7 ConfigIDs`) membership
state.

A concrete reachable class is four distinct pivots followed by a related
fault sharing the fourth pivot row or column.  Historical collection stores a
Hybrid record tagged only for configurations whose `k` admits pivot index 3.
The 204-bit projection has no tag.  It can therefore present that record to a
smaller-k V2 configuration, where the pointer is outside that configuration's
active pivot prefix and can change candidate validity through the analyzer's
dictionary-overflow path.  This is a Config/Pattern semantic distinction, not
debug-only storage loss.

The same historical collector also has complete row and column counter tables
(`12 x (valid + address + count)` for each dimension), while the summary has
only three threshold bits per pivot/dimension.  Whether all counter entries
are necessary for the restricted V2 candidate boundary needs a separate
minimality audit; the Hybrid ConfigID mask alone is sufficient to block 468.

## Consequences

The attempted 264-bit CAM-reuse retention stub was removed before closure.
No baseline or GROUP RTL was changed.  The prior 204-bit S1G-A implementation
remains intact and its previous regression evidence remains applicable only
to its restricted analyzer-summary contract.

```text
S1GA2_STATUS: BLOCKED
ANALYZER_SUMMARY_BITS_PER_SA: 204
CAM_REUSE_PAYLOAD_BITS_PER_SA: 264
COMPLETE_RETAINED_STATE_BITS_PER_SA: NOT_PROVEN (468 is insufficient)
COMPLETE_RETAINED_STATE_BITS_4SA: NOT_PROVEN
SUMMARY_CAM_REUSE_ATOMIC_UPDATE: NOT_IMPLEMENTED
SUMMARY_CAM_REUSE_COMMON_GENERATION: NOT_IMPLEMENTED
HISTORICAL_COLLECTOR_STATE_FULLY_REPRESENTED: NO
CAM_REUSE_REACHABLE_DIRECTED_CASE: SOURCE_PROVEN; RTL injection not implemented
CAM_REUSE_SUCCESS_CASE: NOT_RUN
CAM_REUSE_LEGITIMATE_FAILURE_CASE: NOT_RUN
SAME_CYCLE_UPDATE_VS_DONE_PRIORITY: S1G-A PASS; A2 extension blocked
MULTI_GENERATION_CAM_REUSE_RESTART: NOT_RUN
BIASED_RANDOM_VECTORS: 0
BIASED_RANDOM_CAM_REUSE_REACHED: 0
BIASED_RANDOM_MISMATCHES: N/A
PREMATURE_OVERFLOW_INTRODUCED: NO
V2_ANALYZER_CHANGED: NO
BASELINE_EARLY_CHANGED: NO
GROUP_CHANGED: NO
S1GB_RESUMED: NO
TOTAL_RETAINED_COLLECTOR_BITS: NOT_PROVEN
TOTAL_FUNCTIONAL_STATE_BITS: S1G-A unchanged (883 functional)
TOTAL_ADDITIONAL_STATE_BITS: S1G-A unchanged (896 including debug)
CLOSURE_DOCUMENT: docs/dss_execution/S1GA2_COMPLETE_RETAINED_STATE_CLOSURE.md
```

The next admissible action requires a new completeness/minimal-extension
decision.  It must include the Hybrid ConfigID-membership representation and
must re-audit the counter/overflow distinctions before a new width is frozen.
`S1G-B` remains blocked and no follow-on phase is authorized.
