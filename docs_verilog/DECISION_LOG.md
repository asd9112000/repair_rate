# RTL Decision Log

本文件只記錄會影響 RTL architecture、verification、synthesis methodology、目錄邊界
或結果解讀的決策。工作進度由 [PHASE_STATUS.md](PHASE_STATUS.md) 管理；C++ simulator
的 architecture semantics 仍以 `docs/` 中的 Current 文件為準。

新增決策時應保留既有紀錄，並包含日期、狀態、背景、決策、理由、影響與驗證方式。

---

## D-002 — Use a C-shell terminal for laboratory EDA environments

- Date: 2026-09-08
- Status: ACCEPTED
- Scope: RTL simulation and synthesis tool invocation

### Context

實驗室的 `/cad/scripts/source_eda_tools.csh` 是互動式 C-shell 腳本。它使用
`set choice = $<` 讀取 tool 選單輸入；若從 Bash（包括預設的 VS Code Remote SSH
terminal）執行，會出現 Bash syntax error，且不會載入 VCS、Design Compiler 等環境。

### Decision

- 在 MobaXterm 或 VS Code Remote SSH 中，先進入 `tcsh -l`（沒有 `tcsh` 時使用
  `csh -l`），再執行：

  ```csh
  source /cad/scripts/source_eda_tools.csh
  ```

- 在選單中輸入 `std` 載入 standard-cell tool 環境。
- VS Code 的整合 terminal 預設 profile 應設為 `/bin/tcsh`、arguments 為 `-l`，
  而不是從 Bash `source` 此 `.csh` 檔。
- 所有使用該環境的 EDA command 必須在同一個 csh/tcsh process 或其 child process
  執行；環境變數不會回傳至原本的 Bash process。

### Rationale

shell 語法與環境變數 scope 是載入 toolchain 的前提。明確指定 shell 可讓 VS Code 與
MobaXterm 得到一致的 tool selection behavior，並避免把 shell parse error 誤判為
EDA tool 安裝或 RTL 問題。

### Consequences

- 需要 VCS、Design Compiler、PrimeTime 等 lab tool 的 script 應由 csh/tcsh launcher
  呼叫，或在已載入環境的 terminal 直接執行。
- 開源 Verilator/Yosys validation 不依賴此選單，仍可在一般 Bash workflow 執行。

### Verification

- 在 `tcsh -l` 中成功載入選單並選擇 `std`。
- 在同一 terminal 以 `command -v vcs`、`command -v dc_shell` 確認所需 tool。

---

## D-001 — Separate RTL work from C++ simulation

- Date: 2026-09-07
- Status: ACCEPTED
- Scope: repository organization and artifact ownership

### Context

既有 repository 已使用 `src/`、`inc/`、`tests/`、`scripts/` 與 `reports/` 執行
C++ repair-rate／architecture simulation。RTL 工作需要獨立的 source、testbench、
tool automation 與 results boundary，避免 analytical model 與硬體 measurement 混用。

### Decision

- Synthesizable hardware source 放在 `rtl/`。
- RTL testbench 放在 `tb/`；既有 `tests/` 僅保留 C++／Python simulator tests。
- 既有 C++ runner 保留在 `scripts/{legacy,group,device}/`。
- RTL automation 使用 `scripts/{lint,simulation,synthesis}/`；其中
  `scripts/simulation/` 只代表 HDL/RTL simulation。
- C++ simulation output 保留在 `reports/`；RTL synthesis 與 cycle-accurate output
  放在 `results/`。
- `docs/` 維護 simulator semantics；`docs_verilog/` 維護 RTL project contract、
  phase control、status、assumptions 與 decisions。

### Rationale

兩條工作流的驗證層級與結果語意不同。目錄隔離可防止 C++ work-cycle／area proxy
被誤認為 RTL timing／synthesis area，也避免 RTL implementation 反向改寫既有 repair
algorithm。

### Consequences

- 跨邊界交換必須使用固定 fault trace、golden output 或有版本的 CSV schema。
- RTL result 回灌 C++ simulator 前，必須保留 tool、library、constraint、configuration、
  commit 與原始 log provenance。
- 這項目錄決策不修改任何 legacy、group-level 或 hierarchical-device simulation
  semantics，也不移動既有 simulation results。

### Verification

- Repository boundary inspection
- `git diff --check`

---

## 2026-09-08

## D-003 — 2×2 Directional Multi-Config Analyzer retained-state architecture

- Date: 2026-09-08
- Status: ACCEPTED
- Scope: 2×2 directional analyzer hardware and simulator/RTL contract

### Context

The prior 2026-09-07 solution-take design retained matrix row/column address
dictionaries and a valid-solution bitmap after RECAM analysis.  That is not
the desired retained-state architecture for the new 2×2 multi-configuration
analyzer, which must evaluate resource envelopes without replicating complete
fault-information CAMs.

### Decision

- Define seven physical `ConfigID`s: `2R2C`, `2R1C`, `3R2C`, `3R1C`, `1R2C`,
  `2R3C`, and `1R3C`; implement four canonical R/C-symmetric analyzer classes.
- Keep `PatternID` separate from `ConfigID`; retain the lowest-index valid
  PatternID as the deterministic representative for each configuration.
- Use shared pivot/fault-information storage and shared physical fault counts;
  derive configuration-specific Must conditions by threshold decode.
- Retain ordered `PivotPayload`, `ConfigPatternMap`, and `CAMReusePending`.
  The ConfigID is implicit in the result-map coordinate.
- Use shared/tagged Hybrid storage.  Its final physical capacity is
  measurement-driven by configuration-union occupancy.
- Keep additional Address-CAM-overflow pivots on the temporary CAM-reuse path,
  outside conventional PatternID analysis.
- Keep `PhysicalResourceLedger` as the single physical-spare authority, and
  stage EARLY/GROUP selections in pending repair state until group success.

### Rationale

This separates expensive shared physical fault payload from configuration
interpretation while preserving RECAM candidate validity and directional
resource conservation.

### Consequences

This supersedes the conflicting 2026-09-07 retained
`matrixRowAddresses[]`/`matrixColumnAddresses[]`/`validSolutionBitmap`
decision **for the new 2×2 analyzer only**.  The historical decision remains
valid as historical context and for scopes that explicitly retain that older
compressed representation.  It does not supersede the independent 1×4 work.

### Verification

- Simulator/RTL traces compare PivotPayload, Must decode, PatternID,
  CAMReusePending, selected ConfigID/PatternID, and final decode.
- Synthesis reports retain separate shared-storage, analyzer, selector, ledger,
  and pending-buffer measurements.

---

## D-004 — Retain tagged Hybrid/dictionary delta until GROUP finalization

- Date: 2026-09-08
- Status: ACCEPTED
- Scope: final conventional repair reconstruction for the 2×2 directional multi-config analyzer

### Context

The `must_hybrid_group` simulator golden has faults `(3,9)` and `(4,9)`.
For CFG0/PatternID 1, RECAM extends its matrix row dictionary through the
Hybrid record and selects repair rows 3 and 4.  A PivotPayload containing only
the physical pivot `(3,9)` cannot reconstruct row 4 from ConfigID and
PatternID alone.

### Decision

- Retain one tagged Hybrid/dictionary delta snapshot per subarray from analysis
  completion until the complete EARLY or GROUP result is committed.
- A delta snapshot contains shared Hybrid row/column payload, pivot pointer,
  descriptor, ConfigID-valid mask, and the derived per-ConfigID RowMust and
  ColumnMust masks needed to replay matrix-dictionary construction.
- After a ConfigID is selected, final decode replays only that ConfigID's tagged
  view and emits conventional repair lines from the reconstructed dictionaries.
- The snapshot is copied into PendingRepairBuffer only on whole-group success;
  it is discarded on failure.

### Rationale

This preserves existing RECAM golden behavior and the shared-CAM principle.
The retained payload is one physical/tagged snapshot per subarray, rather than
one Address/Hybrid CAM pair or a decoded repair map per ConfigID.

### Consequences

- `PivotPayload + ConfigID + PatternID` is insufficient by itself whenever
  Hybrid records extend a dictionary; final decoding also consumes the retained
  tagged Hybrid/dictionary delta.
- The delta is temporary group state, not a permanent eFuse encoding and not a
  conventional PatternID field.
- Hybrid payload cannot be released immediately after per-subarray analysis
  under GROUP policy.

### Verification

- The `(3,9),(4,9)` golden must emit rows 3 and 4 for CFG0/PatternID 1.
- ConfigPatternMap, EARLY/GROUP selection, CAM-reuse separation, and no-
  duplicate-CAM structural checks remain unchanged.

---

## D-005 — TSMC018 first-pass synthesis corner and constraints

- Date: 2026-09-08
- Status: ACCEPTED
- Scope: Phase 2 technology-mapped synthesis methodology

### Context

The reference files under `tmp/syn` use a generic 18 ns example and stale
library/operating-condition names.  The installed library is under
`/cad/std_libraries/CBDK_TSMC018_Arm_f1.0/`, and the current Phase 2 structural
metadata uses a 10 ns clock.

### Decision

- Use Synopsys Design Compiler W-2024.09-SP2 from the laboratory tcsh setup.
- Use `CIC/SynopsysDC/db/slow.db`, library/corner `slow`, and wire-load model
  `tsmc18_wl10` for the first technology-mapped measurement.
- Use a 10 ns `clk_i`, 0.1 ns uncertainty, 5 ns input/output delays, 0.5 ns
  input transition, and 0.05 output load.  Treat `rst_ni` as a false path.
- Preserve complete logs, constraints, metadata, reports, and generated
  netlists under `results/phase2/dc_tsmc018_slow/`.
- Use a 600-second first-run limit and record timeout as TIMEOUT, never PASS.

### Rationale

The selected names exist in the installed CBDK and avoid silently falling back
from the stale `WCCOM`/`G5K` settings.  The constraint set stays comparable to
the project metadata while following the reference flow's half-cycle interface
budget.

### Consequences

The conservative14 parallel seven-view design completed DC analyze, elaborate,
link, library analysis, and constraint checks but timed out during
`compile_ultra`.  Its initial size (about 252,548 leaf cells and 9,276 sequential
cells) is evidence for H14 canonical time-multiplexing; it is not a final area or
timing result.

### Verification

- `check_design_before_compile.rpt` contains no `Error`, unresolved reference,
  black box, latch, or combinational-loop report.
- Run provenance and terminal status are recorded in `metadata.txt`.

---

## D-006 — Time-multiplex canonical analysis and final Hybrid decode

- Date: 2026-09-08
- Status: ACCEPTED
- Scope: Phase 2 H14 AREA implementation

### Context

The parallel implementation instantiated seven physical configuration views
and four per-subarray Hybrid-aware final decoders. Its first DC run did not
complete `compile_ultra` within 600 seconds. Both structures duplicate
combinational interpretation logic; neither duplication is required to retain
the shared physical fault payload.

### Decision

- Preserve the original parallel analyzer as the compatibility variant.
- Add an AREA variant with one canonical analyzer reused across seven logical
  ConfigID views. CFG4, CFG5, and CFG6 use complete logical R/C transpose.
- Convert canonical candidate ordering back to physical PatternID ordering
  before retaining the physical lowest PatternID.
- Retain one group-level Hybrid-aware final decoder and time-multiplex it over
  the four persistent PendingRepairBuffer records in A-to-D order.
- Assert final-program validity only after all four physical mappings decode
  successfully.

### Rationale

This removes per-ConfigID analyzer duplication and per-subarray final-decoder
duplication without changing RECAM validity, D-004 retained data,
ConfigPatternMap contents, or physical repair addresses.

### Consequences

- Per-subarray analysis originally took seven analyzer cycles after input
  snapshot; D-009 supersedes this AREA latency with a 40-cycle Pattern-serial
  schedule.
- Final conventional reconstruction takes four decode cycles after the pending
  payload becomes available.
- AREA synthesis sets `PARALLEL_ORIENTATION=0`; the default remains the
  parallel compatibility behavior.

### Verification

- Dedicated AREA-engine tests cover all seven physical PatternIDs and overflow.
- Parallel and AREA tops pass the same simulator-generated golden cases,
  including Hybrid dictionary extension.
- DC elaboration confirms one canonical analyzer and one Hybrid-aware final
  decoder in the AREA variant; the pre-cleanup AREA top reported approximately
  60,924 leaf cells and 10,417 sequential cells.

---

## D-007 — Exact-width scratch state and matrix-free final reconstruction

- Date: 2026-09-09
- Status: ACCEPTED
- Scope: Phase 2 H14 post-correctness optimization

### Context

The canonical analyzer and final Hybrid decoder used SystemVerilog `integer`
variables for values bounded by `MAX_K=5` or ten candidate patterns. Design
Compiler consequently introduced many 32-bit DesignWare adders. The final
decoder also rebuilt a 5x5 candidate-validity matrix even though the selected
PatternID had already passed analyzer validation and the matrix was never read.

### Decision

- Keep loop indices as compile-time integers, but represent active dimensions,
  dictionary occupancies, dictionary indices, and candidate indices with their
  exact 3-bit or 4-bit widths.
- Return row/column requirements from EARLY/GROUP helper functions at
  `SPARE_W`, rather than as 32-bit integers.
- In the final decoder, replay only the tagged Hybrid dictionary extensions
  needed to reconstruct repair addresses. Do not rebuild the analyzer matrix.

### Rationale

These changes remove accidental arithmetic width and dead combinational state
without changing fault collection, Must decode, candidate validity, retained
PatternID policy, or D-004 storage ownership.

### Consequences

At this decision point the AREA path used seven analyzer cycles per subarray;
D-009 supersedes that latency with 40 Pattern-serial cycles. Final decode still
uses four cycles. Candidate validation remains solely owned by the canonical
analyzer; the final decoder trusts the committed ConfigID/PatternID and rebuilds
only the address dictionaries.

### Verification

- `./scripts/simulation/run_multi_config_phase2.sh` passes after exact-width
  conversion and again after removal of the final-decode matrix.
- Both parallel and AREA tops pass `single_group`, `single_early`, and
  `must_hybrid_group` C++-generated golden vectors.
- D-010 subsequently completed technology-mapped top and per-block reports
  after the DC license environment was verified.

---

## D-008 — Internal-block timing boundary for Phase 2 analyzer synthesis

- Date: 2026-09-09
- Status: ACCEPTED
- Scope: Phase 2 H13 timing constraints

### Context

The earlier 10 ns clock with 5 ns input and 5 ns output delays modeled an
external interface budget and left effectively no time for combinational
input-to-output paths after clock uncertainty. The analyzer is used only as an
internal synchronous block; its ports are not package pins.

### Decision

- Use a 20 ns clock with 0 ns input delay and 0 ns output delay for the first
  bounded-runtime mapped-area baseline.
- Keep 0.1 ns clock uncertainty, 0.5 ns input transition, output load, and the
  existing design-rule limits unchanged.
- Make clock, input delay, and output delay explicit runner parameters so a
  10 ns / 0 ns / 0 ns performance variant can be reproduced later.
- This decision supersedes only the clock and interface-delay values in D-005;
  its library, slow corner, wire-load model, tool, and reporting rules remain
  authoritative.

### Rationale

Input/output delay constrains the external timing environment at a block port;
it is not inherently package-pin delay. Zero-delay boundaries correctly model
the current adjacent-register upstream/downstream-register assumption and avoid
spending synthesis effort on an unintended half-cycle budget at both sides.

### Consequences

Reports from 20 ns / IO0 and 10 ns / IO0 are separate synthesis variants and
must not be compared as if they used identical timing constraints. Architecture
comparisons must use the same variant.

### Verification

- The runner records all three timing values in each result's `metadata.txt`.
- The constraint Tcl consumes the same environment values used for metadata.

---

## D-009 — Pattern-serial AREA analyzer

- Date: 2026-09-09
- Status: ACCEPTED
- Scope: Phase 2 H14 post-correctness optimization

### Context

Reusing one canonical analyzer across seven ConfigIDs removed analyzer-bank
duplication, but its combinational `config_analyzer` still evaluated up to ten
candidate patterns in parallel. Conservative-14 DC mapping could not complete
within 600 seconds even when run as an independent major block.

### Decision

- Keep the parallel analyzer as the compatibility/reference variant.
- In the AREA variant, reuse one `config_candidate_evaluator` over both
  ConfigID and PatternID dimensions.
- Evaluate candidates in the existing RECAM order and retain the first valid
  canonical PatternID. For transposed views, scan the complete canonical list
  and apply the established reverse physical PatternID mapping.
- Preserve shared Pivot, counter, tagged Hybrid, and CAM-reuse ownership.

### Consequences

The AREA analyzer requires 40 candidate cycles per subarray (6+3+10+4+3+10+4)
instead of seven configuration cycles. This changes latency only; it does not
change ConfigPatternMap or repair output semantics.

### Verification

- The AREA-engine test checks the exact 40-cycle schedule and all seven results.
- Parallel and AREA tops pass the same C++-generated golden cases.
- Conservative-14 AREA-engine elaboration falls to 15,765 leaf cells from the
  prior 27,837, and structural standard-cell mapping completes in 57 seconds.

---

## D-010 — Structural mapping followed by post-map STA for H13

- Date: 2026-09-09
- Status: ACCEPTED
- Scope: Phase 2 H13 synthesis/report integration

### Context

Timing-driven and exact-map runs with constraints applied before compile spent
more than 600 seconds in delay/design-rule optimization. Positive intermediate
figures from those interrupted runs were not terminal reports and remain
diagnostic only.

### Decision

- Add `COMPILE_MODE=structural_report`: perform standard-cell structural mapping
  before applying the analyzer block timing constraints, then run post-map STA
  and generate the full report/netlist set.
- Use this mode for H13 mapping/report integration and per-major-block area
  breakdown. Do not describe it as timing-driven optimization or closure.
- Retain 20 ns clock, 0 ns input/output delay, slow corner, and all report-time
  design-rule constraints from D-008.

### Consequences

Mapped area is suitable for structural feasibility and like-for-like variant
comparison. Timing and design-rule violations are visible but not repaired by
this flow. A later timing-closure phase requires pipelining or a separately
authorized optimization effort.

### Verification

- The complete conservative-14 AREA top and ten conservative/experimental
  per-block runs end with `STATUS=PASS`, complete reports, and mapped netlists.
- The complete top reports 49,023 leaf cells, 10,418 sequential cells, and
  1,703,003.72 cell area.
- Post-map STA explicitly reports WNS -169.97 ns and 2,291 violating nets.

---

## D-011 — Config-serial, Pattern-parallel shared analyzer architecture

- Date: 2026-09-09
- Status: ACCEPTED
- Scope: Phase 3 analyzer-area research direction

### Context

Phase 2's Directional Multi-Config implementation, including its 40-cycle
Pattern-serial AREA variant and synthesis artifacts, is complete historical
evidence.  Its timing outcome does not establish the correct architecture for
the new analyzer-area baseline.

### Decision

- Preserve the Phase 2 implementation and results without deletion, rewrite,
  refactoring, or further timing optimization.
- Use a fully combinational RECAM 2R2C baseline: CAM logical state → matrix →
  six parallel candidates → PatternID.
- Use one combinational Shared Config Analyzer for DSS configuration analysis.
- Evaluate ConfigIDs sequentially, one per cycle, and retain results in
  ConfigPatternMap registers.
- Keep candidates within one ConfigID parallel; do not use Pattern-serial
  evaluation for the new primary architecture.
- Build EARLY only after ConfigPatternMap completion, then build GROUP and
  PhysicalResourceLedger as separately measurable later stages.
- A timing failure requires reporting area, critical path, slack, and estimated
  Fmax; it does not authorize further serialization or architectural change.

### Rationale

This decomposition isolates RECAM analysis cost from multi-configuration
reuse, selector logic, and physical-resource control while preserving an
interpretable hardware boundary for each measurement.

### Consequences

- The primary DSS analyzer is config-serial plus pattern-parallel, not
  config-parallel plus pattern-parallel and not config-serial plus
  pattern-serial.
- Phase 3 work begins at the RECAM 2R2C analyzer baseline; a complete DSS
  top-level is not currently authorized.
- Required synthesis reporting is `A_RECAM_MATRIX`, `A_RECAM_PATTERN`,
  `A_RECAM_ENCODER`, `A_RECAM_COMPLETE`, `A_SHARED_CONFIG_ANALYZER`,
  `A_CONFIG_SCHEDULER`, `A_CONFIG_PATTERN_MAP`, `A_EARLY`,
  `A_GROUP_CONTROLLER`, and `A_RESOURCE_LEDGER`.

### Verification

- Phase 3A exit gate confirms a fully combinational, six-candidate-parallel
  2R2C analyzer with separately reported matrix, pattern, encoder, and
  complete boundaries.
- Later Phase 3 gates verify seven ConfigID cycles, complete result-map use by
  EARLY, and GROUP operation only after four completed result maps.

---

## Decision template

```text
## D-NNN — Title

- Date: YYYY-MM-DD
- Status: PROPOSED | ACCEPTED | SUPERSEDED
- Scope:

### Context
### Decision
### Rationale
### Consequences
### Verification
```
