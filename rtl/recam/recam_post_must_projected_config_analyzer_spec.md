# `recam_post_must_projected_config_analyzer` specification

## Intent

This isolated Option-A wrapper connects the historical 14-entry physical
Hybrid interface to `recam_shared_config_analyzer` only after exact post-Must
projection. It specializes the existing analyzer to nine compact Hybrid slots;
it does not alter baseline EARLY, GROUP, collector retention, ConfigID, or
PatternID contracts.

## Interface and cycle behavior

The module is fully combinational and has no clock or reset. Pivot data and
the existing `row_gt*`/`col_gt*` vectors are passed unchanged to the canonical
analyzer. The additional physical Hybrid and Must inputs are consumed solely by
the projector. The wrapper exposes both analyzer results and projected-view
observability for verification.

## Combinational timing diagram

```wavedrom
{ "signal": [
  { "name": "physical Hybrid / Must / Pivot inputs", "wave": "x3", "data": ["stable state"] },
  { "name": "H0..H13 post-Must projection", "wave": "x3", "data": ["0..9 entries"] },
  { "name": "candidate_valid_o / pattern_id_o", "wave": "x3", "data": ["canonical result"] }
] }
```

## Constraints and verification

- `PHYSICAL_HYBRID_ENTRIES=14`, `POST_MUST_VIEW_ENTRIES=9`, and
  `NUM_CONFIGS=7` are independent parameters.
- The child analyzer keeps its fixed candidate table, PatternID width, and
  candidate scan order; only `HYBRID_ENTRIES` is set to nine.
- Legal inputs require no projection overflow. Verification compares the
  result with the historical post-Must consumer and with the old seven-slot
  analyzer when the projected count is at most seven.
