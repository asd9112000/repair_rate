# Phase 3D EARLY Functional Verification

`bash scripts/simulation/run_recam_phase3d_functional.sh` passes CFG0-first,
second/third/fourth-entry selection, no-solution defaults, valid-bit
qualification, PatternID/ConfigID alignment, and full candidate-bitmap
propagation. The Phase 3C→Phase 3D integration scan passes: four results are
captured before EARLY selects map entry zero. Phase 3C, Phase 3B, and
paper-faithful regressions were also rerun and pass. Following the Phase 3D
synthesis-wrapper addition, these regressions were rerun on 2026-09-10 and
passed. The wrapper does not alter Phase 3A/3B/3C or make EARLY an early-stop
scheduler. The subsequent selector-only and integrated 20 ns DC runs close;
see [PHASE3D_EARLY_SYNTHESIS_20NS.md](PHASE3D_EARLY_SYNTHESIS_20NS.md).
