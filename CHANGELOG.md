# Changelog

## 2026-09-17 — R3 group derived-data plotting

- Recorded the group-only R3 repair-rate and fault-imbalance figure contract:
  five policy comparison views, metric-layered output directories, and
  combined/single-panel PDF/SVG/PNG exports.
- Fixed `BASELINE_RECAM` to a singleton tuple so policy-set concatenation no
  longer raises a Python `TypeError`.
- Added a lightweight plotting regression target covering policy membership,
  Rs2/Rs3 coverage, output variants/formats, and the current
  `QUICK_SWEEP / DEVELOPMENT / NON-FORMAL` fixture classification.
- Compactified R3 legend labels and moved dense `all`-view legends below the
  axes within the fixed canvas; the regression rejects legend overflow.
- These plotting changes do not alter simulator semantics, raw CSV schema, or
  device-level/global-CAM results.

## 2026-09-10 — Phase 3F/3G functional verification

- Added read-only 12-bit PhysicalResourceLedger snapshot legality/availability
  checking, SA/config resource-action decoding, and policy-aware physical
  fallback with deterministic donor selection.
- Passed Phase 3F/3G directed regressions and all Phase 3B–3E plus RECAM
  paper-faithful regressions. No synthesis, ledger mutation, commit, rollback,
  arbitration, or group scheduler was added.

## 2026-09-10 — Phase 3E GROUP_COMPRESSED functional verification

- Added pure combinational `recam_group_compressed_solution_selector` and its
  Phase 3C-map comparison testbench.
- Froze score `2*Borrow + (1-Release)`, A/D priority CFG4→CFG0→CFG6→CFG5, and
  B/C priority CFG1→CFG0→CFG3→CFG2, with map-order tie-breaking.
- Passed Phase 3E directed and EARLY-vs-GROUP tests plus all Phase 3B–3D and
  RECAM paper-faithful regressions. No synthesis, PhysicalResourceLedger, or
  physical resource ownership logic was added.
- Added reproducible 20 ns selector-only and Phase 3C+GROUP DC flows. The
  accepted run closes: selector-only is 1,127.649610 area / 113.00 GE / zero
  sequential cells; integrated is 73,693.066257 area / 7,384.67 GE / 85
  sequential cells, with 0.00 ns WNS/TNS.

## 2026-09-10 — Phase 3D EARLY synthesis preparation

- Added a production Phase 3D integration boundary and reproducible
  selector-only/integrated 20 ns Design Compiler flow.
- Preserved post-scan combinational EARLY semantics, stable collector state,
  one shared analyzer, and separation from later resource policy.
- Re-ran Phase 3D, Phase 3C, Phase 3B, and RECAM paper-faithful functional
  regressions successfully.
- Accepted 20 ns DC synthesis closes: selector-only is 967.982408 area / 97.00
  GE / zero sequential cells; integrated is 73,533.399055 area / 7,368.67 GE /
  85 sequential cells, with 0.00 ns WNS/TNS.
