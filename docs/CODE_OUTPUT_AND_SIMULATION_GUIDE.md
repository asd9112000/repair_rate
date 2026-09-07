# 目前程式可做的模擬與輸出

> 用途：提供新加入成員快速判斷「該跑哪個程式、結果會寫到哪裡、CSV 可以讀出什麼」。
>
> 最後依據：目前 C++ simulator、Makefile 與 `docs/` 的 current 文件（2026-09-08）。

## 先記住：結果有三個 scope，不能混在一起比較

| Scope | 對應程式 | 模擬的資源範圍 | 可以回答的問題 |
|---|---|---|---|
| `legacy` | `basicPEarray`、`RedundantRate`、`SharedLine`、`SharedLine_SRAM` | 固定的 legacy PE/4-PE pattern | 原始 RECAM 行為、固定 sharing baseline、remap 格式 |
| `group` | `DynamicSpareSharing`、`DynamicSpareSharing_SRAM_RECAM` | 一個獨立 4-subarray (A/B/C/D) repair group | group 內 spare sharing policy、Monte Carlo repair rate、SRAM 搜尋代價 |
| `device` | `HierarchicalRECAM` | 一顆 WoW-v1.0 modeled device；所有 modeled groups 共用有限的 Tier-2 online CAM pool | BIRA engine reuse、跨 group CAM 競爭/overflow、device repairability |

`group` 的多個 run 是彼此獨立的樣本，**不會**累積全 device 的 online CAM occupancy。因此不能拿 group-level repair rate 與 device-level repair rate 平均、排序，或把前者當成 canonical B0–B3 的一列。

## 整體 repair flow

```text
faults
  └─ Tier 0：每個 SA 的 local spare rows/columns
       └─ Tier 1：同一個 4-SA group 內借用 physical spare lines（選配）
            └─ Tier 2：device-wide online CAM/SRAM reuse（僅 hierarchical device path）
```

Tier 1 是 line remapping；Tier 2 是 repair-word reuse，兩者消耗的資源不同。SRAM-RECAM 保持 RECAM repair semantics，改變的是 fault information 的儲存/搜尋與延遲模型，不是另一套 repair oracle。

## 可執行程式與適用情境

| 程式 | Make target | 主要模擬 | 主要輸出 |
|---|---|---|---|
| `basicPEarray` | `make basic_pe_b` | 單 PE RECAM solver 的除錯；讀固定 fault file | `reports/basicPEarray/` 的 fault map、solution vectors/matrices、PE matrix、repair record/report |
| `RedundantRate` | `make rdr_b` | 同一批 faults 下 `(Rs,Cs)`、少一條 row、少一條 column、兩者都少一條的比較 | `reports/RedundantRate/RepairRecordFile.txt`、`RepairReport.txt`、各配置 solution files |
| `SharedLine` | `make sl_b` | legacy 固定 2×2 Ring sharing baseline | `reports/SharedLine/RepairRecord.txt`、repair report；也會在 stdout 印 repair rate |
| `SharedLine_SRAM` | `make sl_sram_b` | legacy sharing + remap/runtime latency 描述 | `reports/legacy/sharedline_sram/RemapTable.txt`、`RemapTable_simplified.txt`，以及 legacy report |
| `DynamicSpareSharing` | `make dynamic_sharing_b` | 4-SA group 的 topology、borrow policy、solution selection、fault model Monte Carlo | `attempts.csv`、`runs.csv`、`summary.csv`；可選 remap/runtime table |
| `DynamicSpareSharing_SRAM_RECAM` | `make dynamic_sram_recam_b` | 同一 group 中 Serial/Chunked/Wide SRAM-RECAM search policy 的功能等價、latency、bits/area proxy | `sram_recam_metrics.csv`、`hardware_cost.json`（及每 policy 的 cost 明細） |
| `HierarchicalRECAM` | `make hierarchical_recam_b` | device-wide hierarchy、shared BIRA scratch、global online CAM pool | `device_summary.csv`、`groups.csv`、`bira_engines.csv`；canonical 模式另有 B0–B3 檔案 |

建置全部 C++ simulator 與工具可執行 `make all`。輸入 fault address 的內部單位是 physical cell column；不要把 fault file 第七欄 `Col` 當成 word column。

## 1. Group-level dynamic spare sharing

適合研究「四個 SA 如何分享原本存在的 spare lines」。可選 2×2 的 `none`、`directional`、`edge`、`global`，或 1×4 row-only 的 `none`、`pair`、`neighbor`、`global`。`global` 只是把相同 physical spare total 移至 group pool，並非新增 spare。

```bash
make dynamic_sharing_b
./build/bin/DynamicSpareSharing 2 2 \
  --topology edge --shared-rows 1 --shared-columns 0 \
  --local-first --fault-count 20 --runs 1000 --seed 20260820 \
  --output-dir reports/group/dynamic_spare_sharing/example
```

### 主要輸出

| 檔案 | 一列代表什麼 | 先看哪些欄位 |
|---|---|---|
| `attempts.csv` | 一個 SA 的一次 RECAM analysis attempt 與候選解 | fault/attempt、CAM/matrix usage、valid candidates |
| `runs.csv` | 一次 4-SA group sample 的最終選解與資源使用 | `group_repair_success`、borrowed/lent rows/columns、`analysis_cycles`、solution IDs、valid-solution bitmaps |
| `summary.csv` | 相同 config 的所有 runs 聚合 | `repair_rate`、`baseline_repair_rate`、`sharing_gain`、borrow 統計、CAM/matrix peak、P95/average cycles、cost proxy |
| `RemapTable.txt`、`RemapTable_simplified.txt` | 單一 config 的 selected remap（需 `--write-remap-tables`） | `MAP` line remap 與 `BUFFMAP` exact-address runtime repair |
| `RuntimeRepairTable.csv` | runtime repair lookup placement/latency（需 remap output） | backend、slot、search rounds、read-hit/write-hit latency |

大量 sweep 請加 `--summary-only`，只寫 `summary.csv`。remap output 只能用於單一 configuration，不能和 sweep 同時使用。常用選解策略為 `--solution-take legacy|early|group`：它只決定四個 SA 怎麼從 RECAM-valid solutions 選出組合，不改變 topology 或 physical spare ownership。

## 2. Group-level SRAM-RECAM policy comparison

適合比較 SRAM 搜尋平行度的 latency/bit/area proxy，同時檢查 repair result 是否與 CAM baseline 一致。

```bash
make dynamic_sram_recam_b
./build/bin/DynamicSpareSharing_SRAM_RECAM 2 2 \
  --topology directional --shared-lines 1 --local-first \
  --fault-count 20 --runs 100 --seed 20260820 \
  --policies serial,chunk2,chunk4,wide --matrix-policy scan \
  --output-dir reports/group/sram_recam/example
```

| 檔案 | 用途 | 關鍵輸出 |
|---|---|---|
| `sram_recam_metrics.csv` | 每個 search policy 一列的完整比較表 | repair rate/與 CAM baseline 是否一致、search rounds/cycles、BIRA cycles、runtime hit/miss、SRAM physical bits、comparators、normalized area proxy |
| `hardware_cost.json` | 各 SA 的 SRAM geometry 與 area proxy 明細 | address/hybrid/runtime SRAM depth/width/bits、comparator count、area proxy |

`serial`、`chunked:N`（如 `chunk2`）與 `wide` 應保有相同 functional repair outcome；若 `repair_result_matches_cam_baseline=0`，不可把該筆 latency/area 結果當成等價設計比較。

## 3. Hierarchical device-level RECAM

這是研究 device-wide CAM capacity 的唯一正確入口。其 resource ownership 為：每 SA 的 local spares、每 group 的 Tier-1 borrowing、由 `--bira-engines` 決定數量的 shared offline BIRA scratch，以及整顆 device **唯一且有限**的 global online CAM pool。

```bash
make hierarchical_recam_b
./build/bin/HierarchicalRECAM 2 2 \
  --groups 32 --fault-count 8 --seed 20260820 \
  --topology edge --shared-rows 1 --shared-columns 0 \
  --bira-engines 1 \
  --output-dir reports/device/hierarchical_recam/example
```

| 檔案 | 粒度 | 主要解讀 |
|---|---|---|
| `device_summary.csv` | 一次 modeled device run | group/bank/domain/device repairability；global CAM capacity、reserved entries、dedup 前後 request、overflow；BIRA storage/bit accounting；最大 after-BIST latency |
| `groups.csv` | 每個 repair group | Tier-0/1/2 成功、借用 rows/columns、CAM words needed/allocated、occupancy before/after、repair source、BIRA work/FIFO/BIST timing、selected solutions |
| `bira_engines.csv` | 每個 physical BIRA engine | groups processed、scratch clear count、transient state、累積 work proxy `total_cycles` |
| `fault_corpus.txt` | resolved seven-field fault list（需 `--write-fault-corpus`） | 讓同一份 fault corpus 可重跑/追溯 |

每份 hierarchical CSV 都必須保留以下 metadata，否則不要當成 global-CAM 結果：

```text
architecture_version = WoW-v1.0
simulator_version    = Hierarchical-RECAM-v2.1
cam_scope            = GLOBAL_LOGIC_DIE
```

`scratch_clear_count` 只表示 offline temporary scratch 被清除；已 commit 的 online CAM assignment 不會因此釋放。`bira_engines.csv` 的 `total_cycles` 是累積 work proxy，不應直接解讀為完整 wall-clock timeline。

## 4. Canonical B0–B3：同一份 faults 比較 CAM、sharing、SRAM

若問題是「CAM vs. SRAM」或「有/無 sharing」的 device-level 正式比較，應使用 `--canonical-four`，一次產生完全相同 fault set 下的四組結果：

| ID | BIRA storage | Tier-1 sharing |
|---|---|---|
| B0 | CAM | 無 |
| B1 | CAM | 有 |
| B2 | SRAM | 無 |
| B3 | SRAM | 有 |

```bash
./build/bin/HierarchicalRECAM 2 2 \
  --canonical-four --groups 32 --fault-count 8 --seed 20260820 \
  --topology edge --shared-rows 1 --shared-columns 0 --max-borrows 1 \
  --sram-policy chunked:2 \
  --write-area-manifest \
  --output-dir reports/device/canonical_four/example
```

| 檔案 | 用途 |
|---|---|
| `canonical_four_summary.csv` | B0–B3 的 repair rate、global reuse reservation/overflow、BIRA completion/after-BIST latency、storage bits、Tier result counts；正式比較的主表 |
| `canonical_four_groups.csv` | B0–B3 每 group 的 repair source、CAM allocation、BIRA/FIFO/BIST timing；用於定位差異 |
| `hardware_area_manifest.csv` | 僅加 `--write-area-manifest` 時產生；逐項列出 BIRA engine store、FIFO、counter、device global repair directory 的 entries/bits/instances |
| `B0/`、`B1/`、`B2/`、`B3/` 下的 `RemapTable.txt`、`RemapTable_simplified.txt` | 各 architecture configuration 的 physical remap；下游一次只能選其中一個配置 |

程式會檢查 B0/B2、B1/B3 的 functional outcome 是否一致。因此 B0–B3 的 CAM/SRAM 比較可比較 latency 與 hardware proxy，但仍須把 sharing 有無與 storage backend 兩個維度分開解讀。

## 5. 建議輸出目錄與分析規則

新實驗建議使用 scope-first 路徑：

```text
reports/
├── legacy/<experiment>/<run-id>/
├── group/<experiment>/<run-id>/
├── device/<experiment>/<run-id>/
└── studies/<study>/<run-id>/
```

每次正式 run 至少保留完整 command、seed、`Rs/Cs`、fault model/geometry、topology/borrowing policy、CAM/SRAM capacities，以及原始 CSV。`studies/` 只是多個 scope 的編排容器，不是第四種結果語意；其中的 group、SRAM、device outputs 仍需分開分析。

輸出中的 `hardware_cost_proxy_bits`、`normalized_*_area`、BIRA storage bits 是 architectural model/proxy，不能直接宣稱為 foundry macro area 或 RTL sign-off timing。RTL 的 synthesis 與 cycle-accurate outputs 另放在 `results/`，不能與 C++ simulator 的 `reports/` 混合。

## 最快的選擇方式

```text
想重現舊 paper / 固定 sharing？             -> SharedLine / SharedLine_SRAM
想比較四個 SA 的 sharing topology？         -> DynamicSpareSharing
想比較 Serial/Chunked/Wide SRAM 搜尋？       -> DynamicSpareSharing_SRAM_RECAM
想研究跨 group CAM occupancy 或 BIRA reuse？ -> HierarchicalRECAM
想在同 faults 下比較 CAM、SRAM、有無 sharing？ -> HierarchicalRECAM --canonical-four
```

相關權威文件：`docs/ARCHITECTURE.md`（resource ownership）、`docs/EXPERIMENTS.md`（CLI/sweep）、`docs/HIERARCHICAL_RECAM.md`（device 模型與 CSV）、`docs/RECAM_SPEC.md`（RECAM semantics）、`docs/SRAM_RECAM_ARCHITECTURE_GUIDE.md`（SRAM-RECAM 解讀）、`docs/REPORTS.md`（scope 與 artifact 規則）。
