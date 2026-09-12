# Codex Phase Control Protocol

## Multi-Config analyzer scope note

For the completed Phase 2 2×2 Directional Multi-Config Analyzer, detailed
implementation decisions in
[03_ANALYZER_RTL_HANDOFF.md](03_ANALYZER_RTL_HANDOFF.md) remain historical
evidence.  They do not authorize Phase 3 implementation.  In particular, Phase
2 must not be read as authorizing per-configuration Address/Hybrid CAM
replication or as replacing the existing separate 1×4 extension scope.

## Authority

Codex 不得自行進入下一 Phase。

每完成一個 Phase：

1. 執行該 Phase verification。
2. 更新 PHASE_STATUS。
3. 提交修改檔案清單。
4. 回報 unresolved issues。
5. 回報 exit-gate checklist。
6. STOP。

只有收到明確：

ADVANCE TO PHASE X

才能開始新的 Phase。

---

# PHASE 0 — Repository Audit and Specification Freeze

Allowed:

- inspect repository
- understand existing simulator
- identify 2×2 DSS implementation
- inspect existing Verilog
- inspect scripts
- read RECAM reference material
- 建立 specification documents
- 建立 directory structure

Required documents:

PHASE_STATUS.md

03_DSS_2X2_SPEC.md

04_RECAM_SPEC.md

05_STORAGE_MICROARCH.md

06_VERIFICATION_PLAN.md

07_SYNTHESIS_PLAN.md

08_RESULTS_SCHEMA.md

09_ASSUMPTIONS.md

DECISION_LOG.md

Forbidden:

- complete analyzer RTL implementation
- algorithm modification
- large parameter sweep

Exit Gate:

2×2 DSS behavior 已被文件化。

RECAM 已列出 paper-confirmed information 與 assumptions。

common interface 已定義。

comparison parameters 已定義。

---

# PHASE 1 — Storage Primitive Validation

Implement only:

generic CAM

SRAM-like storage

FIFO

P-way comparator

search controller

Required tests:

CAM write/read/search

SRAM write/read

FIFO full/empty

P-way search correctness

different ENTRY_NUM

different P

Perform small synthesis sanity test.

Purpose:

確認 synthesis methodology 能反映：

storage

comparator

priority logic

controller

的成本。

Forbidden:

完整 DSS analyzer。

完整 RECAM analyzer。

Exit Gate:

所有 primitive unit tests PASS。

primitive synthesis PASS。

parameterization PASS。

area report 可以被 parser 正確讀取。

---

# PHASE 2 — 2×2 DSS-CAM

This is the DATE primary implementation.

Implement：

2×2 DSS analyzer using CAM storage.

Recommended modules：

dss_2x2_cam_top

fault_collector

fault_cam

candidate_analyzer

sharing_controller

resource_tracker

solution_selector

solution_store

Preserve existing 2×2 repair algorithm.

Required verification：

RTL output vs existing simulator / golden traces.

At least：

normal case

single fault

row-dominant faults

column-dominant faults

sharing-required case

unrepairable case

boundary spare-resource case

Exit Gate：

functional regression PASS。

solution matches golden model。

cycle counter works。

synthesis PASS。

module area breakdown available。

---

# PHASE 3A — RECAM 2R2C Analyzer

Implement only the fully combinational `RS=2`, `CS=2`, `K=4` analyzer path:

```text
CAM logical state → Matrix Builder → 4×4 matrix →
6 parallel candidate evaluators → Pattern Encoder → PatternID
```

Keep separate synthesis boundaries for `A_RECAM_MATRIX`, `A_RECAM_PATTERN`,
`A_RECAM_ENCODER`, and `A_RECAM_COMPLETE`.  The complete analyzer has an
expected sequential-cell count of zero.  Do not serialize candidate evaluation
or change the architecture to address timing.

Exit Gate:

- Functional behavior is traceable to RECAM specification or an explicit
  assumption.
- Matrix Builder, Pattern Analyzer, Pattern Encoder, and complete analyzer
  have separate reports.
- Candidate evaluation is parallel and sequential-cell count is zero.
- Report area, critical path, slack, and estimated Fmax, whether timing passes
  or fails.

---

# PHASE 3B — Shared Multi-Config Analyzer

Implement one combinational Shared Config Analyzer for `2R2C`, `2R1C`,
`3R2C`, `3R1C`, `1R2C`, `2R3C`, and `1R3C`.  Apply canonical R/C transpose
relationships where appropriate.  Do not instantiate one complete analyzer per
`ConfigID`; candidates within each selected `ConfigID` remain parallel.  The
historical 40-cycle Pattern-serial architecture is not authorized for this
workstream.

Exit Gate:

- One `A_SHARED_CONFIG_ANALYZER` combinational datapath is structurally
  identifiable.
- All seven configurations produce the required `PatternID` mapping.
- No candidate serialization or per-ConfigID complete-analyzer replication.

---

# PHASE 3C — Serial Config Analysis Engine

Add sequential control around, but not inside, the Shared Config Analyzer:

```text
Config Scheduler (sequential) → ConfigID → Shared Config Analyzer
(combinational) → PatternID → ConfigPatternMap (sequential)
```

Analyze CFG0 through CFG6 in one configuration per cycle: seven
configuration-analysis cycles per subarray.  Keep `A_CONFIG_SCHEDULER` and
`A_CONFIG_PATTERN_MAP` separately measurable.

Exit Gate:

- The observed schedule is exactly seven ConfigID cycles per subarray.
- ConfigPatternMap contains all seven completed results.
- Scheduler, combinational analyzer, and result storage remain separate RTL
  and reporting boundaries.

---

# PHASE 3D — EARLY

Use a completed ConfigPatternMap: analyze all seven configurations first, then
invoke EARLY selection.  Streaming early termination is out of scope.  Report
`A_EARLY` separately.

Exit Gate:

- EARLY consumes only a complete seven-result map.
- EARLY overhead has a separate functional and synthesis report.

---

# PHASE 3E — GROUP

After all four subarrays complete configuration analysis, feed their four
seven-entry maps to a sequential GROUP Selector FSM and separately measurable
PhysicalResourceLedger.  Report `A_GROUP_CONTROLLER` and `A_RESOURCE_LEDGER`
separately.

Exit Gate:

- GROUP starts only after all four ConfigPatternMaps are complete.
- GROUP controller and resource ledger are separately measurable.

---

## Historical downstream phases

The former Phase 4–8 SRAM/FIFO, unified-verification, sweep, P&R, and 1×4
extension plan is retained as historical planning context only.  It has no
execution authority until it is re-planned after Phase 3E and explicitly
authorized.
