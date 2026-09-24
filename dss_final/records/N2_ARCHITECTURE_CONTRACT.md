# N2 architecture contract

## Common contract

```text
RS = 2, CS = 2, m = 1
ROW_ADDR_W = 9
PHYS_COL_ADDR_W = 13
WORD_COL_ADDR_W = 5
HYBRID_LINE_ADDR_W = 13
MAX_K = 5
PIVOT_SLOTS_PER_SA = 5
```

`PHYS_COL_ADDR_W` identifies a physical repair column. `WORD_COL_ADDR_W`
remains the five-bit logical BIST word-column field and is not interchangeable
with a repair-line address.

## RECAM

`RECAM_N2_2R2C` uses 13-bit physical repair-column identity and retains the
five-bit logical BIST word-column convention. It has no DSS EARLY/GROUP state.

## EARLY

EARLY makes each SA decision immediately and exposes an immediate commit
transaction with streaming selected repair lines. It has no four-SA accumulated
final-result warehouse and no GROUP-style full-pivot retention.

## GROUP

GROUP defers decision until all four SAs are available. Full pivot row and
physical-column coordinates must survive until reconstruction:

```text
4 SA × 5 pivots × (9-bit row + 13-bit physical column) = 440 raw address bits
```

The 440 bits are raw pivot-address payload, not the full synthesized GROUP
overhead.
