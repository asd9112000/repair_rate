# RTL synthesis scripts

The generic `synth.tcl` / `constraints.tcl` path provides Yosys structural
elaboration.  The technology-mapped Phase 2 Design Compiler flow is implemented
by `run_multi_config_phase2_dc.sh`; `run_multi_config_phase2_blocks_dc.sh`
generates conservative-14 and measured-7 reports for each major AREA block.
The associated source manifest, constraints, and DC Tcl live under `dc/`.
`parse_dc_phase2_reports.py` collects terminal PASS area/QoR reports into
`results/phase2/dc_tsmc018_slow/phase2_dc_results.csv`.

See `docs_verilog/TSMC018_SYNTHESIS_FLOW.md` for the verified tool, TSMC018
library, PVT corner, constraints, timeout, and result locations.

## Phase 3 frozen-geometry analyzers

`run_recam_phase3a_dc.sh` synthesizes the corrected Phase 3A analyzer and
`run_recam_phase3b_dc.sh` synthesizes the Phase 3B shared multi-Config
analyzer.  Both use the TSMC018 slow-corner methodology and reject a terminal
result containing sequential cells.  `TIMING_DRIVEN=1` is the closure flow;
`TIMING_DRIVEN=0` is an exact-map structural/post-map-STA diagnostic flow, not
a closure flow.  The retained 27-bit results and their interpretation are in
`docs_verilog/PHASE3_SYNTHESIS_27BIT.md`.

## Required gate-count reporting

Every future technology-mapped synthesis result retained in `results/` must
include both its normal area report and a `gate_count.rpt`.  The report must
state mapped leaf-cell count, combinational/sequential counts, and the
area-normalized NAND2X1 gate-equivalent (GE) count, including the exact
NAND2X1 library area used as the basis.  Do not label mapped standard-cell
count as literal gate count.  `write_gate_count_report.sh REPORT_DIR` derives
this report from terminal DC `area.rpt`, `qos.rpt`, and `references.rpt`.

## Phase 3D EARLY

`run_recam_phase3d_dc.sh` runs the 20 ns timing-driven TSMC018 slow-corner flow
for `recam_early_solution_selector` and `recam_phase3d_integrated_top`. The
latter contains exactly one Phase 3B shared analyzer through the Phase 3C
scheduler and adds combinational EARLY after the stored map. It writes separate
`phase3d_early_only_20ns` and `phase3d_early_integrated_20ns` directories below
`results/phase3d/dc_tsmc018_slow` without overwriting Phase 3C evidence. See
`docs_verilog/PHASE3D_EARLY_SYNTHESIS_20NS.md` for run state and acceptance checks.

## Phase 3E GROUP_COMPRESSED

`run_recam_phase3e_dc.sh` runs the same 20 ns TSMC018 slow-corner flow for the
pure `recam_group_compressed_solution_selector` and the primary
`recam_phase3e_group_integrated_top`. The integrated top is Phase 3C plus GROUP
only: it does not instantiate EARLY, a ledger, or another analyzer. Reports are
kept below `results/phase3e/dc_tsmc018_slow` without overwriting prior phases.
See `docs_verilog/PHASE3E_GROUP_SYNTHESIS_20NS.md` for status and acceptance checks.
