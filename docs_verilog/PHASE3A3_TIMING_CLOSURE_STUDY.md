# Phase 3A.3 — Complete Analyzer Timing Study

## Reproducible commands

Functional verification:

```bash
bash scripts/simulation/run_recam_phase3a.sh
```

Structural mapped-area baseline:

```bash
bash scripts/synthesis/run_recam_phase3a_dc.sh
```

Timing-driven Complete Analyzer point (replace `PERIOD` with `40`, `30`, `25`,
`20`, `15`, or `10`):

```bash
TIMING_DRIVEN=1 TIMING_DRIVEN_FAST=1 CLOCK_PERIOD_NS=PERIOD \
REPORT_TAG_PREFIX=timingPERIOD_fast_ DESIGN_LIST=recam_2r2c_analyzer \
TIMEOUT_SECONDS=300 bash scripts/synthesis/run_recam_phase3a_dc.sh
```

`TIMING_DRIVEN=1` applies the Phase 3A virtual CAM-state-Q to analyzer-output-D
constraints before compilation. `TIMING_DRIVEN_FAST=1` selects DC
`compile -map_effort low`; it is timing-driven optimization, not structural
mapping and not `compile_ultra`. No RTL registers are added.

## Artifact provenance

Each terminal run directory under `results/phase3a/dc_tsmc018_slow/` retains
`metadata.txt`, `dc_shell.log`, constraints, area/timing/QoR reports, netlist,
and `gate_count.rpt`. Only a final `STATUS=PASS`, `STATUS=FAIL`, or
`STATUS=TIMEOUT` is a result. A historical `STATUS=RUNNING` directory is not a
result and must not be used in tables.

## Recorded timing-driven Complete Analyzer points

| Target | Status | Critical path | WNS | Area | NAND2 GE |
|---:|---|---:|---:|---:|---:|
| 40 ns | PASS / closed | 39.97 ns | +0.03 ns | 21,857.775 | 2,190.33 |
| 30 ns | PASS / closed | 30.00 ns | +0.00 ns | 23,976.692 | 2,402.67 |
| 25 ns | PASS / closed | 25.00 ns | +0.00 ns | 26,325.130 | 2,638.00 |
| 20 ns | PASS / closed | 19.99 ns | +0.01 ns | 31,068.576 | 3,113.33 |
| 15 ns | PASS / closed | 14.99 ns | +0.01 ns | 30,755.895 | 3,082.00 |
| 10 ns | PASS / closed | 10.00 ns | +0.00 ns | 44,859.831 | 4,495.33 |

The 10 ns terminal result is `timing10_300s_fast_recam_2r2c_analyzer` and used
the 300-second limit. Structural baseline reports remain separately stored in
the unprefixed Phase 3A result directories.

## Conclusion and comparison use

The fully combinational Complete Analyzer demonstrated one-cycle timing closure
at every tested target from 40 ns through 10 ns under the current TSMC018
`slow.db` methodology.  It has zero analyzer sequential cells and no pipeline,
FSM, Pattern serialization, or Config serialization.  The analyzer boundary is
CAM logical-state outputs → Matrix Builder → Pattern Analyzer → Pattern Encoder
→ `PatternID`/`solution_valid`.

10 ns is the **minimum tested closed period**, not a demonstrated absolute
minimum achievable period.  It is an aggressive timing/area trade-off point:
10 ns closure requires 44,859.831 cell area and 4,495.33 NAND2 GE, substantially
above relaxed targets.  The primary Phase 3B comparison targets remain 20 ns
and 15 ns; this result does not automatically set the DSS comparison clock.
