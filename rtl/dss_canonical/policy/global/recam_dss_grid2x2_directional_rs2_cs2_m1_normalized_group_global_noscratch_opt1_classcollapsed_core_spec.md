# `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_core` specification

## Architecture and boundary

```text
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
OPTIMIZATION_LEVEL: OPT1_SAFE_CLASS_COLLAPSE
BOUNDARY: completed candidate maps -> class filter -> frozen canonical GLOBAL DFS -> selected tuple
```

This module is an OPT1 wrapper, not a replacement for the frozen canonical
GLOBAL core. It preserves the input/output interface, synchronous active-low
reset, accepted-start behavior, and result timing contract of
`recam_dss_canonical_global_noscratch_core`.

## Interface

Inputs are `clk_i`, `rst_ni`, `start_i`, and the existing 160-bit valid,
actual-release, and actual-borrow candidate maps. Outputs are unchanged:
busy/done/repairability, the selected valid/action/ConfigID/PatternID/donor/
release/borrow tuple, and final released/used masks.

`start_i` is accepted only when the wrapped canonical core is idle. The class
filter is combinational at that accepted edge; the wrapped core retains the
filtered valid map and the unmodified two actual-effect maps. Inputs may change
after the accepted edge without changing the in-flight search.

## OPT1 behavior

At each SA, candidates are considered in frozen R, L, RB, B and ascending
PatternID order. A valid candidate is retained only if no earlier valid
candidate has the same key:

```text
(explicit_release, actual_release, actual_borrow)
```

R and RB are explicit-release actions; L and B are not. The first retained
candidate remains at its original stored slot. This keeps the canonical core's
cursor and action/PatternID reconstruction unchanged while suppressing later
equivalent candidates.

The wrapper has no additional architectural registers. It retains the
canonical 480-bit raw map history inside the child and does not implement a
summary, a dominance frontier, scratch reuse, or an analyzer change.

## Timing diagram

```text
{ signal: [
  { name: 'clk_i',       wave: 'p....' },
  { name: 'start_i',     wave: '010..' },
  { name: 'class filter',wave: '0.1..', data: ['first representatives'] },
  { name: 'busy_o',      wave: '0.1..' },
  { name: 'done_o',      wave: '0...1' },
  { name: 'result tuple',wave: 'x...=' }
] }
```

## Verification cases

- N2/F16/group172 retains its canonical RB/R/B/R selected tuple and future
  reservation fulfillment.
- The effect-only-collapse counterexample retains both L and RB because their
  `explicit_release` values differ.
- 1,000 candidate maps with seed `20260918` match the frozen exhaustive core
  on repairability, tuple fields, release/borrow traces, and final masks.
- The existing canonical GLOBAL regression remains unchanged.
