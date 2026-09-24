# Closure summary

`CASE_ID: G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg`

## Functional freeze

- Seed-1 canonical reproducer: PASS.
- Canonical lockstep: 10 smoke vectors and 1000 full vectors: PASS; 0 mismatches.
- Exact comparison: repairability, ConfigID, PatternID, action, donor, release/borrow, final line validity/type, and physical repair address: PASS.
- Physical-column identity: `1 != 257`; boundaries `0,31,32,255,256,257,4095,8191`: PASS.
- Dense-map producer: seven lanes, 16 writes, `0/39/40/.../159` boundaries, and release/borrow replication: PASS.
- Strict Verilator full top and wrapper: `-Wall -Werror-WIDTH`: PASS.

`FUNCTIONAL_COMMIT: eed173cb6e8dacc047e557d67346f3f0c71cb59a`

## Matched DC

`STATUS: PASS` under DC W-2024.09-SP2, TSMC018 `slow.db`, slow corner, 20.0 ns, zero I/O delay, and `compile -map_effort low`. The post-run functional source SHA-256 gate passed. Reports are in `../synthesis/reports/`.

Timing is met (WNS/TNS 0.00/0.00 ns); non-timing design-rule violations remain recorded in the QoR and constraints reports. This archive preserves them and makes no RTL changes for PPA improvement.
