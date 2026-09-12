# Phase 3D EARLY Solution Take Policy

`recam_early_solution_selector` is a pure combinational four-entry priority
mux. It selects the first entry satisfying `valid && solution_valid &&
repairable`; map order is the frozen role-aware priority, never numeric ConfigID
order. A/D order is CFG0, CFG4, CFG5, CFG6; B/C is CFG0, CFG2, CFG1, CFG3.

It consumes the Phase 3C 80-bit map (four 20-bit entries) and returns solution
valid, map index, ConfigID, Phase-3B-selected PatternID, full 10-bit candidate
bitmap, and repairable. It has deterministic zero outputs if no entry qualifies.
EARLY runs only after Phase 3C `done_o`; it never terminates SCAN0–SCAN3 early.
The architecture is `Phase 3C four-config scan → ConfigPatternMap → Phase 3D
EARLY selector → selected solution`; selection does not compare resource
efficiency across later repairable entries.

ConfigPatternMap is candidate information; EARLY is local solution-take policy;
physical ownership/borrow feasibility is a later ledger stage. No GROUP,
resource arbitration, commit/rollback, snapshot, or change to Phase 3A–3C is
included. The frozen collector-state stability contract remains unchanged.
During SCAN0–SCAN3, collector Pivot, Must, Hybrid, and conventional-overflow
state is read-only and held stable by the upstream contract. EARLY adds no
snapshot registers, shadow CAM, or double buffering. Physical shared-spare
ownership, cross-subarray conflicts, reservation, and commit remain outside
Phase 3D.
