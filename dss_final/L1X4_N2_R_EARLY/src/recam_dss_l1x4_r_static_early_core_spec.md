# `recam_dss_l1x4_r_static_early_core`

This is the frozen `Line1x4RowStaticEarly` controller. It visits A, B, C, D
in order and tries `R, L, RB, B` candidates. A permits only LOCAL/RELEASE;
D permits only LOCAL/BORROW. A borrow at B, C, or D requires a release by its
immediate upstream neighbor. Thus the implementation has no reverse sharing,
wraparound, transitive borrowing, or re-lending.

Each compatible candidate commits on its analysis cycle. `selected_commit_o`
is observational only and enables final selected-line capture in the parent.
An active-low synchronous reset clears all policy state.

```wavedrom
{ "signal": [
  {"name":"clk_i", "wave":"p....."},
  {"name":"SA", "wave":"2.3...", "data":["A","B"]},
  {"name":"upstream release", "wave":"010..."},
  {"name":"downstream borrow commit", "wave":"0.10.."}
] }
```
