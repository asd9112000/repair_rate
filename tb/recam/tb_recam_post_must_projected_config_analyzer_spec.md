# `tb_recam_post_must_projected_config_analyzer` specification

## Purpose

This verification-only combinational top exposes the new isolated 14-to-9
post-Must analyzer wrapper and the frozen seven-slot analyzer fed with the
first seven compact projected records. It contains no state, production policy,
or collector implementation.

## Comparison contract

The C++ testbench supplies injected historical physical states. It compares the
new result and projection against an independent historical oracle for every
Config. When `projected_count_o <= 7`, it additionally requires exact equality
between the new nine-slot result and the old seven-slot analyzer result.

## Combinational timing diagram

```wavedrom
{ "signal": [
  { "name": "historical-state inputs", "wave": "x3", "data": ["stable"] },
  { "name": "new 14-to-9 path", "wave": "x3", "data": ["post-Must result"] },
  { "name": "old 7-slot path", "wave": "x3", "data": ["comparison only"] }
] }
```

## Corner cases

- The old-path comparison is valid only for a compact view of seven or fewer
  entries.
- Projection overflow is a test failure for legal historical states.
- Directed tests cover membership holes, descriptor-selected RowMust/ColMust,
  both nine-slot witnesses, and the former eleven-entry blocker.
