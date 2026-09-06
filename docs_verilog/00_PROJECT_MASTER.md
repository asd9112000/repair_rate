# WoW 3D DRAM Repair Analyzer RTL Project

## 1. Project Goal

本專案目標是透過 synthesizable SystemVerilog 與統一 ASIC synthesis methodology，建立 memory repair analyzer 的硬體成本模型。

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

## Primary Architecture — DATE

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

---

# 3. Secondary Architecture

1×4 A–B–C–D architecture 為後續 architecture extension。

目前 1×4 包含：

- row-only spare sharing
- Layout / SharingTopology / SolutionTakePolicy separation
- PhysicalResourceLedger
- GROUP_COMPRESSED solution representation

1×4 不得取代 DATE 2×2 architecture。

在 2×2 RTL hardware evaluation 完成之前，不投入完整 1×4 RTL implementation。

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