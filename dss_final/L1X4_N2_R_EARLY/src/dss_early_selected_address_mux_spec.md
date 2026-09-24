# `dss_early_selected_address_mux`

This combinational formatter converts the current EARLY ConfigID, PatternID,
and live five-pivot input into a one-SA committed repair-line transaction. It
contains no sequential state. Rows are zero-extended from 9 to 13 bits; columns
remain physical 13-bit addresses. The parent exposes these outputs only while
its immediate-commit valid is asserted.

```wavedrom
{ "signal": [
  {"name":"selected commit", "wave":"010.."},
  {"name":"solution_line_valid_o", "wave":"0=0..", "data":["five-slot mask"]},
  {"name":"solution_line_address_flat_o", "wave":"3=3..", "data":["selected lines"]}
] }
```
