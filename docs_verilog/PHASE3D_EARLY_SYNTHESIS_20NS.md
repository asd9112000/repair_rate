# Phase 3D EARLY 20 ns Synthesis Record

## Intended views and frozen implementation

The Phase 3D flow retains the accepted Phase 3A/3B/3C methodology: DC
W-2024.09-SP2, TSMC018 `slow.db` at `slow`, `tsmc18_wl10`, 20.0 ns virtual
clock, zero I/O delay, and `compile -map_effort low`. GE is normalized by the
same `NAND2X1` reference cell, area 9.979200.

| View | Top | Expected observation |
|---|---|---|
| EARLY selector only | `recam_early_solution_selector` | Pure combinational; zero sequential cells. |
| Phase 3D integrated | `recam_phase3d_integrated_top` | Phase 3C scheduler/map, exactly one Phase 3B analyzer, and EARLY; only the Phase 3C 85 sequential cells expected. |

The integrated wrapper adds no state. EARLY consumes the stored map after all
four scans; it neither stops the scan early nor changes collector ownership.

## Accepted run

The successful rerun uses the methodology above and retains the Phase 3C
observable analyzer outputs at the integrated boundary. This prevents
cross-boundary removal of the analyzer path and makes the Phase 3C comparison
meaningful. Both `metadata.txt` files have terminal `STATUS=PASS`.

| View | Area | GE | Mapped / comb. / seq. cells | Comb./seq. area | Path | WNS/TNS | Closure |
|---|---:|---:|---:|---:|---:|---:|---|
| EARLY selector only | 967.982408 | 97.00 | 71 / 71 / 0 | 967.982408 / 0.000000 | 6.49 ns | +13.51 / 0.00 ns | PASS |
| Phase 3C integrated baseline | 72,289.325428 | 7,244.00 | 3,695 / 3,610 / 85 | 67,762.094970 / 4,527.230457 | 20.00 ns | 0.00 / 0.00 ns | PASS |
| Phase 3D integrated | 73,533.399055 | 7,368.67 | 3,784 / 3,699 / 85 | 69,006.168598 / 4,527.230457 | 20.00 ns | 0.00 / 0.00 ns | PASS |

Phase 3D versus Phase 3C: area is +1,244.073627 (+1.7210%), GE is +124.67
(+1.7210%), mapped and combinational cells are each +89, sequential cells are
unchanged at 85, combinational area is +1,244.073628, sequential area is
unchanged, and critical path is unchanged at 20.00 ns. The standalone selector
does not equal the integrated delta because optimization and port loading differ.

The selector-only critical path is `config_pattern_map_i[5]` to
`selected_config_id_o[2]`, 6.49 ns. The integrated critical path is
`pivot_valid_i[0]` to `current_pattern_id_o[2]`, 20.00 ns; it is analyzer
dominated, not an EARLY path. Integrated hierarchy identifies one
`role_aware_scheduler/shared_analyzer` (65,546.7126 area), scheduler/map local
logic (2,491.4736 combinational plus 4,527.2305 sequential area), and the
`early_selector` (967.9824 area).

## Required rerun and acceptance checks

The reproducible command writes separate reports under
`results/phase3d/dc_tsmc018_slow/phase3d_early_only_20ns` and
`results/phase3d/dc_tsmc018_slow/phase3d_early_integrated_20ns`, including
`area.rpt`, `timing.rpt`, `hierarchy_area.rpt`, `constraints.rpt`, tool log,
and `gate_count.rpt`. This accepted run confirms zero selector-only sequential
cells, no new integrated sequential state, and exactly one shared analyzer.
