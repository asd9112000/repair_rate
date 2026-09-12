# Phase 3E GROUP_COMPRESSED Local Policy

`recam_group_compressed_solution_selector` is a pure combinational selector.
It accepts the completed four-entry (80-bit) Phase 3C `ConfigPatternMap` plus
`sa_role_i` (`0=A/D`, `1=B/C`), and returns solution valid, selected map index,
ConfigID, Phase-3B PatternID, ten-bit candidate bitmap, repairable, and the
two-bit score. There is no clock, register, FSM, additional analyzer pass, or
early scan termination. No qualifying entry produces deterministic zero outputs.

An entry participates only when `valid && solution_valid && repairable`. Its
frozen local pressure score is:

```text
GROUP_SCORE = 2 * Borrow + (1 - Release)
```

Lower is preferred: release without borrow (0), local (1), borrow plus release
(2), then pure borrow (3). If scores tie, the selector retains original map
order; it never uses numeric ConfigID order.

For A/D, ConfigID priority is `CFG4 → CFG0 → CFG6 → CFG5`; with the frozen
Phase 3C map `CFG0, CFG4, CFG5, CFG6`, this is map-index order
`1 → 0 → 3 → 2`. For B/C, ConfigID priority is `CFG1 → CFG0 → CFG3 → CFG2`;
with map `CFG0, CFG2, CFG1, CFG3`, this is map-index order `2 → 0 → 3 → 1`.

GROUP_COMPRESSED chooses only a preferred locally repairable configuration. It
preserves that entry's PatternID and candidate bitmap, does not rescan
candidates, and does not alter the source ConfigPatternMap. EARLY and GROUP
therefore have the same local solution-valid condition; GROUP does not expand
the local solution space. Its intended benefit is later improved group-level
feasibility when a PhysicalResourceLedger coordinates subarrays.

The Phase 3C collector contract remains frozen: Pivot, Must, Hybrid, and
conventional-overflow state remains read-only through all four scans; no
snapshot, shadow CAM, or double buffer is added. Physical shared-spare
ownership, conflicts, reservation, arbitration, commit, rollback, and fallback
selection are explicitly outside Phase 3E.

GROUP and EARLY consume the same completed ConfigPatternMap; GROUP invokes no
additional repair analysis. The Phase 3E 20 ns DC flow is defined separately in
[PHASE3E_GROUP_SYNTHESIS_20NS.md](PHASE3E_GROUP_SYNTHESIS_20NS.md).
