# recam_n2_preopt_canonical_path_selector specification

The adapter retains all three dense 160-bit maps but derives the first
ascending PatternID for each of the sixteen stored `L,R,B,RB` slots. It passes
that exact 80-bit slot image to the frozen canonical 81-path selector, so path
priority, role legality, ConfigID, PatternID, and donor metadata are identical
to `G2X2_N2_RC_GROUP_reg`.

```json
{ "signal": [
  {"name":"dense maps","wave":"2..."},
  {"name":"16 first-pattern slots","wave":"x234"},
  {"name":"canonical path selection","wave":"x..3"}
] }
```
