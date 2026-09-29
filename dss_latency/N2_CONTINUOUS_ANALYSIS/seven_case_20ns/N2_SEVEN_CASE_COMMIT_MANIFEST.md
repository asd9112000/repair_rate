# Proposed N2 seven-case commit manifest — not staged

## REQUIRED_THIS_COMMIT

- `rtl/G2X2_N2_R_*_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/` and
  `rtl/L1X4_N2_R_*_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/`: four completed
  CA-LIVE sibling implementations.
- `tb/n2_continuous_analysis/tb_ca_*` and `tb_*_live_*`: directed, full-top,
  persistence, and latency evidence harnesses used for closure.
- `dss_latency/N2_CONTINUOUS_ANALYSIS/N2_CA_LIVE_ANALYZER_WIDTH_PROVENANCE.md`,
  `N2_REMAINING_FOUR_*`, `remaining_four_latency/`, and `seven_case_20ns/`:
  permanent contracts, latency, source freeze, PPA, limitation, and status.
- `results/date2026/n2_remaining_four_ca_live_20ns/`: required raw 20 ns DC
  reports/netlists; retain without alteration.
- `dss_latency/.../remaining_four_latency/provenance/run_matched_ca_live_20ns.tcl`:
  permanent runner used for the four new synthesis runs.

## EXCLUDED_FROM_THIS_COMMIT

- `scripts/README_DATE2026_SWEEP.md`, `scripts/analysis/r3_group/`,
  `scripts/plot/r3_group/`, and `scripts/*group*sweep*`: unrelated repair-rate
  sweep work.
- `results/date2026/6case_1k/`, `results/date2026/repair_rate/group/`,
  `results/thesis/`, `figures/`, and the existing `g2x2_r_group` / `l1x4_r_group`
  result trees: different or pre-existing experiments.
- temporary build directories, virtual environments, and simulator object trees.

## NEEDS_USER_DECISION

- `my_note/DRAM_capacity.md` and `my_note/Estimate_area.md` are tracked
  deletions with no established relation to N2 CA-LIVE.
- `docs/dss_execution/DATE2026 — Transition from Legacy Policy Exploration to Six-Case Evaluation.md`
  has unclear relationship to this exact hardware closure.
- legacy untracked `rtl/G2X2_N2_RC_*_CONTINUOUS_ANALYSIS_reg/` and related
  result trees require confirmation whether they belong to a separate prior
  experiment or this consolidated commit.
