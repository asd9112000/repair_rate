# Scripts 使用說明

`scripts/` 收錄本專案的 C++ system-level simulation 工具，以及彼此隔離的 RTL
automation 入口。建議從專案根目錄
`/home/asd9112000/repair_rate` 執行以下指令，避免相對路徑指向錯誤。

工具依硬體 resource scope 分類；不要跨 `legacy`、`group`、`device` 的目錄合併
repair-rate 結果。輸出 scope 定義與新 run 的目錄規則以
[`docs/REPORTS.md`](../docs/REPORTS.md) 為準。

```text
scripts/
├── legacy/                 # C++ legacy paper / fixed-sharing experiments
├── group/                  # C++ single 4-SA group experiments
├── device/                 # C++ hierarchical device-level experiments
├── lint/                   # RTL lint automation
├── simulation/             # HDL/RTL simulation automation only
└── synthesis/              # RTL synthesis and constraints
```

C++ simulation output 一律寫入 `reports/`；RTL synthesis 與 cycle-accurate RTL
simulation output 一律寫入 `results/`。`scripts/simulation/` 的名稱不代表既有 C++
simulator runner，既有 runner 仍按 `legacy`、`group`、`device` 分類。

## 環境需求

C++ simulator 由 Makefile 編譯；Python 圖表套件可透過下列指令安裝：

```bash
python3 -m pip install -r requirements-plot.txt
```

## Script 一覽

| Script | 功能 | 主要輸入 | 主要輸出 |
| --- | --- | --- | --- |
| `group/dynamic_spare_sharing/sweep.sh` + `plot_sweep.py` | Dynamic Spare Sharing fault-count × spare-line sweep 與圖表 | CLI、`summary.csv` | `reports/group/dynamic_spare_sharing/<run-id>/` |
| `group/dynamic_spare_sharing/plot_figure1_repair_rate.py` | 配對 2×2 sweeps 的 Figure 1 repair-rate 圖與 95% Wilson CI | 各 seed 的 `summary.csv` | PNG、PDF、`figure1_plot_data.csv` |
| `group/dynamic_spare_sharing/generate_table_gallery.sh` + `plot_fault_model_vs_policy.py` | 預設 sweep 與 Fault Model × policy gallery | script 設定、CSV | `tables/`、`curves/`、`table_data/` |
| `group/sram_recam/sweep.sh` | 4-SA SRAM-RECAM policy comparison | CLI 參數 | `reports/group/sram_recam/` |
| `device/canonical_four/sweep.py` + `plot.py` | Device B0/B1/B2/B3 sweep 與圖表 | `HierarchicalRECAM`、axes | `canonical_four_sweep.csv`、`runs/`、PNG/PDF |
| `device/moderate_repair_study/run.py` + `collect.py` + `plot.py` | Scope-separated moderate study | JSON manifest、study root | 合併 CSV、validation、PNG/PDF |
| `legacy/sharedline/sweep.sh` + `plot_repair_rates.py` | SharedLine repair-rate sweep 與圖表 | 固定迴圈、`repairRates.csv` | `reports/legacy/sharedline/` |
| `legacy/sharedline_sram/sweep.sh` | SharedLine SRAM sweep；共用 SharedLine plotter | CLI 參數 | `reports/legacy/sharedline_sram/` |
| `legacy/redundant_rate/sweep.sh` + `plot.py` | RedundantRate sweep 與趨勢圖 | 固定迴圈、CSV | `reports/RedundantRate/` |

`__pycache__/` 是 Python 自動產生的 bytecode cache，不是需要手動執行或維護的
script。

## 使用方法

### 1. Dynamic Spare Sharing 完整分析

這是目前建議使用的可重現 sweep 流程。它會自動建置 simulator、執行
simulation、保留執行設定、產生 CSV，最後呼叫繪圖程式。

```bash
make sim_DynamicSpareSharing \
  ARGS="--fault-model moderate --runs 1000 --run-id moderate_demo"
```

1×4 row-only sweep 使用同一 wrapper；它會改用 No Sharing、Pair、Neighbor 與
Global Row Pool 的代表性 policy，並在 manifest 與 CSV 記錄 layout：

```bash
make sim_DynamicSpareSharing \
  ARGS="--layout 1x4 --fault-model moderate --runs 1000 \
        --run-id layout_1x4_moderate"
```

若要直接把七欄 simplified fault pattern 接到 Dynamic simulator，並輸出可供
下游系統使用的 remap table，可執行：

```bash
make dynamic_sharing_b
./build/bin/DynamicSpareSharing 2 2 \
  --simplified-fault-file fault_generator/faults_simplified.faults \
  --topology edge --shared-lines 1 --local-first \
  --write-remap-tables \
  --output-dir reports/group/dynamic_spare_sharing/integration_run
```

除原本三份 CSV 外，輸出目錄會增加 `RemapTable.txt` 與
`RemapTable_simplified.txt`。未指定 fault file 時仍使用整合式 deterministic
fault generator；remap table 目前只允許單一 policy run，不允許 sweep。

或直接執行：

```bash
scripts/group/dynamic_spare_sharing/sweep.sh \
  --fault-min 16 \
  --fault-max 32 \
  --fault-step 4 \
  --spare-min 2 \
  --spare-max 6 \
  --fault-model moderate \
  --runs 1000 \
  --seed 20260820 \
  --run-id moderate_demo
```

常用參數：

- `--fault-model`：`uniform`、`moderate`、`strong` 或 `hotspot`。
- `--storage`：`cam` 或 `sram`。
- `--layout`：`2x2`（預設）或第一階段 row-only 的 `1x4`。
- `--runs`：每組 configuration 的 simulation 次數。
- `--run-id`：輸出資料夾名稱。
- `--keep-run-details`：額外保留 `attempts.csv` 與 `runs.csv`。
- `--no-build`：使用現有的 `build/bin/DynamicSpareSharing`。

完整參數可用以下指令查看：

```bash
scripts/group/dynamic_spare_sharing/sweep.sh --help
```

注意：指定的 run directory 必須尚不存在，以避免新舊資料混合。

### 2. 單獨重畫 Dynamic Spare Sharing 圖表

若 simulation 已完成，可以直接使用既有的 `summary.csv` 重新繪圖：

```bash
python3 scripts/group/dynamic_spare_sharing/plot_sweep.py \
  reports/group/dynamic_spare_sharing/moderate_demo/raw/summary.csv \
  --output-dir reports/group/dynamic_spare_sharing/moderate_demo/plots \
  --plot-data-dir reports/group/dynamic_spare_sharing/moderate_demo/plot_data \
  --fault-model moderate_imbalance \
  --storage-mode cam
```

輸入資料必須是 `build/bin/DynamicSpareSharing --repair-rate-sweep` 產生的對稱
`Rs=Cs` sweep，而且一次只能選取一種 Fault Model 與 storage mode。

### 2.1 Figure 1 2×2 directional DSS 圖表

此 plotter 只讀取同一組 `Rs`／`Cs`、fault model 與 spatial model 的 CAM 2×2
結果，並要求 No Sharing、Directional `m=1`、Directional `m=2` 三個 policy 都存在，
避免把不完整的 policy set 畫成比較圖。

```bash
python3 scripts/group/dynamic_spare_sharing/plot_figure1_repair_rate.py \
  reports/group/dynamic_spare_sharing/figure1_input \
  --output-dir reports/group/dynamic_spare_sharing/figure1/plots
```

### 3. 產生 Fault Model vs Policy Gallery

此工具會顯示所有不重複的 policy：No Sharing、Directional m=1/m=2、
Pairwise m=1/m=2，以及 Global Pool m=1/m=2，並依每組
`(Rs, Cs, fault_count)` 產生四種 Fault Model 的 repair-rate 表格。各 topology
的 m=0 與 No Sharing 重複，因此不另列；Global Pool m=2 目前標記為
unverified diagnostic，不應直接視為已驗證的正式比較結果。

讀取包含多個 `summary.csv` 的目錄：

```bash
python3 scripts/group/dynamic_spare_sharing/plot_fault_model_vs_policy.py \
  reports/group/dynamic_spare_sharing/fault_model_vs_policy_demo/input \
  --output-dir reports/group/dynamic_spare_sharing/fault_model_vs_policy_demo/gallery
```

也可以指定單一 CSV：

```bash
python3 scripts/group/dynamic_spare_sharing/plot_fault_model_vs_policy.py \
  reports/group/dynamic_spare_sharing/example/raw/summary.csv \
  --output-dir reports/group/dynamic_spare_sharing/example/gallery
```

SRAM 資料需加上 `--storage-mode sram`；未指定時預設為 `cam`。

輸出結構：

```text
gallery/
├── selected_results_long.csv
├── table_data/
├── tables/
└── curves/
```

輸入若包含重複的 `(Rs, Cs, fault_count, fault_model, policy)`，script 會停止，
避免不同 seed 的結果被不明確地覆蓋。

若要直接執行專案預設的完整示範流程，可使用：

```bash
scripts/group/dynamic_spare_sharing/generate_table_gallery.sh
```

此 wrapper 會先執行 `make dynamic_sharing_b`，再以
`build/bin/DynamicSpareSharing` 模擬 `Rs=Cs=2,3,4` 與 fault count
`16,20,24,28,32`（每組 `250` runs），最後產生 gallery。原始 CSV 與圖表
都位於 `reports/group/dynamic_spare_sharing/table_gallery/`。可以透過環境變數調整：

```bash
OUTPUT_ROOT=reports/group/dynamic_spare_sharing/table_gallery_run2 \
RS_VALUES="2 3" \
FAULT_COUNTS="16 20 24" \
RUNS=1000 \
SEED=20260820 \
SPATIAL_MODEL=mixed \
  scripts/group/dynamic_spare_sharing/generate_table_gallery.sh
```

另可用 `GALLERY_OUTPUT_DIR` 將最終 gallery 放到不同目錄。

### 4. Canonical B0/B1/B2/B3 sweep

`HierarchicalRECAM --canonical-four` 的 wrapper 會對每個 parameter point 執行一次，
在單一 process 內配對 B0/B1/B2/B3；因此四組確實使用同一組 generated faults，
而不是四次獨立抽樣。

```bash
make hierarchical_recam_b
python3 scripts/device/canonical_four/sweep.py \
  --simulator build/bin/HierarchicalRECAM \
  --output-dir reports/device/canonical_four/sweep_demo \
  --rs 2 --cs 2 --fault-counts 4,8,12 \
  --word-bits 16 --seeds 20260820 \
  --sram-policies chunked:2 \
  --groups 32 --memory-rows 512 --memory-columns 8192 \
  --topology edge --shared-rows 1 --shared-columns 0 --max-borrows 1

python3 scripts/device/canonical_four/plot.py \
  reports/device/canonical_four/sweep_demo/canonical_four_sweep.csv \
  --output-dir reports/device/canonical_four/sweep_demo/plots
```

每個 `runs/<point>/` 都有原始 B0–B3 outputs 與 `command.txt`；根目錄的
`canonical_four_sweep.csv` 是後續分析唯一應讀取的合併表。輸入可掃描 `Rs`、`Cs`、
fault count、word width、seed 與 SRAM search policy；一次圖表請固定除 seed 和
fault count 外的 axes，避免把不同硬體 configuration 的結果平均在一起。

### 4.1 Moderate repair study

此 workflow 同時包含三種不同用途的階段：

| 階段 | Simulator | 用途 |
|---|---|---|
| Group policy | `DynamicSpareSharing` | 在獨立 4-SA samples 篩選 sharing policy |
| SRAM policy | `DynamicSpareSharing_SRAM_RECAM` | 比較 Serial／Chunked／Wide 與 scan／shadow |
| Device canonical | `HierarchicalRECAM --canonical-four` | 比較 B0–B3 與 finite device-wide CAM |

三種階段會由同一 runner 執行，但 collector 與 plotter 保持 scope 分離。

```bash
python3 scripts/device/moderate_repair_study/run.py --profile smoke
python3 scripts/device/moderate_repair_study/run.py --profile screen

python3 scripts/device/moderate_repair_study/run.py --profile confirm \
  --selection-from \
  reports/studies/moderate_repair/screen/selected_policies.json
```

Profiles：

| Profile | Group runs/point | Device groups/point | Seeds | Policy set |
|---|---:|---:|---|---|
| `smoke` | 50 | 2 | 1 | 全部四種 |
| `screen` | 150 | 32 | 1 | 全部四種 |
| `confirm` | 500 | 64 | 3 | No Sharing + screen 前兩名 |

完整矩陣在 `experiments/moderate_repair_study.json`。Runner 預設建置三個 simulator；
已有 binary 時可加 `--no-build`。標準 smoke／screen／confirm 目錄目前均已存在，
runner 會拒絕覆寫。重跑時必須使用新目錄：

```bash
python3 scripts/device/moderate_repair_study/run.py --profile confirm \
  --selection-from \
  reports/studies/moderate_repair/screen/selected_policies.json \
  --output-dir reports/studies/moderate_repair/confirm_rerun
```

每個 point 保存 `point.json`、`command.txt`、`run.log` 與 simulator 原始輸出；
根目錄保存 `run_config.json` 與 `selected_policies.json`。若 simulation 已完成，只需
重做彙整與繪圖：

```bash
python3 scripts/device/moderate_repair_study/collect.py \
  reports/studies/moderate_repair/screen
python3 scripts/device/moderate_repair_study/plot.py \
  reports/studies/moderate_repair/screen/combined \
  --output-dir reports/studies/moderate_repair/screen/plots
```

`confirm` profile 需以 `--selection-from` 指向 screen 的
`selected_policies.json`，因此不會重新執行已淘汰的 sharing policies。

Collector 會檢查 moderate/mixed metadata、fault corpus counts、B0=B2、B1=B3，
以及 SRAM group signature 等於 CAM baseline。驗證結果寫入
`combined/validation.json`。Plotter 產生八組 PNG/PDF，涵蓋 group repair rate、
spare cost、SRAM latency/cost、device repair rate、global reuse pressure 與 BIRA
modeled bits。

完整研究交接、結果解讀與限制見
[`docs/MODERATE_REPAIR_STUDY_HANDOFF.md`](../docs/MODERATE_REPAIR_STUDY_HANDOFF.md)。

### 5. 傳統 SharedLine 分析

```bash
make sl_b
make analyze_spareline
```

`analyze_SpareLine.sh` 使用固定範圍：

- Fault number：8 到 16。
- Spare line：2 到 6。
- 每個 fault number 產生 100 組 patterns。

它會重建 `reports/legacy/sharedline/repairRates.csv`，因此執行前應先保存需要保留的
舊結果。

若已有 CSV，只需要重新繪圖：

```bash
python3 scripts/legacy/sharedline/plot_repair_rates.py \
  reports/legacy/sharedline/repairRates.csv \
  --output-dir reports/legacy/sharedline/plots \
  --architecture-label SharedLine
```

### 6. SharedLine SRAM 分析

固定 buffer 模式：

```bash
make analyze_spareline_sram \
  ARGS="--buffer 2 --fault-min 8 --fault-max 16"
```

Paper CAM reuse 模式：

```bash
make analyze_spareline_sram ARGS="--paper-cam-reuse"
```

完整參數：

```bash
scripts/legacy/sharedline_sram/sweep.sh --help
```

script 預設會透過 Makefile 建立 `build/bin/SharedLine_SRAM` 與
`build/bin/fault_generator`，並在 simulation 後呼叫目前的 SharedLine plotter。
使用 `--no-build` 時，這兩個 executable 必須已存在。

### 7. RedundantRate 分析

```bash
make rdr_b
make analyze_redundantrate
```

`analyze_RedundantRate.sh` 會執行以下固定 sweep：

- Fault number：4 到 16。
- Spare row/column：2 到 6。
- 每個 fault number 產生 250 組 patterns。

整理後的資料位於：

```text
reports/RedundantRate/RedundantRate.csv
```

若只需重新繪圖：

```bash
python3 scripts/legacy/redundant_rate/plot.py \
  reports/RedundantRate/RedundantRate.csv \
  --output-dir reports/RedundantRate/plots
```

## 建議流程

首次測試可以使用約 `100` runs；確認流程與圖表後，再提高到 `1,000` 或
`10,000` runs。正式比較不同 sharing policy 時，應固定 seed，並保留原始 CSV
與 run configuration，確保結果可重現。
