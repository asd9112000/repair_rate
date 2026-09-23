# `recam_dss_g2x2_r_static_early_core`

The core implements frozen `Grid2x2RowStaticEarly`: it visits SA A through D and
tries slots in `R, L, RB, B` order. Slot `RB` aliases LOCAL; row-only action mapping is LOCAL=2R2C, RELEASE=1R2C, BORROW=3R2C. A compatible candidate commits immediately;
there is no candidate store or deferred GROUP selection. `selected_commit_o`
observes the exact commit predicate and has no feedback into policy state.

An active-low synchronous reset clears sequence state. `start_i` begins a new
sequence only when idle. A successful D commit asserts completion; exhaustion
of slot priority reports the failing SA.

```wavedrom
{ "signal": [
  {"name":"clk_i", "wave":"p....."},
  {"name":"candidate_valid_i", "wave":"01.0.."},
  {"name":"selected_commit_o", "wave":"01.0.."},
  {"name":"current_sa_o", "wave":"2.3...", "data":["A","B"]}
] }
```
