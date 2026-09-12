# HierarchicalRECAM 使用與架構指南

> 文件狀態：Current
> 適用範圍：hierarchical-device
> 建立時間：Unknown
> 最後修改時間：2026-09-08T00:00:00+08:00
> 本文件權威主題：WoW-v1.0 hierarchy、HierarchicalRECAM CLI、輸入、輸出與 device-level 指標

## 1. 這個程式解決什麼問題？

`HierarchicalRECAM.cpp` 是 device-level simulator 入口。它修正舊模擬中最重要的
hardware ownership 錯誤：RECAM 的 Address CAM、Hybrid CAM 與 matrix analyzer
屬於 logic-die BISR/BIRA，不是每個 tile 或 subarray 各有一套。

預設模型：

```text
Memory / WoW device
├── 8 channel-like domains
│   └── 4 banks / domain
│       └── 64 repair groups / bank
│           └── 4 subarrays / group
│
└── Logic die
    ├── shared BIRA engine(s), offline scratch
    └── one finite global online CAM-reuse pool
```

因此：

- local spare rows/columns 仍是每個 SA 的 physical resources；
- Tier-1 sharing 只發生在同一 4-SA group；
- BIRA scratch 由 `bira_engine_count` 決定，預設一套；
- 所有 modeled groups 的 Tier-2 requests 累積到同一個 online CAM pool；
- pool 不會在每個 SA 或 group 開始時重置。

## 2. Reference architecture

本節是目前 WoW-v1.0 hierarchical 硬體參數的權威記錄；索引與參數摘要見
[DOCUMENT_INDEX.md](DOCUMENT_INDEX.md)。

WoW-v1.0 是一個 frozen research baseline，不是商業產品規格。

| Parameter | Baseline | Evidence status |
|---|---:|---|
| Total capacity | 32 Gbit / 4 GiB | Micron-disclosed example |
| Channel-like GBUS domains | 8 | Micron-disclosed example |
| Banks per domain | 4 | Micron-disclosed example |
| Total banks | 32 | Micron-disclosed example |
| Groups/sections per bank | 64 | Micron-disclosed example |
| Group capacity | 16 Mbit | Micron-disclosed section capacity |
| Subarrays per group | 4 | research modeling assumption |
| Total repair groups | 2048 | derived |
| Total subarrays | 8192 | derived |
| Rows per modeled SA | 512 | SALP-based model |
| Cell columns per modeled SA row | 8192 | research model |
| Data word | 256 bits | WoW-inspired baseline |
| Local spare rows / columns | `Rs=Cs=2` | RECAM baseline |

Capacity check：

```text
512 rows × 8192 cells = 4 Mbit / SA
4 SA × 4 Mbit        = 16 Mbit / group
64 groups × 16 Mbit  = 1 Gbit / bank
32 banks × 1 Gbit    = 32 Gbit / device
```

不可宣稱：

- 所有 WoW memory 都一定有 8 channels；
- Micron 明示每個 16-Mbit section 有四個 subarrays；
- 所有 subarray 都一定是 `512×8192`；
- `0.415%` local spare-cell ratio 是完整 BISR area。

### 2.1 Global CAM/SRAM spare-word address budget

Tier-2 是一個 device-wide 的 persistent data-word replacement directory；它不是
每個 SA 各自一份 CAM。故 online lookup 必須包含能唯一識別 source word 的完整
tag，而不能只保存 row／column 或 bank。對一個 scheduler/pool 所代表的單一 device，
`HBMID` 是隱含 scope，不寫入 tag。

在本 baseline、`data_word_bits=256` 下：

| Online Address CAM field | Range | Bits |
|---|---:|---:|
| Enable | valid / invalid | 1 |
| Channel-like domain | 8 | 3 |
| Bank within domain | 4 | 2 |
| Repair group within bank | 64 | 6 |
| Subarray within group | 4 | 2 |
| Row | 512 | 9 |
| Word column | `8192 / 256 = 32` | 5 |
| **Total entry width** |  | **28** |

因此 exact online tag 為 27 bits，含 Enable 的 Address CAM entry 為 28 bits。
`cell column` 是 13 bits，但在 data-word replacement mode 必須先轉成
`word_column = floor(cell_column / 256)`；同一 word 的多個 faulty cells 必須
deduplicate 為同一 CAM entry。

`Rs=Cs=2` 時，`K=Rs+Cs=4`，所以 Address CAM、Hybrid CAM 與 paper-derived
online reuse capacity 都是 4 entries，Hybrid pointer 是 `log2(K)=2` bits：

| Structure / mode | Entry width | Entries | Logical storage |
|---|---:|---:|---:|
| Address CAM, offline collection | `1 + 9 + 13 + 1 + 1 + 2 + 2 = 29` | 4 | 116 bits |
| Address CAM, online global lookup | `1 + 27 = 28` | 4 | 112 bits |
| Hybrid CAM, online replacement | `Enable(1) + pointer(2) + data(256) = 259` | 4 | 1036 bits |

同一份 CAM/SRAM 若在 offline 與 online mode reuse，Address entry 至少取兩種 mode
的最大值，因此是 29 bits；Hybrid entry 是 259 bits。對較小的 `Rs/Cs` 組態，
若 offline entry 較窄，hardware accounting 會自動保留完整 global tag 所需的最小
Address entry 寬度。核心 logical storage 為
`4 × 29 + 4 × 259 = 1152` bits，不包含 comparator、MUX、controller、NVM/eFuse
load state、ECC 或 SRAM macro packing overhead。預設只有 4 個 device-wide online
entries，表示整顆 device 最多持久保存 4 個不同 repair words；它是 paper-derived
capacity policy，不是每個 SA 各有 4 個 spare words。

`HierarchicalRecamConfig::validateDramConfig()` 會在 CLI 與 scheduler 入口檢查：

- domains × banks/domain × groups/bank 是否等於 total repair groups；
- total subarrays 是否恰為每個 group 4 個；
- cell columns 是否可被 data-word width 整除；
- mode-reused Address CAM 是否至少裝得下 complete global word tag；
- Hybrid CAM 是否容納 Enable、`log2(Rs+Cs)` pointer 與 replacement word。

## 3. 實作對應

| 檔案／類別 | 責任 |
|---|---|
| `HierarchicalRECAM.cpp` | CLI、generated/file input、reference address mapping、CSV output |
| `HierarchicalRecamConfig` | global capacity、engine count、granularity、reference totals |
| `DeviceRepairScheduler` | Tier flow、engine assignment、persistent state、hierarchy metrics |
| `GlobalOnlineRepairPool` | canonical tag、dedup、capacity reservation、overflow |
| `DynamicRepairSimulator` | 保留為一個 4-SA group analyzer |
| `RecamGeometry` | CAM／SRAM 共用的 RECAM entry 數與 logical field width |
| `CamRecamModel`／`SramRecamModel` | 共通 `HardwareMetrics`、`BiraLatencyMetrics` 的 backend model |
| `FaultAddress` | external word+mask BIST packet 與 internal physical-cell address 轉換 |
| `CanonicalExperiment` | 同一 fault set 的 B0/B1/B2/B3 runner 與 long-format reporter |
| `HierarchicalCsvReporter` | device、group、engine CSV |

這是增量遷移：原本 RECAM solver 沒有整份重寫，而是放到新的 device scheduler
下面。Functional per-SA software containers 仍可存在，但 hardware accounting 只按
實體 BIRA engines 計算。

## 4. Repair flow

每個 group 依序執行：

```text
Tier 0 — Local line repair
    buffer CAM capacity = 0
    each SA uses only its own Rs/Cs
    │
    ├── all 4 SAs success -> LOCAL_ONLY
    │
    ▼
Tier 1 — Constrained line sharing (if configured)
    physical ledger allocates borrowed rows/columns
    │
    ├── success -> SHARING_REQUIRED
    │
    ▼
Tier 2 — Global CAM reuse
    derive residual BufferRepairMappings
    canonicalize and deduplicate repair words
    reserve from one device-wide pool
    │
    ├── enough capacity -> CAM_REUSE_REQUIRED or
    │                     SHARING_AND_CAM_REUSE_REQUIRED
    └── insufficient capacity -> UNREPAIRABLE / CAM overflow
```

一個 group 的新 CAM assignments 採 transaction-like commit：只有該 group 所需的
novel tags 全部放得下時才提交，避免 failed solution 留下沒有意義的 partial
persistent state。已經存在於 pool 的相同 tag 不會再次占用容量。

## 5. Full tag 與 word dedup

Global tag：

```text
(domain, bank, group, subarray, row, repair_column)
```

Reference model 的 logical tag width：

```text
domain       3 bits
bank         2 bits
group        6 bits
subarray     2 bits
row          9 bits
word_col     5 bits
------------------
total       27 bits + valid/pointer/metadata
```

在預設 `word` granularity：

```text
word_col = floor(cell_col / data_word_bits)
```

例如 `data_word_bits=256`、同一 row 的 `cell_col=10` 與 `200` 都屬於
`word_col=0`，只消耗一個 online CAM entry。`cell` granularity 則保留原始
cell column，每個不同 cell address 都可形成獨立 entry。

## 6. 建置

在 repository root：

```bash
make hierarchical_recam_b
```

產物：

```text
build/bin/HierarchicalRECAM
```

查看即時 CLI：

```bash
./build/bin/HierarchicalRECAM --help
```

## 7. CLI synopsis

```text
./build/bin/HierarchicalRECAM <Rs> <Cs> [options]
```

`Rs`、`Cs` 是每個 subarray 的 local physical spare rows/columns，必須是正整數。

### 7.1 Device input

| Option | Default | Meaning |
|---|---:|---|
| `--simplified-fault-file PATH` | none | 載入 seven-field device fault list；指定後不使用 generated groups |
| `--fault-file PATH` | none | 載入 `fault_generator/faults.faults` 的 count-framed A/B/C/D lists；與 simplified file 互斥 |
| `--groups N` | `1` | 產生 reference hierarchy 的前 N 個 repair groups，範圍 `1..2048` |
| `--fault-count N` | `3` | 每個 generated 4-SA group 的總 faults |
| `--seed N` | `20260820` | deterministic generator seed |
| `--fault-model uniform\|moderate\|strong\|hotspot` | `uniform` | generated A/B/C/D fault-count distribution；`moderate` 使用 2:4:5:9 權重 |
| `--spatial uniform\|mixed\|clustered` | `mixed` | generated physical-cell spatial distribution |
| `--memory-rows N` | `512` | normal row range `0..N-1` |
| `--memory-columns N` | `8192` | normal cell-column range `0..N-1` |

`--fault-count` 是整個 A/B/C/D group 的總數，不是每個 SA 各 N 個。
`--fault-model`／`--spatial` 只適用 generated input；與兩種 fault-file option
同時指定會被拒絕，避免 file metadata 被錯誤覆蓋。

`--canonical-four --write-area-manifest` 會額外輸出
`hardware_area_manifest.csv`。它只屬於 hierarchical device 的 B0/B1/B2/B3
比較：每個 configuration 各列出 per-BIRA-engine Address/Hybrid store、temporary
fault buffer、standalone counters、fault-collection FIFO，以及一個 device-global
online repair directory。`logical_entries`、`entry_bits` 與 `total_bits` 是提供外部
CAM/SRAM/FIFO/counter 面積模擬器的結構化輸入，不是 foundry area claim。

Address/Hybrid 的 `Rs/Cs` 採該 configuration 所有已評估 Tier-0/1/2 attempt 的
`max(provisioned Rs)`、`max(provisioned Cs)`，故是 conservative envelope；FIFO
採本次事件模擬的最大 observed depth（最小仍配置一 entry）。global online
directory 則獨立於 per-engine BIRA scratch 計數，不能乘上 `--bira-engines`。

`--fault-file` 的 parser 會讀取檔案開頭的 list count，再以每四個 A/B/C/D list
組成一個 repair group；同一組的 `(HBMID, ChannelID, BankID,
SubarrayGroupID)` 必須一致。這正是現有 `fault_generator/faults.faults` 格式，
其第七欄仍是 **physical cell column**。

### 7.2 Hierarchical RECAM

| Option | Default | Meaning |
|---|---:|---|
| `--online-global-reuse-entries N` | `Rs+Cs` | 明確覆寫整個 device 共用的 finite Tier-2 capacity；可設 0 |
| `--bira-engines N` | `1` | 實體 offline BIRA engines；groups 以 round-robin 分派 |
| `--data-word-bits N` | `256` | word canonicalization width |
| `--cam-granularity word\|cell` | `word` | Tier-2 entry 的 data-word 或 cell granularity |

未指定 `--online-global-reuse-entries` 時，capacity 依 RECAM paper Table I
由 Address CAM 大小導出：

```text
paper Address CAM entries = Rs + Cs
paper Hybrid CAM entries  = Rs × (Cs − 1) + Cs × (Rs − 1)
online reuse entries      = Address CAM entries
```

例如 `Rs=Cs=2` 時，Address CAM、Hybrid CAM 與 online reuse capacity 都是
4 entries。這些 entries 在 offline mode 收集／分析 faults，完成 repair solution
後於 online mode 重新用作 repair address 與 replacement data，不另計一個 private
CAM。

明確指定 N 時，CSV 會標記 `EXPLICIT_OVERRIDE`。若 N 大於 `Rs+Cs`，它代表擴充
實體 **CAM** 的架構實驗，CAM hardware accounting 會把 Address CAM 與 Hybrid
CAM 的最低實體 entries 放大到 N。SRAM backend 的 baseline bit accounting 已使用
同一份 `RecamGeometry`；將 explicit enlarged online capacity 對應為 SRAM macro
depth/width 的 sensitivity study 前，應另明確選定 array packing policy，不可直接
與 baseline SRAM bit 數混為同一個 macro 實作。建議 enlarged-CAM sweep：

```text
4, 8, 16, 32, 64, 128, 256
```

### 7.3 Tier-1 sharing

| Option | Default | Meaning |
|---|---:|---|
| `--topology none\|directional\|edge\|global` | `none` | physical sharing topology |
| `--shared-rows N` | `0` | 可共享／重新分配的 rows per relevant owner |
| `--shared-columns N` | `0` | 可共享／重新分配的 columns per relevant owner |
| `--local-first` | enabled | local success 時不做不必要的 borrowed configurations |
| `--max-borrows N` | `3` | one group 的最大 borrowed physical lines |
| `--solution-take legacy\|early\|group` | `legacy` | 四個 SA 的 solution selection policy |

Topology：

- `none`：每個 SA 只用 local `Rs/Cs`。
- `directional`：A→C、D→B 借 row；B→A、C→D 借 column。
- `edge`：row 在 A↔C、B↔D；column 在 A↔B、C↔D；無 diagonal sharing。
- `global`：從相同 physical total 中抽出 group-level row/column pools，四個 SA
  都能請求；不是額外新增 spares。

`shared_rows <= Rs` 且 `shared_columns <= Cs`。若 topology 為 `global`，CLI
會自動設定：

```text
local rows / SA = Rs - shared_rows
local cols / SA = Cs - shared_columns
global rows      = 4 × shared_rows
global cols      = 4 × shared_columns
```

Solution Take Policy 不改變 Tier-0/Tier-1/Tier-2 ownership。`early` 在每次 group
analysis 內依 A→D 選解且不回溯；`group` 對四個 SA 的 RECAM-valid compressed
states 做完整組合選擇。Tier-2 仍只在 group solution 選定後，將該解的 BUFFMAP
requests transactionally 提交到 device-wide global CAM pool；不進行跨 group
solution reordering。

### 7.4 Output

| Option | Default |
|---|---|
| `--output-dir PATH` | `reports/device/hierarchical_recam` |
| `--write-fault-corpus` | off；啟用後寫出 `fault_corpus.txt` |

三份 CSV 會以 truncate 模式寫入指定目錄；重跑同一路徑會覆寫同名 CSV。
`fault_corpus.txt` 使用可由 `--simplified-fault-file` 再次載入的七欄 physical-cell
格式，並在註解 header 保存 fault model、spatial model 與 seed。

### 7.5 Canonical B0/B1/B2/B3 comparison

`--canonical-four` 會啟用 decoupled FIFO，並以完全相同的 input fault groups
依序建立四個獨立的 scheduler／ledger／global pool：

| ID | Backend | Tier-1 sharing |
|---|---|---|
| `B0_CAM_NO_SHARING` | CAM RECAM | off |
| `B1_CAM_SHARING` | CAM RECAM | on (`--topology` 等 sharing options) |
| `B2_SRAM_NO_SHARING` | SRAM RECAM | off |
| `B3_SRAM_SHARING` | SRAM RECAM | on |

此 runner 強制 B0=B2、B1=B3 的 functional repair outcome、borrowed line 與
online reuse allocation 相同；不同的是 BIRA storage／search latency hardware
model。SRAM search 可設定：

```text
--sram-policy serial | chunked:N | wide
--sram-matrix-policy scan | shadow
--registered-sram-search
```

`shadow` 使用 collection-only latency profile：Address SRAM 採
`strict_occupied_scan`，空表 search 為 0 cycle，只有 row/column matches 都找到才
early terminate；CAM 固定為每 fault 1 compare + 1 update = 2 cycles。CAM 與
SRAM 的 matrix conversion／fault-free analyzer 仍執行以維持功能正確性，但不計入
此 profile 的 modeled latency。

典型指令：

```bash
./build/bin/HierarchicalRECAM 2 2 \
  --canonical-four --groups 32 --fault-count 8 --seed 20260820 \
  --topology edge --shared-rows 1 --shared-columns 0 --max-borrows 1 \
  --sram-policy chunked:2 --output-dir reports/device/canonical_four/example
```

輸出包括：

```text
canonical_four_summary.csv       # 4 rows, common hardware/repair schema
canonical_four_groups.csv        # one row per configuration × repair group
B0_CAM_NO_SHARING/RemapTable.txt
B1_CAM_SHARING/RemapTable.txt
B2_SRAM_NO_SHARING/RemapTable.txt
B3_SRAM_SHARING/RemapTable.txt
```

不要把四組 RemapTable 混成同一份下游輸入；應根據你要交付的 architecture
configuration 選取其中一個完整 `RemapTable.txt`。

## 8. Generated-input example

```bash
./build/bin/HierarchicalRECAM 2 2 \
  --groups 32 \
  --fault-count 8 \
  --seed 20260820 \
  --memory-rows 512 \
  --memory-columns 8192 \
  --topology edge \
  --shared-rows 1 \
  --shared-columns 0 \
  --local-first \
  --max-borrows 1 \
  --bira-engines 1 \
  --data-word-bits 256 \
  --cam-granularity word \
  --output-dir reports/device/hierarchical_recam/edge_m1_paper_cam
```

Generated group index 使用：

```text
domain = index / (4 banks × 64 groups)
bank   = (index mod 256) / 64
group  = index mod 64
```

所以 `--groups 32` 模擬 domain 0、bank 0 的 group 0..31。它是 scaled
device experiment，不代表完整 2048 groups 都被模擬。

## 9. File-input example

格式每行七個整數：

```text
HBMID ChannelID BankID SubarrayGroupID SubarrayID Row Col
```

例如：

```text
// one device, two repair groups
0 0 0 0 0 10 20
0 0 0 0 0 11 40
0 0 0 0 2 12 60
0 0 0 1 1 10 20
```

執行：

```bash
./build/bin/HierarchicalRECAM 2 2 \
  --simplified-fault-file path/to/device_faults.txt \
  --memory-rows 512 \
  --memory-columns 8192 \
  --output-dir reports/device/hierarchical_recam/file_run
```

規則：

- blank、`#`、`//` lines 會被忽略；
- groups 依 `(HBMID, ChannelID, BankID, SubarrayGroupID)` 分組與排序；
- `SubarrayID` 必須是 `0..3`；
- row/column 必須落在 CLI 指定的 memory dimensions；
- 一次 run 只能含一個 HBMID，因為每個 device 有自己的 global pool；
- 多 device input 應先拆成每個 HBMID 一個檔案／run。

### 9.1 External BIST word address versus fault-file address

`Fault`、`--simplified-fault-file`、`--fault-file` 與 `RemapTable.txt` 的七欄
都採 physical address：`Col = cell_col`。這是下游 remap validator 與既有
整合端的契約。

若 BIST/測試端輸出的是一個 data word 的 fault syndrome，請在 port boundary
使用 `BistFaultPacket`，不要直接餵給 physical fault loader：

```text
address:  HBMID ChannelID BankID SubarrayGroupID SubarrayID Row WordCol
sideband: failMask[word_bits]
```

例如 `word_bits=256`、`WordCol=5`、fail mask 的 bit 17 set，internal physical
fault 為 `CellCol=5×256+17=1297`。同一 word 的多個 set bits 解成多筆 physical
fault，保留 exact-cell remap coverage；online reuse capacity 則只以 `WordCol=5`
計一筆 replacement word。

### 9.2 A/B/C/D serial BIST and FIFO

開啟 `--bist-decoupled-fifo`（`--canonical-four` 會自動開啟）後，每個 group
以 **A→B→C→D** 串行，且每個 subarray 以 row-major、word-column-major 掃描：

```text
R = rows, C = physical cell columns, W = word bits, Q = C/W
S = SubarrayID (A=0, B=1, C=2, D=3), Tw = bist cycles/word

scan_index      = ((S × R + row) × Q) + word_col
fault_arrival   = group_start + (scan_index + 1) × Tw
bist_group_done = group_start + 4 × R × Q × Tw
```

可用 `--bist-cycles-per-word N` 改變 `Tw`。FIFO 為 unbounded、single-server、
decoupled baseline：BIST 不會因 collection 壅塞停住，所以
`bist_backpressure_cycles=0`；壅塞反映在 queue depth、queue wait 與
`latency_after_bist_cycles`。

## 10. 輸出

### 10.1 `device_summary.csv`

一列代表本次 modeled device/subset。主要欄位：

| Field | Meaning |
|---|---|
| `architecture_version` | `WoW-v1.0` |
| `simulator_version` | `Hierarchical-RECAM-v2.1` |
| `cam_scope` | `GLOBAL_LOGIC_DIE` |
| `modeled_groups`／`repairable_groups` | 本次提供及成功 groups |
| `modeled_banks/domains` | input 實際涵蓋的 hierarchy nodes |
| `device_success` | 本次 modeled groups 是否全部成功 |
| `global_cam_capacity` | device-wide capacity |
| `global_cam_capacity_source` | `PAPER_RS_PLUS_CS` 或 `EXPLICIT_OVERRIDE` |
| `paper_*_cam_entries_per_structure` | 由 Rs/Cs 導出的論文 Address/Hybrid CAM entries |
| `physical_*_cam_entries_per_structure` | 納入 explicit enlargement 後的每套實體 entries |
| `total_physical_*_cam_entries` | 每套 entries × BIRA engine structures |
| `global_cam_entries_reserved` | final persistent occupancy |
| `global_cam_peak_occupancy` | peak committed occupancy |
| `global_cam_utilization` | `entries_reserved / capacity`；capacity 0 時回報 0 |
| `cam_request_count` | dedup 後的 Tier-2 requests |
| `cam_allocation_*` | successful／failed requests |
| `cam_overflow_rate` | `failure_count / request_count` |
| `*_requested_before/after_dedup` | word canonicalization 效果 |
| `groups/banks/domains_requesting_cam` | Tier-2 demand 的 hierarchy coverage |
| repair-source counts | 四種 success source 與 unrepairable count |
| `address_cam_structures` | 等於 `bira_engine_count`，不是 SA count |
| `separately_counted_online_cam_pools` | mode reuse baseline 為 0 |

`device_success` 只針對本次 input 的 groups。例如 `--groups 32` 成功表示前
32 groups 全成功，不是完整 2048-group device yield。

### 10.2 `groups.csv`

每列一個 repair group：

- hierarchy address 與 assigned BIRA engine；
- `local_success_mask`（A/B/C/D）；
- Tier-0/Tier-1 outcome；
- borrowed R/C；
- CAM words needed/allocated/failures；
- allocation 前後 global occupancy；
- `repair_source` 與 `final_success`；
- technology-neutral BIRA work breakdown；
- BIST/event timeline 與 after-BIST latency（僅在 `bist_timeline_modeled=1`
  時有效）。

共用 latency 欄位分成兩類：

```text
Work accounting:
  bira_fault_collection_work_cycles
  bira_repair_analysis_work_cycles
  bira_sharing_allocation_work_cycles
  bira_total_work_cycles

Event timeline:
  first_fault_arrival_cycle
  last_fault_arrival_cycle
  bist_completion_cycle
  fault_collection_completion_cycle
  solution_ready_cycle
  fault_collection_latency_after_bist_cycles
  latency_after_bist_cycles
  maximum_fault_queue_depth
  total_fault_queue_wait_cycles
  bist_backpressure_cycles
```

CAM/SRAM aggregate models 都會填 work accounting。occupied-prefix 使 SRAM 每筆
fault 的 service time 可能不同；開啟 BIST FIFO 後會直接使用 adapter 保存的逐 fault
service cycles，而不是以 aggregate cycles 除以 fault count。之後
`FaultAddress::SerialBistSchedule` 會由 physical address 產生 arrival events，
並將 `bist_timeline_modeled=1`。未開啟時 timeline 欄位的 0 表示「未建模」，
不是量測結果為零。

`latency_after_bist_cycles = max(0, solution_ready_cycle - bist_completion_cycle)`。
這是 group-relative BIST completion 到 solution ready 的 event-model 結果；它不是
多 groups 共用 engine 時的 device-wide wall-clock completion。現階段 B0/B1 的
CAM event timeline 在 collection-only profile 不計 matrix/analyzer；不要把其值宣稱為論文
RECAM 的完整 RTL dynamic-overlap timing。

Repair source 恰為以下之一：

```text
LOCAL_ONLY
SHARING_REQUIRED
CAM_REUSE_REQUIRED
SHARING_AND_CAM_REUSE_REQUIRED
UNREPAIRABLE
```

### 10.3 `bira_engines.csv`

每列一個 physical engine：

- `groups_processed`；
- `scratch_clear_count`；
- `transient_faults_remaining`，正常完成後應為 0；
- `total_cycles`。

多 engines 的 work-only completion proxy 是所有 engines 中最大的 `total_cycles`，
因為 groups 採 static round-robin。它不是 `latency_after_bist_cycles`；後者定義為：

```text
max(0, solution_ready_cycle - bist_completion_cycle)
```

## 11. 如何判讀 global CAM contention

假設 capacity 2，三個 groups 各需要一個不同 tag：

```text
group 0: occupancy 0 -> 1, success
group 1: occupancy 1 -> 2, success
group 2: occupancy 2 -> 2, overflow/failure
```

這與錯誤的 private pool 模型不同；private model 會讓三個 groups 都看到兩個
free entries，因而高估 repairability。

若兩個 requests canonicalize 成完全相同的 full tag，第二次是 existing hit，
不新增 occupancy。不同 group 即使 row/word_col 相同，仍因 group field 不同而是
不同 tags。

## 12. 建議實驗

1. 先執行不帶 capacity override 的 paper baseline；再固定 sharing policy，
   sweep enlarged/restricted global entries：

   ```text
   4, 8, 16, 32, 64, 128, 256
   ```

2. 固定 capacity，增加 `--groups`，觀察 occupancy 與 overflow。
3. 比較 `none` 與 row-only `edge`，量測 Tier-1 是否降低 Tier-2 demand。
4. Sweep `--bira-engines 1,2,4,8`，比較 offline completion proxy 與硬體數量。
5. 比較 `word` 與 `cell` granularity，但必須在圖表和 CSV metadata 中清楚區分。

完整實驗與 plotting 工作流見 [EXPERIMENTS.md](EXPERIMENTS.md)。

## 13. 測試

```bash
make test_hierarchical_recam
make test_fault_address_bist
make test_sram_fault_collection_fifo
make test_recam_common_models
make test_canonical_experiment
make test_canonical_sweep
```

測試對應 v2 requirements：

- no physical CAM multiplication；
- scratch reuse and persistent solution；
- capacity-2 global contention；
- complete tag disambiguation；
- 256-bit same-word dedup；
- sharing reduces CAM demand。
- BIST word+mask ↔ physical-cell conversion，與 A/B/C/D serial boundary；
- SRAM serial／chunked／wide、registered search、shadow matrix 的 FIFO golden；
- B0 frozen golden，以及 B0/B1/B2/B3 CAM/SRAM logical equivalence；
- canonical sweep 產生完整 4-row schema。

SRAM FIFO golden fixture 在
`tests/golden/sram_fault_collection_fifo_expected.csv`。基本 case `Rs=Cs=2`、
shadow + chunked P=2、三個依序新增的 pivot、arrival `{1,2,3}`、BIST done=5
的手算結果為：

```text
search rounds={0,1,1}, service={1,2,2}
collection work=5, final analysis=0, collection done=6
queue wait=1, max queue depth=1, solution ready=6
latency after BIST=1, BIST backpressure=0
```

已另外實測 `fault_generator/faults.faults` 與既有
已另外實測舊輸出 `reports/SharedLine_SRAM/RemapTable.txt`（scope-first 整理前的
legacy artifact）的 compatibility validator：250 patterns、
63 options、4032/4032 covered、0 invalid。以同一份 fault file、`Rs=Cs=6`、
`word_bits=16` 執行 canonical runner 後，B0/B2 各輸出並驗證 56 個 options，
B1/B3 各 86 個 options；所有 emitted options 均為 valid physical remaps。

## 14. 常見錯誤

### 把 `--groups` 當 Monte Carlo trials

在這個 app 裡，每個 generated group 具有不同 hierarchy address，並共享同一 pool。
它們不是互相獨立、每次重置硬體狀態的 trials。需要估計 probability 時，應建立
多個獨立 output directories／seeds，每次各自模擬一個 device/subset，再做外部彙總。

### 用 `DynamicSpareSharing` 產生 global-device 結果

該 app 的 `runs` 是 independent group samples；它不保存跨 run global occupancy。
它適合 Tier-1 policy exploration，不適合 device-wide finite CAM 結論。

### 將 functional software buffer 當 physical CAM 數量

Tier-2 analysis 可以用 software containers 發現 residual demand；physical limit 只由
`GlobalOnlineRepairPool` 與 hardware accounting 決定。不可將 solver 內暫存 vector
乘上 SA count當成面積。

### 將 C++ loop 次數直接當 RTL cycles

`bira_*_work_cycles` 是目前的 operation-work proxy。當
`bist_timeline_modeled=0` 時，未建模 BIST scan、fault arrival queue、full RTL、
memory command timing、NoC 或 bank scheduling，不應把 work cycles 宣稱為
after-BIST latency 或晶片實測 latency。

## 15. Sources 與研究假設

主要來源：

- Micron, “Input/Output Connections of Wafer-on-Wafer Bonded Memory and
  Logic,” US20230048628A1：top-level WoW example。
  https://patents.google.com/patent/US20230048628A1/en
- Y. Kim et al., “A Case for Exploiting Subarray-Level Parallelism (SALP) in
  DRAM,” ISCA 2012：tile/subarray organization。
  https://doi.org/10.1109/ISCA.2012.6237032
- J. Yoon et al., “A Built-In Self-Repair With Maximum Fault Collection and
  Fast Analysis Method for HBM,” IEEE TCAD 2025：RECAM/BISR/BIRA。
  https://doi.org/10.1109/TCAD.2024.3499903

Research-specific assumptions：

- one 16-Mbit section = one 4-SA repair group；
- one modeled SA = `512×8192` cells；
- 256-bit replacement-word baseline；
- constrained Tier-1 sharing topology；
- finite global CAM allocation policy。

改變 bank/group/SA geometry、data width、spare baseline、BIRA engine ownership 或
CAM scope 時，應遞增 architecture version，不能仍標記為 `WoW-v1.0`。
