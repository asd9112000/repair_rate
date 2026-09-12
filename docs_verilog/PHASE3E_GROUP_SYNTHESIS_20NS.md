# Phase 3E GROUP_COMPRESSED 20 ns Synthesis Record

The authorized Phase 3E flow uses the accepted Phase 3A–3D methodology: DC
W-2024.09-SP2, TSMC018 `slow.db` at `slow`, `tsmc18_wl10`, 20.0 ns virtual
clock, zero I/O delay, `compile -map_effort low`, and NAND2X1 area-normalized
GE. It has two primary views:

| View | Top | Expected state |
|---|---|---|
| GROUP selector only | `recam_group_compressed_solution_selector` | Pure combinational; zero sequential cells and area. |
| GROUP integrated | `recam_phase3e_group_integrated_top` | Phase 3C scheduler/map + exactly one shared analyzer + GROUP; 85 sequential cells from Phase 3C only. |

The primary integrated top deliberately excludes EARLY. It retains Phase 3C
observable analyzer outputs so its analyzer timing boundary is comparable to
the accepted Phase 3C and Phase 3D records.

## Accepted run

Both runs have terminal `STATUS=PASS`. The standard-cell references in the
selector-only report contain AOI/OAI/NAND/NOR/buffer/inverter logic only; there
is no arithmetic macro or synthetic operator. The fixed score has therefore
mapped to optimized decode/priority logic, rather than an explicit multiplier
or adder.

| View | Area | GE | Mapped / comb. / seq. cells | Comb. / seq. area | Path | WNS/TNS | Closure |
|---|---:|---:|---:|---:|---:|---:|---|
| GROUP selector only | 1,127.649610 | 113.00 | 83 / 83 / 0 | 1,127.649610 / 0.000000 | 9.55 ns | +10.45 / 0.00 ns | PASS |
| Phase 3C baseline | 72,289.325428 | 7,244.00 | 3,695 / 3,610 / 85 | 67,762.094970 / 4,527.230457 | 20.00 ns | 0.00 / 0.00 ns | PASS |
| Phase 3D EARLY integrated | 73,533.399055 | 7,368.67 | 3,784 / 3,699 / 85 | 69,006.168598 / 4,527.230457 | 20.00 ns | 0.00 / 0.00 ns | PASS |
| Phase 3E GROUP integrated | 73,693.066257 | 7,384.67 | 3,796 / 3,711 / 85 | 69,165.835800 / 4,527.230457 | 20.00 ns | 0.00 / 0.00 ns | PASS |

### GROUP incremental overhead versus Phase 3C

| Metric | Delta |
|---|---:|
| Area | +1,403.740829 (+1.9418%) |
| GE | +140.67 (+1.9419%) |
| Mapped / combinational cells | +101 / +101 |
| Sequential cells / area | 0 / 0.000000 |
| Critical path | 0.00 ns |

### GROUP versus EARLY integrated policy cost

| Metric | Delta |
|---|---:|
| Area | +159.667202 (+0.2171%) |
| GE | +16.00 (+0.2171%) |
| Mapped / combinational / sequential cells | +12 / +12 / 0 |
| Critical path | 0.00 ns |

GROUP selector only versus EARLY selector only is +159.667202 area, +16.00 GE,
+12 mapped cells, and +3.06 ns path (9.55 ns versus 6.49 ns). The standalone
and integrated deltas need not match because of cross-boundary optimization and
port loading.

The GROUP-only critical path is `config_pattern_map_i[4]` to
`selected_config_id_o[2]`, 9.55 ns. The integrated critical path is
`pivot_valid_i[0]` to `current_pattern_id_o[2]`, 20.00 ns, so it is
analyzer-dominated rather than GROUP-selector-dominated. Integrated hierarchy
shows exactly one `role_aware_scheduler/shared_analyzer` (65,546.7126 area),
scheduler/map local logic (2,491.4736 combinational plus 4,527.2305 sequential
area), and `group_selector` (1,127.6496 area).

The reproducible command writes separate report sets in
`phase3e_group_only_20ns` and `phase3e_group_integrated_20ns`, including area,
timing, hierarchy, constraints, tool log, and NAND2 GE report. Inspect zero
selector-only sequential cells, 85 integrated sequential cells, one shared
analyzer, actual critical path classification, and whether fixed score mapping
has optimized to decode/priority logic rather than arithmetic. This accepted
run confirms each of these observations.
