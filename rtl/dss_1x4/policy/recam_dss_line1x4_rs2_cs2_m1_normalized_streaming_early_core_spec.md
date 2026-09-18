# 1x4 Single-Hop STREAMING EARLY core

The core scans `(attempt, PatternID)` in C++ v1 priority: local first,
increasing extra rows, then PatternID. A selected candidate immediately updates
the eight-row physical ownership ledger. The row assignment encoding is
`A=0`, `B=1`, `C=2`, `D=3`, `unassigned=4`.

```wavedrom
{signal:[{name:'start_i',wave:'010'},{name:'SA',wave:'=2345',data:'A B C D'},{name:'commit',wave:'0.1.1.1.10'},{name:'done_o',wave:'0........10'}]}
```
