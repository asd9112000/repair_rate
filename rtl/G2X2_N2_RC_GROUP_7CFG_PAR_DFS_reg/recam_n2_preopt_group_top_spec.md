# recam_n2_preopt_group_top specification

The top snapshots all four SA inputs, builds the dense maps, starts the
recovered OPT1/DFS and atomic-commit diagnostic paths, and exposes the exact
frozen canonical 81-path selection tuple. It captures each SA's five 9-bit
rows and five 13-bit physical columns on that SA's final map slot, then
reconstructs final repair lines from the canonicalized selected tuple.

```json
{ "signal": [
  {"name":"clk","wave":"p........."},
  {"name":"start","wave":"010......."},
  {"name":"map build","wave":"0.1......."},
  {"name":"OPT1 / DFS","wave":"0...1....."},
  {"name":"atomic commit","wave":"0.....1..."},
  {"name":"done","wave":"0........1"}
] }
```
