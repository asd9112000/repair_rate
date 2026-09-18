# DSS Thesis Execution Master Evidence

> 文件狀態：Current
> 適用範圍：DSS thesis execution evidence index
> 建立時間：2026-09-13T06:28:23+08:00
> 最後修改時間：2026-09-14T04:40:00+08:00

This index records evidence pointers.  It deliberately does not duplicate or
reinterpret accepted DATE 2026 DSS V2 numeric results.

| Evidence subject | Current authoritative source / location |
|---|---|
| Frozen baseline evidence | `docs_verilog/PHASE4E_TO_4K_MASTER_EVIDENCE.md` and `docs_verilog/PHASE_STATUS.md` |
| RTL policy/interface authority | `docs_verilog/01_COMPARISON_CONTRACT.md`, `02_PHASE_CONTROL.md`, and `PHASE3B_ANALYZER_INTERFACE.md` |
| Simulator architecture and scope | `docs/ARCHITECTURE.md`, `RECAM_SPEC.md`, `EXPERIMENTS.md`, `REPORTS.md` |
| Current source-code families | `rtl/dss_v2/`, `rtl/recam/`, `rtl/dss_2x2/analyzer/`, `src/`, `tests/`, `tb/` |
| Current formal experiment workflow | `docs/EXPERIMENT_CATALOG.md`; DATE-2X2 manifest/runner/analyzer recorded in the execution master |
| Output scopes | C++ `reports/`; RTL verification/synthesis `results/`; execution evidence `docs/dss_execution/` |
| Synthesis provenance sources | `scripts/synthesis/run_recam_phase4e_dc.sh`, `run_recam_phase4f_dc.sh`, their `dc/` manifests, and `results/phase4e/` / `results/phase4f/` reports |
| Known verified parameter point | DSS V2 2×2 Directional CAM, `RS=CS=2`, `SHARE_M=1`, EARLY and GROUP-NoScratch |
| Known future target | 2×2 Directional CAM, `RS=CS=3`, `SHARE_M=1`, EARLY and GROUP-NoScratch |

Future accepted H0/H1/H2/... evidence is added here only after its respective
phase completes and is explicitly accepted.

## H0/S0 evidence

| Phase | Artifact | Accepted boundary finding |
|---|---|---|
| H0 | `H0_REUSE_PARAMETERIZATION_AUDIT.md` | Four-resource m=1 ledger is reusable; analyzer/configuration state is fixed to 2,2,1 and requires H1/H2 target derivation. |
| S0 | `S0_SIMULATOR_EXPERIMENT_CONTRACT.md` | Existing DATE group workflow is an extension base, but its GroupCompressed selector needs semantic calibration before any V2 GROUP-NoScratch claim. |
| H1 | `H1_RS3_CS3_M1_CONFIG_SPACE.md` | 3,3,1 has seven role-union envelopes, four role slots, Kmax=7, maximum 35 patterns, 6-bit one-based PatternID, a 35-bit bitmap, and compatible four-resource m=1 actions. |
| H2/H2R | `H2_RS3_CS3_M1_HARDWARE_SIZING.md` | Target analyzer envelope is 7 Address/pivot entries, 17 Hybrid entries, a 7x7 matrix, and a 35-candidate/6-bit PatternID interface; minimum GROUP-NoScratch history is 112 bits and m=1 ledger is unchanged. Logical 17/14-bit entries are distinguished from 28/20-bit mode-reused physical entries. H2-CL-005 is resolved as a stale execution gate. |
| H3 | `H3_RS3_CS3_M1_RTL_IMPLEMENTATION.md` | Isolated target EARLY and GROUP-NoScratch RTL implements the H1 table, K=7 analyzer interface, 35-candidate/6-bit result, and 112-bit GROUP history while reusing the four-resource ledger. Target structural smoke and frozen 2,2,1 protection regression pass; H4 independent functional closure remains required. |
| H4 | `H4_RS3_CS3_M1_FUNCTIONAL_VERIFICATION.md` | Independent C++ golden matched isolated target EARLY and GROUP-NoScratch for 1000 vectors each at seed 20260910 with zero mismatches. Seven configurations, thresholds, transpose classes, 112-bit history, directed resource/failure cases, and frozen 2,2,1 regression pass. |
| H5 | `H5_RS3_CS3_M1_HARDWARE_CHARACTERIZATION.md` and `results/dss_v2_rs3cs3m1/h5_hardware_retry2/hardware_characterization.csv` | Original H4-verified 3,3,1 EARLY and GROUP-NoScratch completed accepted-methodology DC mapping. Both target points miss 20-ns timing; the result records actual area, GE, state, hierarchy, and timing rather than treating synthesis completion as timing closure. |

## Roadmap boundary (not evidence)

The execution control documents now distinguish independent group-scope
DynamicSpareSharing evidence from future device-scope HierarchicalRECAM
evidence.  D0–D3 are proposed, not authorized, and contribute no current
numerical result or thesis claim.  `H0S0-CL-004` remains open: existing
GroupCompressed simulator behavior is not frozen V2 GROUP-NoScratch.
