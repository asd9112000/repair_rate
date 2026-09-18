# `recam_post_must_hybrid_projector` specification

## Intent

This combinational module produces one selected Config's compact
`POST_MUST_ANALYZER_VIEW` from the historical 14-entry physical Hybrid set. It
does not retain collector state, alter occupancy, or change Must semantics.

## Ports and behavior

`config_id_i` selects one of the unchanged seven ConfigIDs. Each physical entry
contains validity, a three-bit Pivot pointer, descriptor, normalized differing
address, and seven membership bits. `row_must_by_cfg_i` and
`col_must_by_cfg_i` are the final historical Must views indexed by Config and
Pivot.

The output is a stable H0-to-H13 subsequence. An entry survives exactly when
it is physically valid, membership-valid for `config_id_i`, has
`pointer < MAX_K`, and its descriptor-selected Must bit is clear. The module
copies survivors to slots 0 through 8 without sorting. `projected_count_o`
counts all survivors; `projection_overflow_o` and a simulation assertion make
any count greater than nine explicit rather than silently truncating it.

## Combinational timing diagram

```wavedrom
{ "signal": [
  { "name": "config_id_i / physical inputs", "wave": "x3", "data": ["stable state"] },
  { "name": "final Must view", "wave": "x3", "data": ["stable state"] },
  { "name": "projected outputs", "wave": "x3", "data": ["stable H-order view"] },
  { "name": "projection_overflow_o", "wave": "0." }
] }
```

## Corner cases and checks

- Invalid ConfigIDs produce no membership-valid output entries.
- A pointer outside `MAX_K` is rejected exactly as in the historical bank.
- A legal state must have `projected_count_o <= 9`; a violation is a proof
  contradiction, not a supported truncation behavior.
- The verification contract covers row/column Must retirement, membership
  holes, stable ordering, and both nine-entry witnesses.
