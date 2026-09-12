# Phase 3A–3D RECAM RTL Handoff

> Current phase: **3D** · Status: **EARLY functional verification complete** · Next authorization: **NO**

## Frozen boundary

The active analyzer uses frozen 27-bit geometry: Domain3 + Bank2 + Group6 +
SA2 + Row9 + ColumnWord5. The analyzer boundary begins after collector/CAM
state, with `ROW_ADDR_W=9`, `COL_ADDR_W=5`, `DIFF_ADDR_W=9`, `MAX_K=5`, five
Address entries, and seven Hybrid entries. Raw reusable logical CAM accounting
is 280 bits and is not analyzer standard-cell area.

## Completed implementation

- Phase 3A: corrected 2R2C combinational analyzer; uncovered predicate is
  `pattern[row] && !pattern[col]`. Corrected 20 ns result: 50,877.288438 area,
  2,413 combinational cells, 0 sequential cells, 5,098.33 GE, WNS 0.00 ns.
- Phase 3B: one combinational ConfigID-controlled shared analyzer. Universal
  IDs are CFG0=2R2C, CFG1=2R1C, CFG2=3R2C, CFG3=3R1C, CFG4=1R2C, CFG5=2R3C,
  CFG6=1R3C. It remains 0 sequential cells. 20 ns: 67,229.870944 area,
  6,737.00 GE, WNS +0.02 ns.
- Phase 3C: `recam_role_aware_config_scheduler` evaluates the one shared
  analyzer four times and stores four 20-bit records (80-bit map). Role 0=A/D:
  CFG0→CFG4→CFG5→CFG6; role 1=B/C: CFG0→CFG2→CFG1→CFG3. Start E0, captures
  E1–E4, `done_o` pulses after E4. Start while busy is ignored. Integrated 20
  ns: 72,289.325428 area, 3,610 combinational + 85 sequential cells, 7,244 GE,
  WNS/TNS 0.00/0.00; hierarchy has exactly one shared analyzer.
- Phase 3D: `recam_early_solution_selector` is a pure combinational selector
  of the first `valid && solution_valid && repairable` record in map order. It
  outputs map index, ConfigID, Phase-3B PatternID, repairable, and full 10-bit
  candidate bitmap. It is consumed only after Phase 3C done and never stops a
  scan early.

## Mandatory collector contract

**The collector state is held stable while the role-aware scheduler evaluates
the four configurations of one subarray. No analyzer-state snapshot is
required.** Pivot CAM, Must metadata, Hybrid CAM, and conventional overflow
become read-only after collection completion through SCAN0–SCAN3; only after
Phase 3C done may they clear/reuse/advance. Full snapshots, shadow CAM, and
double buffers are explicitly excluded. If upstream cannot guarantee this,
stop before adding concurrent collection/analysis hardware.

## Re-run commands

```bash
bash scripts/simulation/run_recam_phase3d_functional.sh
bash scripts/simulation/run_recam_phase3c_functional.sh
bash scripts/simulation/run_recam_phase3b_functional.sh
make test_recam_paper_faithful
```

All last passed. Phase 3B includes 2,008-vector Phase3A/CFG0 golden equivalence
and CFG0–CFG6/MAX_K5/Hybrid[6]/transpose/address-packing/line-expansion tests.
Phase 3D covers each priority position, no-solution defaults, valid-bit
qualification, PatternID/bitmap alignment, and Phase3C→3D integration.

## Essential references

- [PHASE_STATUS.md](PHASE_STATUS.md)
- [PHASE3B_ANALYZER_INTERFACE.md](PHASE3B_ANALYZER_INTERFACE.md)
- [PHASE3_SYNTHESIS_27BIT.md](PHASE3_SYNTHESIS_27BIT.md)
- [PHASE3C_ARCHITECTURE.md](PHASE3C_ARCHITECTURE.md)
- [PHASE3C_SYNTHESIS_20NS.md](PHASE3C_SYNTHESIS_20NS.md)
- [PHASE3D_EARLY_ARCHITECTURE.md](PHASE3D_EARLY_ARCHITECTURE.md)

## Stop boundary

Do not modify Phase 3A/3B semantics, Phase 3C scan order, or collector contract.
Do not add pipeline, analyzer duplication, candidate serialization, early scan
termination, GROUP, PhysicalResourceLedger, shared resource ownership,
reservation, commit, rollback, or snapshot storage. Phase 3D synthesis is not
authorized or run. Historical Phase 2 `multi_config_early_selector` must not
be reused: it contains global resource/ledger policy that is outside local
four-entry EARLY semantics.
