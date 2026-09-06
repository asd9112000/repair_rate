# Repository 架構

> 文件狀態：Current
> 適用範圍：cross-cutting
> 建立時間：Unknown
> 最後修改時間：2026-09-07T00:00:00+08:00
> 本文件權威主題：架構分層、resource ownership 與 Legacy/Hierarchical 邊界

## 1. 架構邊界

repository 目前不是只有一個 simulator，而是三條可並存、用途不同的路徑：

```text
Legacy paper regression
    basicPEarray / RedundantRate / SharedLine / SharedLine_SRAM

4-SA group experiments
    DynamicSpareSharing / DynamicSpareSharing_SRAM_RECAM

Device-level hierarchical experiments
    HierarchicalRECAM
```

`RECAM_PE`、CAM、solution generator 等 solver 元件可以共用；但是否跨 group
保留 CAM occupancy，必須由上層架構決定。Legacy 或 group-level app 的一個
software container，不等於一份實體 CAM hardware。

## 2. 分層

```text
Apps / CLI
│
├── Legacy experiment loops
├── Dynamic group runner
└── DeviceRepairScheduler
        │
        ├── BIRA engine scheduling / scratch lifetime
        └── GlobalOnlineRepairPool / persistent Tier-2 assignments
                 │
                 ▼
        DynamicRepairSimulator          4-SA group analyzer
                 │
        PhysicalResourceLedger          Tier-0/Tier-1 physical spares
                 │
        RepairAttemptSolver interface
          ├── RECAMSolverAdapter
          └── SramRecamSolverAdapter
                 │
        RECAM_PE / CAM / SolGenerator
```

`RecamGeometry` 是 CAM 與 SRAM backend 共用的 entry-count／field-width 來源；
`HardwareMetrics` 與 `BiraLatencyMetrics` 則是兩個 backend 共用的輸出 contract。
因此 CSV 中的 CAM/SRAM bit 數與 BIRA work／event-timeline 欄位可以直接並列，
但不應將其視為 foundry macro area 或 RTL sign-off timing。

## 3. 主要模組

| 模組 | 責任 |
|---|---|
| `Fault`／`FaultList` | fault address、classification state、fault-file loading |
| `RECAM_addressCAM` | pivot address collection 與 must-rule state |
| `RECAM_hybridCAM` | compressed nonpivot information |
| `RECAM_bufferCAM` | additional-pivot CAM-reuse functional representation |
| `SolGenerator` | `choose(R+C,R)` repair candidates 與 matrices |
| `RECAM_PE` | 一次 RECAM collection、matrix conversion、candidate validation |
| `RECAMSolverAdapter` | 將 legacy pointer-rich solver 結果轉為 pointer-free DTO |
| `SramRecamSolverAdapter` | 保持 repair semantics，改用 SRAM search/latency model |
| `DynamicRepairSimulator` | 一個 4-SA group 的 Tier-0/Tier-1 candidate analysis |
| `PhysicalResourceLedger` | spare ownership、借用、reserve 與 physical conservation |
| `TileSolutionState` | pointer-free matrix address dictionary、valid-solution bitmap 與 persistent BUFFMAP snapshot |
| `DeviceRepairScheduler` | 跨 groups 的 BIRA scheduling、Tier-2 allocation 與 hierarchy success |
| `GlobalOnlineRepairPool` | device-wide finite CAM capacity、full tags、dedup、occupancy |
| `DynamicCsvReporter` | group-level attempts/runs/summary CSV |
| `HierarchicalCsvReporter` | v2 device/groups/BIRA-engine CSV |
| `CanonicalExperimentReporter` | B0/B1/B2/B3 long-format CSV 與每組獨立 RemapTable |

## 4. Fault 與位址

程式內 `Fault` 保留：

```text
HBMID
ChannelID
BankID
SubarrayGroupID
SubarrayID
row
cell column
```

這是 **internal physical-cell address**。既有 `fault_generator/faults.faults` 與
給下游使用的 `RemapTable.txt` 都維持這個單位；不可把第七欄 `Col` 改解釋為
word column，否則會破壞 line-remap coverage 的語意。

外部 BIST port 則使用明確不同的型別／介面：

```text
BIST_PORT_WORD_MASK_V1
    HBMID ChannelID BankID SubarrayGroupID SubarrayID Row WordCol
    + wordBits-wide failMask (syndrome sideband)

FAULT_FILE_PHYSICAL_V1 / REMAP_LOG_PHYSICAL_V2
    HBMID ChannelID BankID SubarrayGroupID SubarrayID Row CellCol
```

每個 fail-mask set bit 會解碼為一筆 internal `Fault`：

```text
cell_col = word_col × word_bits + bit_offset
word_col = cell_col / word_bits
bit_offset = cell_col % word_bits
```

因此同一個 word 的多個 faulty bits 有相同 BIST arrival cycle，但在目前
physical-fault FIFO baseline 中仍是不同的 queue entries；online CAM/SRAM reuse
才依 word address 去重。

在 WoW-v1.0 hierarchical model 中：

```text
HBMID              = device
ChannelID          = GBUS-based channel-like domain
BankID             = bank within domain
SubarrayGroupID    = 16-Mbit section / 4-SA repair group
SubarrayID         = A/B/C/D
```

Online data-word repair tag 不含 device ID，因為一個
`DeviceRepairScheduler`／pool 只處理一個 device：

```text
(domain, bank, group, subarray, row, repair_column)
```

`repair_column` 在 `DATA_WORD` 模式為 `cell_col / data_word_bits`，在 `CELL`
模式為原始 cell column。

## 5. Resource ownership

| Resource | Ownership | Lifetime |
|---|---|---|
| Local spare rows/columns | 每個 SA | physical、persistent |
| Tier-1 borrowable spare lines | 同一 4-SA group | physical、由 ledger 分配 |
| Address/Hybrid CAM + matrix scratch | 每個實體 BIRA engine | offline temporary |
| Final line-repair solution | 每個 repair domain | eFuse/NVM conceptual state |
| Online CAM-reuse assignments | 一個 memory device 共用 | runtime persistent |

硬體面積不得用：

```text
CAM_per_SA × total_subarrays
```

Hierarchical accounting 使用：

```text
local spare cost × total_subarrays
+ sharing logic × total_repair_groups
+ BIRA scratch cost × bira_engine_count
+ persistent solution storage
+ one finite global online CAM structure
```

若 offline 與 online 使用同一 mode-reused CAM structure，不再額外加上一份
online CAM area。

## 5.1 Report scope

`reports/` 的 scope 是實驗結果的硬體 ownership 邊界，而非單純的資料夾名稱：

- `group`：獨立 4-SA analysis；不累積跨 group 的 online CAM occupancy。
- `device`：hierarchical device analysis；所有 modeled groups 競爭同一個 finite
  device-wide online CAM pool。
- `studies`：編排多個 scope 的研究容器，不是新的 simulator scope；其 group、SRAM、
  device 子輸出仍不可混合比較。

完整輸出目錄、legacy scope 與 artifact 保留規則以
[REPORTS.md](REPORTS.md) 為準。

## 6. 三個 repair tiers

```text
Tier 0: one SA
    local spare rows / columns

Tier 1: one 4-SA group
    constrained borrowed physical spare lines

Tier 2: whole device
    residual repair words request global CAM entries
```

Tier 1 仍是 line remapping，不是 CAM reuse。Tier 2 不消耗 spare row/column。

## 7. Offline／online lifetime

```text
Manufacturing / offline
group N faults
    -> selected BIRA engine scratch
    -> Tier-0 / Tier-1 analysis
    -> derive Tier-2 word requests if needed
    -> commit persistent solution
    -> clear scratch
    -> next group

Power-up / runtime
persistent CAM assignments
    -> load global Address/Hybrid CAM representation
    -> tag hit returns replacement word
    -> miss uses normal memory path
```

`scratchClearCount` 只表示 transient analysis state 被清除；不會釋放已提交的
global online CAM entry。

## 8. Legacy architecture

`SharedLine.cpp` 使用固定 2×2 Ring 與 candidate enumeration。
`DynamicSpareSharing.cpp` 將它一般化為 configurable 4-SA group policies，並用
`PhysicalResourceLedger` 確保：

- 一條 physical spare 最多分配一次；
- borrowing 不增加 physical total；
- Directional／Global／Edge topology 遵守各自連線限制；
- minimum reserve 與 single-dimension modifiers 可觀察。

Solution Take Policy 是獨立於 layout/topology 的第三個維度。預設 `legacy` 保留既有
group selector；`early` 依 A→D commit、失敗時不回溯；`group` 在不改動 ledger
的 speculative allocation 上枚舉四個 SA 的 RECAM-valid solution product，找到
最佳組合後才做一次 logical commit。兩個新 policy 都重用
`PhysicalResourceLedger`，不建立另一套 spare ownership 或 repairability oracle。

`DynamicSpareSharing` 的 group geometry 與 sharing topology 是兩個獨立設定：

```text
GRID_2X2                   LINE_1X4
A B                       A — B — C — D
C D
```

`GRID_2X2` 保留既有 `none`／`directional`／`edge`／`global` 行為。
`LINE_1X4` 第一階段只允許 row sharing，並提供 `none`、`pair`
（A↔B、C↔D）、`neighbor`（A↔B↔C↔D 的直接相鄰邊）與 `global`。
Neighbor spare 不可跨越中間 SA 進行 multi-hop borrowing。兩種 layout 都沿用同一個
`PhysicalResourceLedger` 與 RECAM backend；layout 不改變 fault address、SubarrayID
或 RECAM classification／solution semantics。

這些 app 的多個 `runs` 是 Monte Carlo group samples，不代表一個 device 中會
共同保留 online CAM occupancy。需要研究跨 bank/domain/global capacity 時，
必須使用 `HierarchicalRECAM`。

## 9. 輸出責任

| Reporter | 輸出 | 架構 scope |
|---|---|---|
| `DynamicCsvReporter` | `attempts.csv`、`runs.csv`、`summary.csv` | 4-SA group／legacy experiments |
| `DynamicRemapReporter` | `RemapTable.txt`、simplified table | selected group remaps |
| `HierarchicalCsvReporter` | `device_summary.csv`、`groups.csv`、`bira_engines.csv` | WoW-v1.0 device |
| SRAM-RECAM app/reporters | policy CSV、area JSON、runtime metrics | group-level CAM-vs-SRAM comparison |

所有 hierarchical CSV 必須有 `WoW-v1.0`、`Hierarchical-RECAM-v2.1` 與
`GLOBAL_LOGIC_DIE` metadata。完整欄位見
[HIERARCHICAL_RECAM.md](HIERARCHICAL_RECAM.md)。

## 10. 目前限制

- Hierarchical CLI 一次只處理一個 memory device；包含多個 HBMID 的輸入會被拒絕。
- `--groups N` 可模擬 reference device 的前 N 個 groups；`device_success` 只針對
  本次提供的 modeled groups，不自動補齊未提供的 groups。
- Reference geometry 的 4-SA section decomposition 是研究模型，不是 Micron
  patent 明示的實體 subarray 組織。
- Full RTL timing、NoC traffic、bank scheduling 與複雜 global allocation optimization
  尚未建模。
- `BiraLatencyMetrics` 的 BIST event timeline 是 **group-relative** A→B→C→D
  schedule；多個 groups 共用同一 BIRA engine 的 device-wide wall-clock backlog
  尚未加入這條 timeline。`bira_engines.csv` 的 `total_cycles` 是累積 work proxy。
- Legacy `SharedLine.cpp` 有 source fingerprint golden；修改其演算法前必須明確
  recapture regression data。
