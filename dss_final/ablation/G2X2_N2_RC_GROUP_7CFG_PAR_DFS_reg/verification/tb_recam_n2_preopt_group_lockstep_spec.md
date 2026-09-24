# tb_recam_n2_preopt_group_lockstep specification

This verification wrapper supplies one frozen canonical-SA snapshot to both
tops. The ablation receives four replicated snapshots because its proven
pre-optimization producer is SA-scheduled, while the canonical top owns one
snapshot interface. It exposes repairability, selections, resource effects,
and final repair lines for equality comparison.

```json
{ "signal": [
  {"name":"clk","wave":"p................"},
  {"name":"start","wave":"010.............."},
  {"name":"canonical done","wave":"0......10......."},
  {"name":"ablation done","wave":"0..............1"},
  {"name":"compare persistent outputs","wave":"x..............3"}
] }
```

The C++ test resets before each vector because the recovered atomic commit
intentionally retains its resource ledger after an accepted tuple.
