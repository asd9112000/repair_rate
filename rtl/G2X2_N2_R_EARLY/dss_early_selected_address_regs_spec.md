# `dss_early_selected_address_regs`

## Contract

The module captures only the physical line selected by an immediate EARLY
commit. `commit_enable_i` is asserted for one SA decision; the selected
ConfigID and PatternID determine the selected pivot half before the clock
edge. Each valid output entry retains a 13-bit physical address and a
row/column flag. It never stores an unselected pivot half and therefore is not
the 440-bit GROUP pivot warehouse.

## Reset and timing

An active-low synchronous reset clears all output-valid bits. A commit stores
one SA's active pivot lines at the rising edge and the outputs reflect that
selection immediately after the edge.

## Constraints and verification

`PHYS_COL_ADDR_W` must be at least `ROW_ADDR_W`; row addresses are explicitly
zero-extended to physical-column width. Directed verification covers physical
columns 0, 31, 32, 255, 256, 257, 4095 and 8191, including the non-alias pair
1 and 257.

## Timing diagram

```wavedrom
{ "signal": [
  {"name":"clk_i", "wave":"p...."},
  {"name":"commit_enable_i", "wave":"010.."},
  {"name":"final_repair_line_valid_flat_o", "wave":"0.1.."}
] }
```
