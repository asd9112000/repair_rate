# WoW 3D DRAM Repair Analyzer RTL Project

> Current project-level authority for RTL scope.  The detailed Phase 2
> implementation record for the 2×2 Directional Multi-Config Analyzer is
> [03_ANALYZER_RTL_HANDOFF.md](03_ANALYZER_RTL_HANDOFF.md).  Phase 3 execution
> authority is instead governed by [02_PHASE_CONTROL.md](02_PHASE_CONTROL.md).

## 1. Project Goal

本專案目標是透過 synthesizable SystemVerilog 與統一 ASIC synthesis methodology，建立 memory repair analyzer 的硬體成本模型。

## Frozen Phase 3 address boundary

For the active Phase 3 RECAM/DSS analyzer work, the authoritative RTL address
boundary is [PHASE3B_ANALYZER_INTERFACE.md](PHASE3B_ANALYZER_INTERFACE.md):
`Domain[2:0] + Bank[1:0] + Group[5:0] + SA[1:0] + Row[8:0] +
ColumnWord[4:0] = 27` bits. RECAM Channel is the 13-bit
`Domain+Bank+Group+SA` prefix. This analyzer address interface is distinct from
the C++ simulator's physical-cell geometry and does not authorize synthesis or
Phase 3C.

主要比較三種 architecture：

1. 2×2 Dynamic Spare Sharing with CAM
2. RECAM baseline
3. 2×2 Dynamic Spare Sharing with SRAM + FIFO

主要評估：

- total cell area
- combinational area
- sequential area
- cell count
- critical path
- achievable clock frequency
- fault collection cycles
- analysis cycles
- total repair latency
- storage bits
- comparator count
- FIFO overhead

最終 synthesis 結果將輸出為 CSV，並回灌 system-level simulator，以分析 repair rate、area 與 latency 的 Pareto tradeoff。

---

# 2. Architecture Priority

## Primary Architecture — Phase 3 Analyzer-Area Decomposition

The Phase 2 Directional Multi-Config implementation is frozen historical
evidence.  It remains reproducible, but it is not the primary architecture for
new analyzer-area comparisons and must not be deleted, refactored, or subjected
to further timing optimization.

The current research order is:

1. RECAM 2R2C analyzer area: CAM logical state → matrix → `PatternID`.
2. One shared DSS multi-config analyzer, with configuration-serial and
   pattern-parallel operation.
3. EARLY selector overhead.
4. GROUP_COMPRESSED local policy, then separately `PhysicalResourceLedger`
   overhead.

Phase 3D remains at its authorized EARLY-only boundary: it is a combinational
selector after the complete four-config Phase 3C map, not early-stop scheduling.
Its accepted 20 ns integrated synthesis closes at 73,533.399055 area and
7,368.67 GE, a +1.7210% area increment over Phase 3C. It does not authorize
PhysicalResourceLedger mutation or group allocation work.

Phase 3E is functionally complete for GROUP_COMPRESSED only. It ranks completed
local map entries by `2*Borrow + (1-Release)` after all four Phase 3C scans;
it does not allocate or commit a shared spare. Its accepted 20 ns integrated
synthesis closes at 73,693.066257 area and 7,384.67 GE, +1.9418% over Phase 3C.
PhysicalResourceLedger mutation and group allocation remain unstarted and
require separate authorization.

Phase 3F/3G now defines only read-only ledger semantics and a single-SA
policy-aware fallback over the completed map. Neither phase commits a release
or borrow, arbitrates subarrays, or fixes group order; those remain Phase 3H
research questions.

The immediate target is a clean analyzer-area baseline, not a completed DSS
top-level implementation.  Phase 3 authority and exit gates are defined in
[02_PHASE_CONTROL.md](02_PHASE_CONTROL.md).

## Historical Primary Architecture — DATE Phase 2

DATE paper 的主要 Dynamic Spare Sharing architecture 為：

2×2 neighboring subarray group

A B  
C D

2×2 DSS 為目前最高優先研究架構。

所有 DATE 所需：

- RTL
- verification
- synthesis
- area comparison
- timing comparison
- analyzer latency comparison

均優先完成 2×2 architecture。

不得將 2×2 標示為：

- legacy
- deprecated
- regression-only

### 2.1 Frozen Phase 2 2×2 analyzer architecture

The current 2×2 directional RTL study uses the **Directional Multi-Config
Analyzer**.  It evaluates seven physical R/C resource envelopes:

```text
2R2C, 2R1C, 3R2C, 3R1C, 1R2C, 2R3C, 1R3C
```

`ConfigID` identifies that physical resource envelope.  It is distinct from
`PatternID`, which identifies the RECAM row/column ordering within one
configuration.  The target physical architecture shares pivot/address payload
and physical fault counters; configuration-specific state is logical metadata,
not a complete CAM/counter replica.  The retained 2×2 group state is therefore
conceptually ordered `PivotPayload`, `ConfigPatternMap`, and
`CAMReusePending`, with selected repairs staged before eFuse programming.

This supersedes the earlier matrix-address plus valid-bitmap retention model
for this **new 2×2 analyzer only**.  It does not alter legacy RECAM,
hierarchical, or 1×4 experiment representations.

---

# 3. Secondary Architecture

1×4 A–B–C–D architecture 為後續 architecture extension。

目前 1×4 包含：

- row-only spare sharing
- Layout / SharingTopology / SolutionTakePolicy separation
- PhysicalResourceLedger
- GROUP_COMPRESSED solution representation

1×4 不得取代 DATE 2×2 architecture。

The existing 1×4 row-only experiments remain a separate, valid research
layout.  The 2×2 multi-config RTL architecture must not be interpreted as a
replacement for their topology, policy, or retained-state semantics.

在 2×2 RTL hardware evaluation 完成之前，不投入完整 1×4 RTL implementation。

## 3.1 Phase 3 measurement decomposition

Phase 3 reports must retain separate synthesis visibility for:

```text
A_RECAM_MATRIX
A_RECAM_PATTERN
A_RECAM_ENCODER
A_RECAM_COMPLETE

A_SHARED_CONFIG_ANALYZER
A_CONFIG_SCHEDULER
A_CONFIG_PATTERN_MAP

A_EARLY

A_GROUP_CONTROLLER
A_RESOURCE_LEDGER
```

Do not replace this decomposition with one opaque total-area report.  Area,
critical path, slack, and estimated Fmax are separate observations; timing
failure does not authorize serializing RECAM candidates or changing the
approved architecture.

---

# 4. Primary Hardware Comparisons

本專案主要建立：

### Architecture A

RECAM baseline

用途：

提供 literature-based competitive repair analyzer baseline。

### Architecture B

2×2 DSS + CAM

用途：

量化 Dynamic Spare Sharing 相對 baseline 所增加的 analyzer hardware cost。

### Architecture C

2×2 DSS + SRAM/FIFO

用途：

研究將 associative fault storage 改為 sequential/chunked SRAM-based search 後的 area–latency tradeoff。

---

# 5. Hardware Research Questions

本 RTL experiment 必須回答：

RQ1.

2×2 DSS 相較 RECAM 增加多少 analyzer area？

RQ2.

DSS 額外面積主要來自：

- fault storage
- comparator
- candidate generation
- sharing/resource-management logic
- solution storage

中的哪些部分？

RQ3.

在相同 DSS repair policy 下：

CAM 與 SRAM/FIFO implementation 的 area 與 latency tradeoff 為何？

RQ4.

SRAM search comparator parallelism P 如何影響：

- area
- critical path
- analysis cycles

RQ5.

hardware cost 隨：

- fault entries
- spare count
- address width

如何 scaling？

---

# 6. Methodology Principle

三套 architecture 必須在相同：

- technology library
- synthesis tool
- synthesis options
- clock constraint
- address configuration
- fault capacity
- spare configuration
- top-level interface convention

下進行比較。

不得透過不同 RTL coding style 人為偏向其中一個 architecture。

algorithm 與 storage implementation 應盡可能分離。

---

# 7. RECAM Reproduction Rule

RECAM implementation 必須以提供的 paper 為依據。

paper 沒有明確說明的 RTL detail：

不得當作 paper fact。

所有補充 implementation decision 必須記錄於：

09_ASSUMPTIONS.md

若無法完全重建 paper microarchitecture，論文與文件應使用：

RECAM-inspired RTL implementation

或：

RTL reproduction based on RECAM architecture

而非宣稱 bit-exact reproduction。

---

# 8. SRAM Terminology Rule

若 SRAM storage 使用 synthesizable register array 實現，而未 mapping 至實際 SRAM macro：

必須稱為：

SRAM-like storage

或：

SRAM-oriented architecture

不得直接宣稱其 synthesis area 等於實際 SRAM macro area。

同理，若 CAM 使用 standard-cell register + comparator 實作，其結果代表：

standard-cell CAM implementation。

若未來導入 SRAM/OpenRAM/memory compiler macro，macro area 必須與 standard-cell logic area 分開報告。

---

# 9. Verification Principle

hardware optimization 不得改變 repair algorithm。

對相同 fault trace：

2×2 DSS-CAM

與

2×2 DSS-SRAM/FIFO

在相同 repair policy 下必須產生相同 functional repair result。

允許不同：

- analysis latency
- internal processing order

不允許因 storage implementation 改變：

- repairable result
- selected solution
- spare ownership constraint

除非該差異本身是明確定義的新 architecture policy。

---

# 10. Final Output

最終至少輸出：

results/synthesis_results.csv

results/latency_results.csv

results/module_area_breakdown.csv

並包含：

architecture
configuration
ENTRY_NUM
ADDR_WIDTH
RS
CS
P
FIFO_DEPTH
area
cell_count
critical_path
frequency
analysis_cycles

供 system-level simulator 與 plotting 使用。

---

# 11. Scope Control

目前不得因 RTL implementation 主動修改：

- DSS algorithm
- spare sharing rule
- repair policy
- fault model

若發現 algorithm specification 不足，應記錄問題並停止該部分 implementation。

hardware project 的主要任務是：

量化 implementation cost，而不是重新設計 repair algorithm。

---

# 12. Repository Boundary

RTL hardware work 與既有 C++ system-level simulation 使用不同的 source、test、
script 與 output boundaries：

```text
C++ simulation
├── *.cpp, inc/, src/, tests/, fault_generator/
├── scripts/legacy/, scripts/group/, scripts/device/
└── reports/

RTL hardware work
├── rtl/
├── tb/
├── scripts/lint/, scripts/simulation/, scripts/synthesis/
├── scripts/sweep.py, scripts/parse_reports.py
└── results/
```

`scripts/simulation/` 專指 HDL/RTL simulation automation；既有 C++ simulator 的
runner 不得搬入該目錄。`results/` 專收 RTL synthesis 與 RTL cycle-accurate
verification 結果；C++ architecture／repair-rate simulation 仍輸出至 `reports/`。

兩條流程只透過明確版本化的 fault trace、golden output 或 CSV schema 交換資料。
RTL 不得直接修改 C++ repair semantics，C++ analytical cycle／area proxy 也不得寫入
RTL `results/` 冒充 synthesis 或 cycle-accurate RTL measurement。
