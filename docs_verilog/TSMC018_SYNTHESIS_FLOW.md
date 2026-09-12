# TSMC 0.18 um synthesis flow for the multi-config analyzer

This flow is derived from `tmp/syn`, but uses the actual names and paths in the
installed CBDK instead of the stale `WCCOM` / `G5K` example settings.

## Tool environment

Interactive laboratory setup:

```csh
tcsh
source /cad/scripts/source_eda_tools.csh
# Enter: std
```

The automated runner sources `/cad/synopsys/CIC/synthesis.cshrc` directly in a
child `tcsh`.  This is equivalent for Design Compiler and avoids blocking on the
interactive multi-tool menu.  Verified tool: Synopsys Design Compiler
W-2024.09-SP2.

## Library and corner

```text
CBDK:    /cad/std_libraries/CBDK_TSMC018_Arm_f1.0/
Target:  CIC/SynopsysDC/db/slow.db
Library: slow
Corner:  slow
Wireload: tsmc18_wl10
```

The first Phase 2 run uses only the slow standard-cell library.  No memory
macro is inferred or linked by this analyzer RTL.

## First-pass constraints

```text
clock port:        clk_i
clock period:      20.0 ns
clock uncertainty: 0.1 ns
input delay:       0.0 ns (excluding clk_i and rst_ni)
output delay:      0.0 ns
input transition:  0.5 ns
output load:       0.05 library units
max transition:    3.0 ns
max capacitance:   0.15 library units
max fanout:        10
rst_ni:            false path
```

These are analyzer block-level comparison constraints, not package-pin timing.
They model adjacent upstream/downstream registers at the internal block
boundary. Any later change must be recorded with the resulting reports.

## Run

From the repository root:

```bash
scripts/synthesis/run_multi_config_phase2_dc.sh
```

The default timeout is 600 seconds. A longer controlled retry may be selected,
for example:

```bash
TIMEOUT_SECONDS=1200 scripts/synthesis/run_multi_config_phase2_dc.sh
```

The clock and interface assumptions are overridable without editing Tcl. For
example, the tighter performance variant is:

```bash
CLOCK_PERIOD_NS=10.0 INPUT_DELAY_NS=0.0 OUTPUT_DELAY_NS=0.0 \
REPORT_TAG=area_serial_conservative14_10ns_io0 \
scripts/synthesis/run_multi_config_phase2_dc.sh
```

Reports, netlist, constraints, SDF, DDC, metadata, and the complete DC log are
written under:

```text
results/phase2/dc_tsmc018_slow/<report-tag>/
```

The default report tag includes the compile mode. Set `REPORT_TAG` to preserve
multiple runs with different limits or implementation variants.

`COMPILE_MODE=mapped_only` uses DC `compile -exact_map` to produce a complete
standard-cell netlist and reports without timing/design-rule optimization. It
is the bounded-runtime synthesis baseline; it must not be described as timing
closure. `COMPILE_MODE=quick` and the default `ultra` retain optimization but
may require substantially more runtime for this combinational analyzer.

`COMPILE_MODE=mapped_no_drc` uses
`compile -exact_map -no_design_rule -map_effort low -area_effort none`. This is
the shortest technology-mapped reporting variant. It deliberately does not fix
max-transition, capacitance, or fanout violations and therefore must be labeled
as a mapped area/report-integration baseline, not DRC or timing closure.

`COMPILE_MODE=structural_report` performs standard-cell structural mapping
before it applies the clock/interface constraints, then runs post-map STA and
writes the normal report/netlist set. This avoids spending the bounded H13 run
in timing/design-rule optimization while still exposing the unoptimized timing
and design-rule violations. It is the default for the per-major-block runner:

```bash
scripts/synthesis/run_multi_config_phase2_blocks_dc.sh
```

That runner covers the conservative 14-entry collector, canonical analyzer,
EARLY, GROUP, PendingRepairBuffer, and final decoder. It also repeats the four
Hybrid-capacity-dependent blocks with the experimental 7-entry parameter.

Every invocation uses a distinct `REPORT_TAG` when results must be retained.
`metadata.txt` is authoritative: only a terminal `STATUS=PASS` is a synthesis
result. `RUNNING`, `TIMEOUT`, `FAIL`, partial elaboration statistics, and logs
that stop during mapping are diagnostic evidence only.

Files implementing the flow:

```text
scripts/synthesis/run_multi_config_phase2_dc.sh
scripts/synthesis/dc/run_dss_analyzer.tcl
scripts/synthesis/dc/dss_analyzer_sources.tcl
scripts/synthesis/dc/dss_analyzer_constraints.tcl
```

## Phase 2 synthesis result

The AREA architecture uses one fault collector, one canonical analyzer reused
over seven ConfigIDs and 40 ConfigID/Pattern cycles, and one Hybrid-aware final
decoder reused over four subarrays. The complete conservative-14 top structural
mapping is stored under:

```text
results/phase2/dc_tsmc018_slow/
  area_serial_pattern_conservative14_20ns_io0_structural_report/
```

It completed in about 140 seconds and reports 49,023 leaf cells, 10,418
sequential cells, and cell area 1,703,003.72. The per-major-block conservative
and experimental variants also have terminal `STATUS=PASS`.

This result is not timing closure. The post-map top report has WNS -169.97 ns,
TNS -54,129.51 ns, and 2,291 nets with design-rule violations. Timing-driven
optimization and any required pipeline partitioning remain future work.
