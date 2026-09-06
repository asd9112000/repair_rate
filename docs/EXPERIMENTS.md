# 實驗與結果整理指南

> 文件狀態：Current
> 適用範圍：cross-cutting
> 建立時間：Unknown
> 最後修改時間：2026-09-02T00:00:00+08:00
> 本文件權威主題：實驗方法、可重現性、sweep、plotting 與結果比較規則

本文件整合 dynamic spare sharing、SRAM-RECAM、sweep 與 plotting 的操作方式。
Device-level CAM reuse 實驗請使用 `HierarchicalRECAM`；四 subarray group-level
sharing 實驗則使用 `DynamicSpareSharing`。兩者的 CAM ownership 不同，結果不可
直接混用。

## 1. 選擇正確的 simulator

| 問題 | 建議程式 | Resource scope |
|---|---|---|
| 原始 RECAM／固定 sharing baseline | `SharedLine` | legacy 4-PE pattern |
| 比較 group 內 spare-sharing topology | `DynamicSpareSharing` | 單一 4-SA group |
| 比較 Serial／Chunked／Wide SRAM search | `DynamicSpareSharing_SRAM_RECAM` | 單一 4-SA group |
| BIRA engine reuse、跨 groups 的 finite online CAM | `HierarchicalRECAM` | device-wide logic die |

`DynamicSpareSharing` 不會跨 group 累積 online CAM occupancy；若研究問題是「整顆
記憶體共用的 CAM 是否溢出」，必須改用 `HierarchicalRECAM`。

## 2. 可重現性規則

每組正式實驗至少記錄：

- simulator 與 architecture version；
- 完整 CLI、Git revision 與 random seed；
- `Rs/Cs`、fault model、runs、memory geometry；
- sharing topology 與實體 shared-line 數量；
- CAM/SRAM capacity、granularity 與 latency parameters；
- 原始 CSV，不只保存圖片。

同一比較中的各 policy 應使用相同 fault maps。不要重用已有結果的輸出目錄；
hierarchical runner 會 truncate 同名 CSV，dynamic sweep wrapper 則會拒絕既有 run
directory。

建議目錄：

```text
reports/<simulator>/<run-id>/
├── run_config.txt
├── raw/
│   └── summary.csv
├── plot_data/
├── plots/
└── logs/
```

## 3. Dynamic spare sharing

### 3.1 單一 configuration

```bash
make dynamic_sharing_b
./build/bin/DynamicSpareSharing 2 2 \
  --topology directional \
  --shared-lines 1 \
  --local-first \
  --fault-count 20 \
  --runs 10000 \
  --seed 20260820 \
  --hybrid-cam-entry-width-bits 64 \
  --output-dir reports/dynamic_spare_sharing/manual_run
```

`fault-count` 是 A/B/C/D 一個 group 的總 faults。預設採 paper CAM reuse，buffer
capacity 為 `Rs+Cs`；只有要重現固定容量 legacy baseline 時才使用 `--buffer N`。
`--buffer` 與 `--paper-cam-reuse` 互斥。

常用 topology：

- `none`：不共享；
- `directional`：A→C、D→B 借 row，B→A、C→D 借 column；
- `edge`：只沿 2×2 group 邊相鄰 sharing，不允許 diagonal；
- `global`：把相同 physical total 的部分 spares 移入 group pool，不是新增資源。

預設 `--layout 2x2`，因此既有未指定 layout 的命令維持原本語意。1×4 linear
row-only 實驗使用：

```bash
./build/bin/DynamicSpareSharing 2 2 \
  --layout 1x4 \
  --topology neighbor \
  --shared-rows 1 --shared-columns 0 \
  --local-first --max-borrows 1 \
  --fault-count 20 --runs 100 --seed 20260820 \
  --output-dir reports/dynamic_spare_sharing/layout_1x4_neighbor_row_m1
```

1×4 topology 定義如下：

- `none`：每個 SA 只使用自己的 `Rs` rows；
- `pair`：只允許 A↔B、C↔D；
- `neighbor`：只允許 A↔B、B↔C、C↔D，不進行 multi-hop；
- `global`：每個 SA 將 `shared_rows=m` 從 local ownership 移入容量 `4m` 的有限
  group row pool。

1×4 第一階段不支援 column sharing；`--shared-columns` 必須為 0，且會拒絕會同時
設定 row/column 的 `--shared-lines N`（N>0）。`directional` 與 2×2 `edge` 在 1×4
也會被明確拒絕。所有 CSV 都包含 `layout` 欄位；`summary.csv` 另提供 local repair、
需要 borrowing 的成功 group，以及 borrowed-row total／average／maximum。

四個 1×4 policy 的直接比較必須固定 seed、fault model、fault count 與 memory
geometry。相同 generator 設定不因 layout/topology 改變 fault corpus；正式分析仍
建議保留輸入 fault file 或完整 CLI/run config。

Policy modifier 包含 `--local-first`、`--minimum-row-reserve N`、
`--minimum-column-reserve N`、`--single-dimension` 與 `--max-borrows N`。完整選項
以 `./build/bin/DynamicSpareSharing --help` 為準。

輸出：

- `attempts.csv`：每次 RECAM attempt 與 valid candidates；
- `runs.csv`：每個 run 的選擇、borrowing、utilization 與 cycle metrics；
- `summary.csv`：repair rate、平均／peak／P95 與 cost proxies。

大量 sweep 建議加 `--summary-only`，避免 `attempts.csv` 與 `runs.csv` 過大。

### 3.2 Solution Take Policy

四個 SA 的 RECAM analysis 完成後，可用獨立於 layout/topology 的選解參數：

```text
--solution-take legacy   保留既有 group selector，亦為預設值
--solution-take early    A→B→C→D 依序選解，已選解不回溯
--solution-take group    保留四個 SA 的壓縮狀態後枚舉完整有效解組合
```

`early` 與 `group` 都只從既有 RECAM `validSolList` 選取，不重新判斷 solution
validity，也不改變 candidate numbering。`early` 依序最小化 borrowed lines、總
spare lines、solution ID；`group` 對所有可行四元組最小化總 borrowed lines、總
spare lines，再選 lexicographically smallest `(A,B,C,D)` solution IDs。

`runs.csv` 記錄 valid-solution bitmap、selected solution IDs、selector work、可行
組合數、EARLY remaining resources 與 compressed-state bits；`summary.csv` 記錄
greedy loss 與 selector 統計。selector work 是軟體候選檢查次數，不是硬體 cycle。

壓縮狀態的分析位元數為：

```text
K × row_address_bits + K × column_address_bits + 2K + C(K,Rs)
```

`2K` 保存 row/column matrix-address slot validity；BUFFMAP persistent CAM-reuse
mappings 另行保存與計數。

### 3.3 Fault model

支援 `uniform`、`moderate`、`strong`、`hotspot` 與 `user`，空間分布支援
`uniform`、`mixed`、`clustered`。自訂 A/B/C/D 數量範例：

```bash
./build/bin/DynamicSpareSharing 2 2 \
  --fault-model user --fault-counts 2,4,5,9 \
  --spatial mixed --runs 1000 --seed 20260820 \
  --topology edge --shared-lines 1 --local-first \
  --output-dir reports/dynamic_spare_sharing/user_2_4_5_9
```

### 3.4 Fault-count × spare-line sweep

建議使用 wrapper，讓 simulation、plot 與設定檔位於同一個 run directory：

```bash
make sim_DynamicSpareSharing \
  ARGS="--fault-min 16 --fault-max 64 --fault-step 4 \
        --spare-min 2 --spare-max 6 \
        --fault-model moderate --runs 1000 \
        --seed 20260820 --run-id moderate_demo"
```

或直接執行 `scripts/sim_DynamicSpareSharing.sh --help`。代表性 policy set 為 No
Sharing、Directional m=1/m=2、Pairwise m=1/m=2、Global Pool m=1。
Global Pool m=2 的 `local=0/global=8` 組態仍是 unverified diagnostic，不應放入
正式 publication comparison。

## 4. Remap-table integration

輸入七欄 simplified fault address：

```text
HBMID ChannelID BankID SubarrayGroupID SubarrayID Row Col
```

範例：

```bash
./build/bin/DynamicSpareSharing 2 2 \
  --simplified-fault-file fault_generator/faults_simplified.faults \
  --topology edge --shared-lines 1 --local-first \
  --write-remap-tables \
  --output-dir reports/dynamic_spare_sharing/integration_run
```

另產生 `RemapTable.txt` 與 `RemapTable_simplified.txt`。Remap output 只允許單一
policy configuration，不可與 `--sweep` 或 `--repair-rate-sweep` 同時使用。

## 5. SRAM-RECAM policy comparison

```bash
make dynamic_sram_recam_b
./build/bin/DynamicSpareSharing_SRAM_RECAM 2 2 \
  --topology directional --shared-lines 1 --local-first \
  --fault-count 20 --runs 100 --seed 20260820 \
  --policies serial,chunk2,chunk4,wide \
  --matrix-policy scan \
  --output-dir reports/dynamic_sram_recam/example
```

政策可使用 `serial`、`chunk2`、`chunk4`、`wide` 或 `chunked:N`。硬體 geometry、
search latency、runtime hit/miss 與 area proxy 的解讀見
[SRAM_RECAM_ARCHITECTURE_GUIDE.md](SRAM_RECAM_ARCHITECTURE_GUIDE.md)。

## 6. Hierarchical device experiments

基本執行方式與 CSV 欄位見 [HIERARCHICAL_RECAM.md](HIERARCHICAL_RECAM.md)。建議
分開進行下列 sweep：

1. **Global CAM capacity**：先使用 paper baseline `Rs+Cs`；若要研究放大的實體
   CAM，再明確掃描 `4,8,16,32,64,128,256` entries，並把結果標示為
   `EXPLICIT_OVERRIDE`。
2. **Modeled groups**：逐步增加 `--groups`，觀察 occupancy、dedup 與 overflow。
3. **Tier-1 sharing**：比較 `none` 與相同 physical total 的 topology，量測 Tier-2
   demand 是否下降。
4. **BIRA engines**：只用於 throughput／scheduler 實驗；不應讓 engine count 改變
   global online CAM capacity。
5. **Granularity**：比較 `word` 與 `cell`，並明確記錄 `--data-word-bits`。

每一筆 Hierarchical v2 結果應具有：

```text
architecture_version = WoW-v1.0
simulator_version = Hierarchical-RECAM-v2.1
cam_scope = GLOBAL_LOGIC_DIE
```

### 6.1 B0/B1/B2/B3 unified experiment

正式比較 CAM、sharing 與 SRAM 時，使用一個 `--canonical-four` process 完成四組，
讓四者吃到完全相同的 input fault set：

```bash
./build/bin/HierarchicalRECAM 2 2 \
  --canonical-four --groups 32 --fault-count 8 --seed 20260820 \
  --topology edge --shared-rows 1 --shared-columns 0 --max-borrows 1 \
  --sram-policy chunked:2 \
  --output-dir reports/canonical/baseline_r2c2
```

四組定義固定為 B0 CAM/no sharing、B1 CAM/sharing、B2 SRAM/no sharing、
B3 SRAM/sharing。比較表一律讀 `canonical_four_summary.csv`；每組的 detailed
BIRA FIFO timing 讀 `canonical_four_groups.csv`。不要將 legacy/group-level 的
`DynamicSpareSharing` 結果當成這四組 device-level comparison 的其中一列。

若使用 count-framed fault list，將 `--groups`／`--fault-count` 換成：

```bash
--fault-file fault_generator/faults.faults \
--memory-rows 1024 --memory-columns 1024 --data-word-bits 16
```

`RemapTable.txt` 仍是 physical cell-column format；每個 B0–B3 directory 分別
輸出一份，下游只能選取一份 architecture configuration 使用。

### 6.2 Canonical parameter sweep and plotting

先建置：

```bash
make hierarchical_recam_b
```

以一個 small smoke sweep 為例：

```bash
python3 scripts/run_canonical_four_sweep.py \
  --simulator build/bin/HierarchicalRECAM \
  --output-dir reports/canonical/sweep_demo \
  --rs 2 --cs 2 --fault-counts 4,8,12 \
  --word-bits 16 --seeds 20260820 \
  --sram-policies chunked:2 \
  --groups 32 --memory-rows 512 --memory-columns 8192 \
  --topology edge --shared-rows 1 --shared-columns 0 --max-borrows 1

python3 scripts/plot_canonical_four.py \
  reports/canonical/sweep_demo/canonical_four_sweep.csv \
  --output-dir reports/canonical/sweep_demo/plots
```

runner 會保留每個 point 的 command、四組原始 output，並合併為 long-format
`canonical_four_sweep.csv`。plotter 產生 repair rate、maximum after-BIST latency、
hardware bits 的 PNG/PDF，以及實際畫圖的 CSV copy。

目前 plotter 對同一 fault count 只應合併 **同一情境的不同 seed**。因此一次 plot
請固定 `Rs/Cs`、word width、SRAM policy、topology 與 borrowing policy；若要
比較不同架構點，請分開 output directory／圖表，避免將不同 hardware configuration
平均成一條沒有物理意義的曲線。

### 6.3 Moderate repair-rate／cost study

`experiments/moderate_repair_study.json` 固定研究矩陣：`Rs=Cs=2,3`、每 group
`16,24,32,48` faults、`moderate`／`mixed`，以及 No Sharing、Directional m=1、
Pairwise/Edge m=1、Global m=1。runner 先做 group-level policy screen，再只把每個
spare configuration 的最佳 sharing policy 帶入 device-level B0–B3，兩種 scope
各自輸出 CSV 與圖表，不相互合併。

```bash
# 50 runs/point、2 groups/device 的端到端檢查
python3 scripts/run_moderate_repair_study.py --profile smoke

# 150 runs/point、32 groups/device；group 主矩陣共 4,800 samples
python3 scripts/run_moderate_repair_study.py --profile screen

# 後續較完整統計只保留 no-sharing 與 screen 前兩名
python3 scripts/run_moderate_repair_study.py --profile confirm \
  --selection-from reports/moderate_repair_study/screen/selected_policies.json
```

每個 point 保存 `command.txt`、`point.json`、`run.log` 與原始 simulator outputs。
`combined/` 保存四份可直接分析的 CSV；`plots/` 同時保存 PNG/PDF。
spare cost 使用：

```text
spare-cell-equivalent/group = 4 × (Rs × memory_columns + Cs × memory_rows)
```

固定 Rs/Cs 時，sharing 不增加 physical spare-cell total；routing／switch／controller
尚未包含在此 proxy。collector 會拒絕 CAM/SRAM functional outcome 不相等、SRAM
group result 不符合 CAM baseline、或 device fault corpus 不是 moderate/mixed 的結果。

本次已完成的 smoke、screen、confirm 結果、code flow、selected policies、輸出目錄與
後續限制，集中記錄於
[handoff/MODERATE_REPAIR_STUDY_HANDOFF.md](handoff/MODERATE_REPAIR_STUDY_HANDOFF.md)。

## 7. Plotting

先安裝 Python plotting dependencies：

```bash
python3 -m pip install -r requirements-plot.txt
```

對 repair-rate sweep 重畫圖：

```bash
python3 scripts/plot_dynamic_repair_rate_sweep.py \
  reports/dynamic_spare_sharing/moderate_demo/raw/summary.csv \
  --output-dir reports/dynamic_spare_sharing/moderate_demo/plots \
  --plot-data-dir reports/dynamic_spare_sharing/moderate_demo/plot_data \
  --fault-model moderate_imbalance \
  --storage-mode cam
```

輸出同時保留 PNG、PDF 與產圖使用的 processed CSV。Repair gain 的單位是
percentage points，不是 relative percent。

## 8. 驗證

```bash
make test_hierarchical_recam
make test_hierarchical_fault_models
make test_dynamic_spare_sharing_policy
make test_dynamic_repair_rate_sweep
make test_dynamic_remap_output
make test_sram_recam_model
make test_sram_recam_dynamic_equivalence
make test_sram_fault_collection_fifo
make test_fault_address_bist
make test_recam_common_models
make test_canonical_experiment
make test_canonical_sweep
```

正式 sweep 前先做小型 smoke run，確認 seed、fault count、CSV version fields 與
output path；之後再提高 `runs` 或 `groups`。
