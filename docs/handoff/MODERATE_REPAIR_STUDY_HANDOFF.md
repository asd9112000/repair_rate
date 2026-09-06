# Moderate Repair Study 交接文件

> 文件狀態：Handoff
> 適用範圍：cross-cutting / moderate-study
> 建立時間：Unknown
> 最後修改時間：2026-09-07T00:00:00+08:00
> 本文件權威主題：moderate study 的階段性狀態、結果摘要與後續交接
> 注意：本文件是時間點快照，不取代架構與行為規格

## 1. 文件目的與目前狀態

本文件交接本次對話完成的 moderate fault repair study。目標是讓後續研究者或
agent 不需要重新推導實驗意圖，即可：

- 理解 group-level 與 device-level 架構邊界；
- 重現 smoke、screen、confirm 實驗；
- 讀取、驗證及重新繪製現有結果；
- 在不改變 repair semantics 的前提下擴充 seeds、groups 或 fault model；
- 避免把不同 resource ownership 的 repair rate 混成同一條曲線。

截至 2026-09-01，以下 profiles 均已完成：

| Profile | Group runs/point | Device groups/point | Seeds | 狀態 |
|---|---:|---:|---|---|
| `smoke` | 50 | 2 | `20260820` | 完成、驗證通過 |
| `screen` | 150 | 32 | `20260820` | 完成、驗證通過 |
| `confirm` | 500 | 64 | `20260820,20260821,20260822` | 完成、驗證通過 |

結果位於：

```text
reports/studies/moderate_repair/{smoke,screen,confirm}/
```

## 2. 研究目的

本實驗比較下列因素對 repair behavior、latency 與 cost proxy 的影響：

1. local spare 數量：`Rs=Cs=2` 與 `Rs=Cs=3`；
2. group 內 spare-line sharing：No Sharing、Directional、Pairwise/Edge、Global；
3. CAM-RECAM 與 SRAM-RECAM BIRA backend；
4. SRAM associative search：Serial、Chunked P=2、Chunked P=4、Wide；
5. SRAM matrix policy：scan rebuild 與代表性 Chunked P=2 shadow matrix；
6. moderate imbalance fault model 下的 repair rate；
7. spare-cell-equivalent、BIRA modeled bits 與 BIRA latency tradeoff；
8. device-wide finite online CAM contention 對 group repairability 的影響。

主要研究問題為：

- sharing 能否在不增加 physical spare-cell total 的情況下提高 repair rate？
- 增加 Rs/Cs 的 50% spare cost，能換得多少 repair-rate 改善？
- SRAM search policy 的 comparator cost 與 BIRA latency 如何取捨？
- group-level 最佳 policy 在 device-wide finite CAM 下是否仍然最佳？
- CAM 與 SRAM backend 是否維持完全相同的 functional repair outcome？

## 3. 不可混用的架構邊界

本研究刻意保留兩種不同 scope：

| Scope | Simulator | 一筆 run 的語意 | CAM ownership |
|---|---|---|---|
| Group-level | `DynamicSpareSharing`、`DynamicSpareSharing_SRAM_RECAM` | 一個獨立 4-SA repair group sample | 每個 sample 獨立，不累積跨 group occupancy |
| Device-level | `HierarchicalRECAM --canonical-four` | 同一 modeled device/subset 中的多個 repair groups | 一個 device-wide finite online CAM pool |

因此：

- group-level repair rate 與 device-level repairable-group fraction 必須分圖；
- group-level 最佳 policy 只能作為 device 實驗候選，不能直接視為 device 最佳；
- legacy `SharedLine` 結果不可插入 B0–B3 device comparison；
- device 結果必須保留 `WoW-v1.0`、`Hierarchical-RECAM-v2.1`、
  `GLOBAL_LOGIC_DIE` metadata。

本次 confirm 正好觀察到此差異：`Rs=Cs=2` 的 Global m=1 在 group-level 整體
排名最佳，但在 device-level、faults/group=24 時低於 no-sharing。這不是資料矛盾，
而是 finite global CAM contention 與 resource ownership 不同造成的結果。

## 4. 固定實驗矩陣

唯一的 machine-readable source of truth 是：

```text
experiments/moderate_repair_study.json
```

固定參數：

| Axis | Values |
|---|---|
| Memory geometry | `512 × 8192` physical cells / SA |
| Data word | 256 bits |
| Rs/Cs | `(2,2)`、`(3,3)` |
| Faults / 4-SA group | `16,24,32,48` |
| Fault-count model | `moderate`，A/B/C/D 權重 `2:4:5:9` |
| Spatial model | `mixed` |
| Sharing capacity | m=1，同時配置一條 shared row 與一條 shared column |
| Maximum group borrows | 3 |
| Global online reuse capacity | paper baseline `Rs+Cs` |
| SRAM device policy | `chunked:2`、scan matrix |

Screen 會比較所有四種 group policies：

```text
no_sharing
directional_m1
pairwise_edge_m1
global_m1
```

Confirm 不重新執行已淘汰的 policy，而是讀取 screen 的：

```text
reports/studies/moderate_repair/screen/selected_policies.json
```

本次選擇結果：

| Rs=Cs | Device candidate | Confirm 保留的 sharing policies |
|---:|---|---|
| 2 | Global m=1 | Global m=1、Pairwise/Edge m=1 |
| 3 | Pairwise/Edge m=1 | Pairwise/Edge m=1、Global m=1 |

每個 Rs/Cs 另保留 No Sharing 作為 baseline。

## 5. 指標定義

### 5.1 Repair rate

Group-level：

```text
repair_rate = repaired independent group samples / simulation runs
```

Device-level：

```text
repair_rate = repairable modeled groups / modeled groups in this device subset
```

Device `--groups 64` 的結果只代表該 seed 的 64-group subset，不是完整 2048-group
device yield。

### 5.2 Spare cost

本研究採用 cell-capacity proxy：

```text
spare-cell-equivalent/group
    = 4 × (Rs × memory_columns + Cs × memory_rows)
```

在 `512 × 8192` geometry 下：

| Rs=Cs | Physical spare lines/group | Spare-cell-equivalent/group | 相對值 |
|---:|---:|---:|---:|
| 2 | 16 | 69,632 | 1.0× |
| 3 | 24 | 104,448 | 1.5× |

相同 Rs/Cs 的 sharing policies 必須維持相同 physical spare-cell total。此 proxy
尚未包含 sharing routing、mux、switch 或 controller area。

### 5.3 BIRA latency

主要欄位：

- group SRAM experiment：`bira_cycles_per_fault`；
- canonical summary：`max_solution_after_bist_cycles`；
- canonical group detail：`latency_after_bist_cycles`、queue depth、queue wait；
- engine throughput proxy：`total_bira_completion_cycles`。

`latency_after_bist_cycles` 是 group-relative event model，不是多 groups 共用 engine
時的完整 device wall-clock latency，也不是 RTL sign-off timing。

### 5.4 BIRA cost

目前提供：

- CAM/SRAM storage bits；
- matrix bits；
- SRAM comparator bits；
- `bira_modeled_bits_per_engine`；
- SRAM normalized area proxy。

normalized area 的預設 SRAM-bit、comparator-bit coefficient 均為 1，只能用於本模型
內相對比較。小 depth SRAM macro 可能被 peripheral area 主導，因此不可把 modeled
bits 宣稱為 PDK/foundry macro area。

## 6. Code 與資料流

### 6.1 主要檔案

| 檔案 | 責任 |
|---|---|
| `experiments/moderate_repair_study.json` | axes、profiles、policies 與固定 geometry |
| `scripts/device/moderate_repair_study/run.py` | build、執行 point、policy selection、呼叫 collector/plotter |
| `scripts/device/moderate_repair_study/collect.py` | 合併 CSV、計算 spare cost、執行跨 backend invariant checks |
| `scripts/device/moderate_repair_study/plot.py` | 分開產生 group、SRAM、device PNG/PDF |
| `HierarchicalRECAM.cpp` | moderate/mixed CLI、canonical B0–B3、fault corpus 輸出 |
| `src/DynamicFaultGenerator.cpp` | 共用 deterministic fault generator 與 2:4:5:9 weights |
| `tests/hierarchical_fault_model_test.py` | corpus determinism、reload 與 B0/B2、B1/B3 regression |

### 6.2 Runner 流程

```text
moderate_repair_study.json
    |
    v
build three simulators (unless --no-build)
    |
    v
Group phase
    DynamicSpareSharing
    same seed/run index -> same generated fault maps across policies
    |
    v
rank sharing policies per Rs/Cs
    mean repair rate descending
    -> mean borrow count ascending as tie-breaker
    |
    +--------------------+
    |                    |
    v                    v
SRAM phase          Device phase
Serial/Chunk/Wide   HierarchicalRECAM --canonical-four
scan + shadow       B0 CAM/no sharing
                    B1 CAM/sharing
                    B2 SRAM/no sharing
                    B3 SRAM/sharing
                         |
                         v
                 resolved fault_corpus.txt
    |                    |
    +----------+---------+
               v
collect_moderate_repair_results.py
    validate + merge + derive spare cost
               |
               v
plot_moderate_repair_study.py
    group plots / SRAM plots / device plots
```

每一個 point 在開始 simulator 前建立：

- `point.json`：該點的 axes 與 scope；
- `command.txt`：可直接重現的完整 shell command；
- `run.log`：stdout/stderr；
- simulator 原始 CSV/JSON/remap outputs。

Runner 拒絕已存在的 output directory，避免新舊資料混合。若要重跑 profile，應使用
新的 `--output-dir`，不要刪除或覆寫正式結果。

### 6.3 Hierarchical moderate input

本次加入：

```text
--fault-model uniform|moderate|strong|hotspot
--spatial uniform|mixed|clustered
--write-fault-corpus
```

`HierarchicalRECAM` 直接重用 `DynamicFaultGenerator`，不在 Python 複製亂數算法。
`--canonical-four` 先生成一次 inputs，再交給四個獨立 scheduler／ledger／global pool，
因此 B0–B3 使用完全相同的 faults。

`fault_corpus.txt` 是七欄 physical-cell format：

```text
HBMID ChannelID BankID SubarrayGroupID SubarrayID Row CellCol
```

它可透過 `--simplified-fault-file` 原樣載回。Header 保存 model、spatial 與 seed。

### 6.4 Collector 驗證

Collector 在產生 `combined/` 前強制檢查：

1. group CSV 是 `moderate_imbalance`／`mixed`；
2. 每份 device corpus 的 group 數與每 group fault count 正確；
3. corpus header 是 moderate/mixed；
4. B0 與 B2 functional outcomes 完全相同；
5. B1 與 B3 functional outcomes 完全相同；
6. SRAM group result 與 CAM baseline signature 相同。

任一條件失敗時，collector 結束且不宣稱資料驗證通過。

## 7. 操作方法

### 7.1 完整端到端執行

```bash
# 建置 + 小型流程驗證
python3 scripts/device/moderate_repair_study/run.py --profile smoke

# 全 policy screen
python3 scripts/device/moderate_repair_study/run.py --profile screen

# 三 seeds confirm，只保留 no-sharing 與 screen 前兩名
python3 scripts/device/moderate_repair_study/run.py --profile confirm \
  --selection-from \
  reports/studies/moderate_repair/screen/selected_policies.json
```

若 binary 已存在，可加 `--no-build`。目前三個標準 output directory 已存在；再次
執行請指定新目錄，例如：

```bash
python3 scripts/device/moderate_repair_study/run.py --profile confirm \
  --selection-from \
  reports/studies/moderate_repair/screen/selected_policies.json \
  --output-dir reports/studies/moderate_repair/confirm_rerun
```

### 7.2 只重新彙整

```bash
python3 scripts/device/moderate_repair_study/collect.py \
  reports/studies/moderate_repair/confirm
```

### 7.3 只重新繪圖

```bash
python3 scripts/device/moderate_repair_study/plot.py \
  reports/studies/moderate_repair/confirm/combined \
  --output-dir reports/studies/moderate_repair/confirm/plots
```

### 7.4 驗證

```bash
make test_dynamic_spare_sharing_policy
make test_sram_recam_dynamic_equivalence
make test_hierarchical_recam
make test_hierarchical_fault_models
make test_canonical_experiment
make test_canonical_sweep
```

## 8. Output schema 與目錄

```text
reports/studies/moderate_repair/<profile>/
├── run_config.json
├── selected_policies.json
├── group/<point>/
│   ├── point.json
│   ├── command.txt
│   ├── run.log
│   └── summary.csv
├── sram/<point>/
│   ├── point.json
│   ├── command.txt
│   ├── run.log
│   ├── sram_recam_metrics.csv
│   └── hardware_cost_*.json
├── device/<point>/
│   ├── point.json
│   ├── command.txt
│   ├── run.log
│   ├── fault_corpus.txt
│   ├── canonical_four_summary.csv
│   ├── canonical_four_groups.csv
│   └── B*/RemapTable.txt
├── combined/
│   ├── group_summary.csv
│   ├── sram_summary.csv
│   ├── device_summary.csv
│   ├── device_groups.csv
│   └── validation.json
└── plots/
    ├── *.png
    ├── *.pdf
    └── plot_manifest.json
```

後續分析應優先讀 `combined/*.csv`；需要追查單點時再讀 point 原始輸出與 command。

## 9. Confirm 結果摘要

Confirm validation：

```text
group rows        = 72
SRAM rows         = 30
device rows       = 96
device-group rows = 6144
status            = passed
```

### 9.1 Group-level mean repair rate

| Rs=Cs | Faults | No Sharing | Pairwise/Edge m=1 | Global m=1 |
|---:|---:|---:|---:|---:|
| 2 | 16 | 0.8553 | **0.9773** | 0.9573 |
| 2 | 24 | 0.2740 | 0.3733 | **0.4707** |
| 2 | 32 | 0.0153 | 0.0227 | **0.0407** |
| 2 | 48 | 0 | 0 | 0 |
| 3 | 16 | 0.9993 | 1.0000 | 1.0000 |
| 3 | 24 | 0.7533 | **0.8940** | 0.8300 |
| 3 | 32 | 0.5793 | 0.6253 | **0.6313** |
| 3 | 48 | 0.0087 | 0.0093 | **0.0280** |

### 9.2 Device-level mean repairable-group fraction

Device sharing configuration：Rs=Cs=2 使用 Global m=1；Rs=Cs=3 使用
Pairwise/Edge m=1。

| Rs=Cs | Faults | No Sharing | Sharing |
|---:|---:|---:|---:|
| 2 | 16 | 0.2292 | 0.2396 |
| 2 | 24 | **0.0208** | 0.0052 |
| 2 | 32 | 0.0052 | 0 |
| 2 | 48 | 0 | 0 |
| 3 | 16 | 1.0000 | 1.0000 |
| 3 | 24 | 0.1562 | **0.6094** |
| 3 | 32 | 0.0260 | **0.0469** |
| 3 | 48 | 0.0052 | 0.0052 |

CAM 與 SRAM repair rates 在每個 point 完全相同；backend 差異只出現在 latency 與
hardware metrics。

### 9.3 SRAM search mean BIRA cycles/fault

| Rs=Cs | Serial | Chunk2 scan | Chunk4 scan | Wide scan | Chunk2 shadow |
|---:|---:|---:|---:|---:|---:|
| 2 | 5.6825 | 3.7783 | 2.8170 | **2.4398** | 4.2548 |
| 3 | 10.4529 | 5.9770 | 4.0901 | **2.5949** | 5.3858 |

Wide latency最低但 comparator/area proxy 最高。Chunk2 是較保守的中間點。Shadow
matrix 在 Rs=Cs=2 比 scan 慢，在 Rs=Cs=3 才有改善，因此不可宣稱 shadow 對所有
configuration 都更快。

## 10. 現有限制與後續工作

1. Confirm device point 為 64 groups/seed，不是完整 2048-group device yield。
2. 只有三個 seeds；可描述 seed range與平均，不應宣稱完整製程統計信賴區間。
3. Fault=48 多數 configuration 已接近 floor；增加 patterns 的資訊效益有限。
4. Global online capacity 使用 paper `Rs+Cs` baseline，結果對 capacity 很敏感。
5. Sharing routing/control area 尚未建模。
6. SRAM/CAM macro、decoder、sense amp、precharge、routing 與 energy 尚未做 PDK 評估。
7. BIRA latency 是 simulator event/work model，不是 RTL 或 silicon timing。
8. 若研究 enlarged online capacity，SRAM macro packing policy 必須先明確定義，不能
   直接用 enlarged CAM bit accounting 取代。

建議後續優先順序：

1. 固定 Rs=Cs=3、Pairwise/Edge m=1，增加 device groups／seeds；
2. 對 paper capacity 周圍做少量 global CAM sensitivity；
3. 加入 sharing routing/control cost model；
4. 用 OpenRAM／RTL synthesis 取代 normalized area proxy；
5. 若要比較其他 fault models，建立新的 manifest/output root，不要混入本次 moderate
   combined CSV。
