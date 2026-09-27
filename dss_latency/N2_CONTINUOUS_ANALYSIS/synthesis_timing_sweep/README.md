# N2 CA-LIVE timing sweep

This directory holds cross-period evidence for the frozen N2 CA-LIVE
policy-engine timing sweep.  Raw DC reports remain isolated below each
architecture's `rtl/.../synthesis/by_period/<period>/` directory.

The sweep source is commit `9bb956608f66891d0967f4f631afb19812c5a724`.
All points use DC W-2024.09-SP2, TSMC018 `slow.db`, slow corner, zero I/O
delay, `tsmc18_wl10`, `set_load 0.05`, and `compile -map_effort low`.
Only `CLOCK_PERIOD_NS` varies.  GE uses `NAND2X1 = 9.979200 um^2`.

The existing frozen 20 ns reports are reused after source and methodology
verification.  The 10, 15, and 25 ns reports are generated with the same
period-capable runners and kept in separate period directories.
