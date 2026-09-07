# Codex Phase Control Protocol

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

# PHASE 3 — RECAM Baseline

Implement RECAM architecture according to supplied paper.

Separate：

paper facts

implementation assumptions

Do not silently optimize the baseline.

Required：

RECAM top

fault collection storage

Address CAM

Hybrid CAM

matrix / repair-analysis path according to available specification

solution output

If paper does not provide enough implementation detail：

record it in 09_ASSUMPTIONS.md.

Exit Gate：

all implemented behavior traceable to paper or explicit assumption。

functional tests PASS。

synthesis PASS。

module area breakdown available。

---

# PHASE 4 — 2×2 DSS SRAM/FIFO

Reuse the Phase 2 DSS repair algorithm.

Replace storage/search backend with：

FIFO

SRAM-like storage

P-way comparator search engine

controller

Do not redesign DSS repair policy.

Required configurations：

P = 1

P = 2

P = 4

P = 8

where legal for ENTRY_NUM.

Required correctness：

For identical fault input and DSS policy：

DSS-CAM functional repair output

must equal

DSS-SRAM/FIFO functional repair output.

Latency may differ.

Exit Gate：

functional equivalence PASS。

FIFO behavior PASS。

all selected P configurations PASS。

synthesis PASS。

---

# PHASE 5 — Unified Verification

Build common regression.

Same traces run through：

RECAM

DSS-CAM

DSS-SRAM/FIFO

For DSS-CAM vs DSS-SRAM：

check functional equivalence.

For RECAM vs DSS：

do not require identical solutions if algorithms differ.

Instead compare：

repair result

hardware cost

analysis latency

under defined experiment conditions.

Exit Gate：

regression reproducible。

all configurations tracked。

no unexplained mismatch。

---

# PHASE 6 — Synthesis Sweep

Only after Phase 5.

Sweep primary parameters：

ENTRY_NUM

P

spare configuration where required.

Output：

synthesis_results.csv

latency_results.csv

module_area_breakdown.csv

Do not change RTL algorithm during sweep.

Every failed synthesis configuration must be recorded instead of silently dropped.

Exit Gate：

all CSV schema validated。

all results reproducible from scripts。

area and latency scaling sanity-checked。

---

# PHASE 7 — Pareto and Selective P&R

Use synthesis results to identify representative points：

smallest-area

lowest-latency

balanced Pareto point

RECAM baseline

DSS-CAM baseline

Only selected points proceed to P&R.

P&R is validation rather than exhaustive sweep.

Compare synthesis vs post-layout trends.

---

# PHASE 8 — 1×4 Extension

Only after DATE primary hardware evaluation is stable.

Reuse：

storage primitive

synthesis framework

verification framework

results schema

Extend DSS architecture to 1×4 row-only.

This Phase must not delay DATE 2×2 results.
