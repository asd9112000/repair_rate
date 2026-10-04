# DSS final archive status

This archive is an immutable evidence index, not a development worktree.

| Evidence class | Status | Canonical use |
| --- | --- | --- |
| CA-LIVE six-case N2 packages | authoritative | final matched Continuous-Analysis PPA and policy latency |
| RECAM_N2 non-CA package | authoritative reference | non-CA area/context baseline only |
| CA-SB / no-scratch / ablation packages | historical | prototype or architecture-history comparison only |
| generated reports/netlists | evidence | read through their package provenance; do not relocate |

See [EVIDENCE_INDEX.md](EVIDENCE_INDEX.md) and [BRANCH_PROVENANCE.md](BRANCH_PROVENANCE.md) for controlled navigation.

R3 group-level formal evidence is separately closed and explicitly not a
hardware/archive replacement; see `../docs/date2026/r3/R3_FINAL_STATUS.md`.
