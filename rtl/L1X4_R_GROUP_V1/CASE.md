# L1X4_R_GROUP_V1

Status: functionally closed; ready for Design Compiler; synthesis not started.

## Provenance

- Canonical case: `L1X4_R_GROUP_V1`
- Template: `rtl/g2x2_r_group`
- Pre-verification commit: `71aa103effc23afe9238c25ac476019a7ca3b411`
- Top module: `recam_dss_l1x4_r_static_global_top`
- C++ policy: `SolutionTakePolicy::Line1x4RowStaticGlobal`
- Analyzer changed from template: no

The implementation uses four subarrays arranged as the directed line
`A -> B -> C -> D`. Its static policy contains 27 ordered paths. Each
subarray contributes three dense analyzer results (1R2C, 2R2C, and 3R2C),
for 12 stored results collected over 12 cycles.

## Dense configuration contract

| Action | Dense configuration | RTL ConfigID |
| --- | --- | --- |
| RELEASE | 1R2C | 4 |
| LOCAL | 2R2C | 0 |
| RELEASE_BORROW | 2R2C | 0 |
| BORROW | 3R2C | 2 |

LOCAL and RELEASE_BORROW intentionally share the 2R2C dense result while
remaining distinct actions.

## Architecture delta from G2X2_R_GROUP

`G2X2_R_GROUP` implements a ring with 81 static paths. This case replaces
only topology-dependent selection and metadata with a directed line and 27
static paths. The analyzer, parameter/type packages, candidate store, dense
configuration universe, collection schedule, PatternID mechanism, and group
commit boundary are retained.

File classification against `rtl/g2x2_r_group`:

- BYTE_IDENTICAL: `dss_v2_group_candidate_store.sv`,
  `dss_v2_params_pkg.sv`, `dss_v2_types_pkg.sv`,
  `recam_shared_config_analyzer.sv`
- RENAMED_ONLY: `recam_dss_l1x4_r_static_global_top.sv`
- COMMENT_ONLY: `dss_v2_group_slot_decode.sv`
- TOPOLOGY_SPECIFIC_CHANGE: `recam_dss_l1x4_r_static_selector.sv`,
  `recam_dss_l1x4_r_static_global_core.sv`
- VERIFICATION_ONLY: `tb/L1X4_R_GROUP_V1/*.cpp`, `verification/*`

## Evidence

- `source_manifest.txt`: complete synthesizable source order
- `source_sha256.txt`: per-source SHA256 values
- `verification/static_path_table.csv`: canonical ordered 27-path table
- `verification/regression_summary.txt`: V2-V5 functional closure results

Design Compiler was not launched as part of functional closure.
