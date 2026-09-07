# RTL Decision Log

本文件只記錄會影響 RTL architecture、verification、synthesis methodology、目錄邊界
或結果解讀的決策。工作進度由 [PHASE_STATUS.md](PHASE_STATUS.md) 管理；C++ simulator
的 architecture semantics 仍以 `docs/` 中的 Current 文件為準。

新增決策時應保留既有紀錄，並包含日期、狀態、背景、決策、理由、影響與驗證方式。

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
