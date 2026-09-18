# Candidate transition adapter specification

This combinational RS2/CS2/m1 directional adapter translates a locally valid candidate descriptor into actual release/borrow effects and fixed physical resource identities. A/D compare row demand against RS and column demand against CS; B/C apply the transposed comparison. Effects are qualified by `candidate_valid_i`.

The fixed mappings are A `(A_ROW,B_COL)`, B `(B_COL,D_ROW)`, C `(C_COL,A_ROW)`, and D `(D_ROW,C_COL)` for `(release, donor)`. The module has no state, clock, reset, policy priority, or ledger mutation.

```json
{"signal":[{"name":"candidate_valid_i","wave":"01."},{"name":"actual_release_o","wave":"01."},{"name":"actual_borrow_o","wave":"0.."}]}
```
