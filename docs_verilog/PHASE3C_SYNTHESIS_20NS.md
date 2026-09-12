# Phase 3C 20 ns Synthesis Record

Two timing-driven DC W-2024.09-SP2 views use the Phase 3A/3B method: TSMC018
`slow.db`, `slow`, `tsmc18_wl10`, a 20.0 ns virtual clock, and zero I/O delay.
`compile -map_effort low` is retained for comparability (this DC defaults it to
medium). GE uses `NAND2X1`, actual slow-library area `9.979200`.

| View | Boundary | Cell area | Comb. / seq. area | Leaf / comb. / seq. cells | GE | Path | WNS / TNS | Closure |
|---|---|---:|---:|---:|---:|---:|---:|---|
| Control-only | Scheduler + 80-bit four-entry map | 6,945.523273 | 2,418.292815 / 4,527.230457 | 300 / 215 / 85 | 696.00 | 2.53 ns | 0.00 / 0.00 ns | PASS |
| Integrated | Scheduler + map + exactly one Phase 3B analyzer | 72,289.325428 | 67,762.094970 / 4,527.230457 | 3,695 / 3,610 / 85 | 7,244.00 | 20.00 ns | 0.00 / 0.00 ns | PASS |

The 85 sequential cells implement all 80 map bits plus busy, done, latched
role, and the two-bit scan counter. Phase 3B itself remains zero-sequential.
Integrated hierarchy identifies exactly one shared analyzer: 65,350.4550 area;
scheduler/map local area is 6,938.8704 (2,411.6400 combinational plus
4,527.2305 sequential).

The integrated critical path is `pivot_valid_i[1]` to `current_pattern_id_o[3]`,
79 levels and 20.00 ns. It closes the 20 ns virtual evaluation boundary. This
established virtual-I/O methodology reports a PI-to-output path, not a literal
`scan_index → analyzer → map-register` arc; the 85 captured registers are
nevertheless mapped. A future interface-level STA constraint may measure that
internal arc specifically.

Against Phase 3B standalone at 20 ns (67,229.870944 area, 6,737.00 GE, 3,514
cells, 19.98 ns, WNS +0.02), integrated Phase 3C adds 5,059.454484 area
(+7.53%), 507.00 GE (+7.53%), 181 cells (+5.15%), 85 sequential cells and
4,527.230457 sequential area. Path changes +0.02 ns and WNS -0.02 ns, while
closure remains PASS. The control-only 6,945.523273 area is not expected to
equal this delta: cross-boundary optimization and changed analyzer port loading
reduce the integrated analyzer hierarchy versus the standalone boundary.

The frozen collector contract is authoritative: collection completes, then
Pivot CAM state, Must metadata, Hybrid CAM state, and conventional overflow
remain stable for SCAN0–SCAN3. No full CAM-state snapshot, double buffer, or
shadow CAM is in this area. Reports: `results/phase3c/dc_tsmc018_slow/phase3c_control_20ns`
and `results/phase3c/dc_tsmc018_slow/phase3c_integrated_20ns`.
