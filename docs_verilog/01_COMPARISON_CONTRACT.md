# Hardware Comparison Contract

## 1. Objective

本文件定義 RECAM、2×2 DSS-CAM 與 2×2 DSS-SRAM/FIFO 的公平比較條件。

除非 experiment 明確研究某個 parameter，其他條件應保持一致。

---

# 2. Shared Parameters

所有 architecture 優先共用以下 parameter naming：

ADDR_WIDTH

ROW_ADDR_WIDTH

COL_ADDR_WIDTH

FAULT_ENTRY_NUM

FAULT_NUM_MAX

RS

CS

GROUP_SIZE

CLK_PERIOD

---

# 3. DSS-SRAM Specific Parameters

DSS SRAM implementation 額外具有：

P

FIFO_DEPTH

其中：

P = 每 cycle 可同時比較的 entry 數量。

---

# 4. Primary DSS Configuration

DATE primary architecture：

GROUP_SIZE = 4

physical arrangement：

A B  
C D

此 configuration 代表 2×2 neighboring sharing group。

RS、CS 與 sharing rule 應由 DSS architecture specification 定義，不得由 RTL designer 自行修改。

---

# 5. Common Transaction Interface

三個 analyzer top-level 應盡可能採用一致的 transaction concept：

Input：

- clk
- reset
- fault_valid
- fault_address
- fault metadata
- analysis_start

Output：

- fault_ready
- analysis_busy
- analysis_done
- repairable
- repair solution
- analysis cycle count

若 architecture 本質上需要額外 signal，可以擴充，但必須記錄原因。

---

# 6. Area Definition

至少記錄：

total cell area

combinational area

sequential area

cell count

mapped leaf-cell count

NAND2X1-equivalent gate count (GE), including the NAND2X1 area basis

此外盡可能提供 module-level breakdown：

fault storage

comparators

candidate analyzer

sharing logic

solution storage

FIFO

controller

---

# 7. Timing Definition

統一報告：

worst critical path

target clock period

timing slack

estimated maximum operating frequency

不得只報 synthesis tool 是否 pass。

---

# 8. Latency Definition

至少分開：

fault collection cycles

analysis cycles

solution generation cycles

必要時：

total cycles

不得只使用 analytical latency equation。

cycle count 必須由 RTL simulation 驗證。

analytical model可用於 cross-check。

---

# 9. CAM Implementation

CAM 必須明確包含：

stored entries

valid bits

equality comparison logic

match vector

priority/select logic

不得以不可綜合 behavioral CAM primitive 隱藏 comparator cost。

---

# 10. SRAM-like Implementation

SRAM-like architecture 必須將：

storage

read addressing

P-way comparator array

search controller

FIFO

分離。

P 必須 parameterized。

不得將 SRAM-like implementation 寫成對所有 entries 同時 combinational compare，否則其本質再次成為 CAM-style search。

---

# 11. Search Parallelism

對 N 個 valid entries：

P = 1

代表每 cycle 最多處理一個 search entry。

P > 1

代表每 cycle 最多處理 P 個 entries。

analytical worst-case search latency 可用：

ceil(N / P)

作為 sanity check。

真正 latency 使用 RTL simulation cycle count。

---

# 12. Storage Fairness

比較 CAM 與 SRAM/FIFO 時，必須確認兩者能保存等價的 fault information。

至少記錄：

number of entries

bits per entry

metadata bits

valid bits

total logical storage bits

不得只比較 module area 而不報 logical storage capacity。

---

# 13. Synthesis Fairness

所有 standard-cell RTL 使用：

相同 technology library

相同 PVT corner

相同 synthesis tool

相同 synthesis effort

相同 timing constraint

相同 optimization policy

每次 experiment 保存 synthesis script 與 log。

---

# 14. Macro Boundary

如果使用 SRAM macro：

SRAM macro area 不得與 synthesized register-array area 混為同一種 measurement。

報告：

standard-cell logic area

memory macro area

total estimated area

三項數值。

---

# 15. Initial Sweep

第一階段建議：

FAULT_ENTRY_NUM：

4
8
16
24
32

SRAM comparator parallelism P：

1
2
4
8
16

超過 ENTRY_NUM 的 P configuration 不需執行。

RS / CS sweep 應依 DATE architecture 實際設定決定。

不要在 architecture 尚未 functional-verified 前進行大規模 sweep。

---

# 16. Simulation / RTL Artifact Boundary

比較資料必須保留來源類型：

- C++ system-level simulation source、runner 與輸出分別位於既有 simulator path、
  `scripts/{legacy,group,device}/` 與 `reports/`。
- RTL source、testbench、automation 與輸出分別位於 `rtl/`、`tb/`、
  `scripts/{lint,simulation,synthesis}/` 與 `results/`。
- C++ simulator 可作為 RTL functional golden model，但必須以固定 fault trace 與明確
  schema 交換資料，不得讓兩者共用未版本化的內部狀態。
- `reports/` 中的 analytical area／latency proxy 不得填入 `results/` 的 synthesis 或
  RTL cycle 欄位；回灌 simulator 的 RTL 數值必須能追溯到 configuration、tool、
  library、constraint、RTL commit 與原始 log。

---

# 17. Directional Multi-Config Analyzer as a comparison consumer

The 2×2 Directional Multi-Config Analyzer specified in
[03_ANALYZER_RTL_HANDOFF.md](03_ANALYZER_RTL_HANDOFF.md) is a concrete consumer
of this comparison methodology.  Its shared Pivot CAM, tagged Hybrid storage,
counter table, selector, ledger, and pending-repair buffer must be synthesized
as complete functionally equivalent blocks under the same standard-cell
library, constraints, and reporting rules as other compared designs.

Do not presume SRAM-like storage is smaller than a CAM implementation.  Report
area and critical path for the complete selected configuration, and retain the
logical entry/metadata-bit accounting required by Sections 9--14.

This contract does not define the analyzer's ConfigID, PatternID, transpose,
or policy semantics; those are defined by the analyzer handoff and simulator
golden-model contract.
