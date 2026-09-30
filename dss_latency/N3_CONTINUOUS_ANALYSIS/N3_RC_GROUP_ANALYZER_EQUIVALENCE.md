# N3 RC-GROUP analyzer equivalence evidence

## Layer 1: fixed-mask authority

The production analyzer uses a constant 80-entry `fixed_mask` case table.
Its source is the preserved `N3_RC_GROUP_FIXED_MASK_EQUIVALENCE.csv`.

```
FIXED_MASK_EQUIVALENCE: PASS
FIXED_MASK_ROWS: 80
FIXED_MASK_MISMATCHES: 0
RUNTIME_NTH_PATTERN: ABSENT
```

## Directed full-width analyzer harness

Permanent harness:
`tb/n3_continuous_analysis/tb_n3_rc_fixed_mask_analyzer.cpp`.

It checks the first valid fixed pattern for 3R3C, 3R2C, 2R3C, 4R3C, 3R4C,
4R2C, and 2R4C; overflow suppression; and a 13-bit anti-alias condition.

```
N3_RC_FIXED_MASK_ANALYZER_PASS cases=9 mismatches=0 physical_column_alias=PASS
```

The alias condition uses physical columns 1 and 257.  A Hybrid differing
address of 257 matches the 257 pivot; changing it to 1 changes the candidate
decision.  This would be lost by a 5-bit historical truncation.

```
ANALYZER_DIRECTED: PASS
PHYSICAL_COLUMN_ALIAS_TEST: PASS
ANALYZER_DIRECTED_MISMATCHES: 0
```

## Open layer-2 coverage

The permanent directed harness is independent of the RTL but does not yet
implement an unconstrained random golden model for arbitrary threshold and
Hybrid matrices.  The full-top corpus below randomizes physical addresses but
is deliberately no-conflict.

```
ANALYZER_RANDOM: NOT_YET_CLOSED
```
