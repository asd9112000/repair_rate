# Scripts 使用說明

`scripts/` 收錄本專案的批次 simulation 與結果繪圖工具。建議從專案根目錄
`/home/asd9112000/repair_rate` 執行以下指令，避免相對路徑指向錯誤。

## 環境需求

C++ simulator 由 Makefile 編譯；Python 圖表套件可透過下列指令安裝：

```bash
python3 -m pip install -r requirements-plot.txt
```

## Script 一覽

| Script | 功能 | 主要輸入 | 主要輸出 |
| --- | --- | --- | --- |
| `sim_DynamicSpareSharing.sh` | 建置並執行 Dynamic Spare Sharing 的 fault-count × spare-line sweep，接著自動繪圖 | CLI 參數 | `reports/dynamic_spare_sharing/<run-id>/` |
| `plot_dynamic_repair_rate_sweep.py` | 將 Dynamic Spare Sharing 的 `summary.csv` 轉為比較圖、heatmap、趨勢圖及分析表 | `summary.csv` | `plots/`、`plot_data/` |
| `generate_DynamicSpareSharing_table_gallery.sh` | 建置 simulator，批次執行預設的 `Rs=Cs`／fault-count 組合，再呼叫 gallery 產圖 | script 內的 sweep 設定 | `reports/dynamic_spare_sharing/table_gallery/` |
| `plot_fault_model_vs_policy.py` | 從一個或多個 `summary.csv` 產生各 Fault Model × policy 的 repair-rate 表格與 fault-count 趨勢圖 | CSV 或包含 CSV 的目錄 | `tables/`、`curves/`、`table_data/` |
| `analyze_SpareLine.sh` | 執行傳統 `build/bin/SharedLine` 的固定 fault/spare sweep，整理 repair rate 並繪圖 | 固定迴圈參數 | `reports/SharedLine/` |
| `plot_repair_rates_for_sharedline_cpp.py` | 繪製 SharedLine 與 RECAM 的 repair-rate 比較、改善量及 heatmap | `repairRates.csv` | PNG、PDF |
| `analyze_SpareLine_SRAM.sh` | 執行 `build/bin/SharedLine_SRAM` 的 fault/spare sweep，可切換固定 buffer 或 paper CAM reuse | CLI 參數 | `reports/SharedLine_SRAM/` |
| `analyze_RedundantRate.sh` | 執行 `build/bin/RedundantRate` sweep，從報告整理平均額外線數 | 固定迴圈參數 | `reports/RedundantRate/RedundantRate.csv` |
| `plot_redundant_rate_for_redudant_rate_cpp.py` | 繪製 RedundantRate 的平均額外線數趨勢 | `RedundantRate.csv` | PNG、PDF |

`__pycache__/` 是 Python 自動產生的 bytecode cache，不是需要手動執行或維護的
script。

## 使用方法

### 1. Dynamic Spare Sharing 完整分析

這是目前建議使用的可重現 sweep 流程。它會自動建置 simulator、執行
simulation、保留執行設定、產生 CSV，最後呼叫繪圖程式。

```bash
make analyze_dynamic_spare_sharing \
  ARGS="--fault-model moderate --runs 1000 --run-id moderate_demo"
```

若要直接把七欄 simplified fault pattern 接到 Dynamic simulator，並輸出可供
下游系統使用的 remap table，可執行：

```bash
make dynamic_sharing_b
./build/bin/DynamicSpareSharing 2 2 \
  --simplified-fault-file fault_generator/faults_simplified.faults \
  --topology edge --shared-lines 1 --local-first \
  --write-remap-tables \
  --output-dir reports/dynamic_spare_sharing/integration_run
```

除原本三份 CSV 外，輸出目錄會增加 `RemapTable.txt` 與
`RemapTable_simplified.txt`。未指定 fault file 時仍使用整合式 deterministic
fault generator；remap table 目前只允許單一 policy run，不允許 sweep。

或直接執行：

```bash
scripts/sim_DynamicSpareSharing.sh \
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
- `--runs`：每組 configuration 的 simulation 次數。
- `--run-id`：輸出資料夾名稱。
- `--keep-run-details`：額外保留 `attempts.csv` 與 `runs.csv`。
- `--no-build`：使用現有的 `build/bin/DynamicSpareSharing`。

完整參數可用以下指令查看：

```bash
scripts/sim_DynamicSpareSharing.sh --help
```

注意：指定的 run directory 必須尚不存在，以避免新舊資料混合。

### 2. 單獨重畫 Dynamic Spare Sharing 圖表

若 simulation 已完成，可以直接使用既有的 `summary.csv` 重新繪圖：

```bash
python3 scripts/plot_dynamic_repair_rate_sweep.py \
  reports/dynamic_spare_sharing/moderate_demo/raw/summary.csv \
  --output-dir reports/dynamic_spare_sharing/moderate_demo/plots \
  --plot-data-dir reports/dynamic_spare_sharing/moderate_demo/plot_data \
  --fault-model moderate_imbalance \
  --storage-mode cam
```

輸入資料必須是 `build/bin/DynamicSpareSharing --repair-rate-sweep` 產生的對稱
`Rs=Cs` sweep，而且一次只能選取一種 Fault Model 與 storage mode。

### 3. 產生 Fault Model vs Policy Gallery

此工具會顯示所有不重複的 policy：No Sharing、Directional m=1/m=2、
Pairwise m=1/m=2，以及 Global Pool m=1/m=2，並依每組
`(Rs, Cs, fault_count)` 產生四種 Fault Model 的 repair-rate 表格。各 topology
的 m=0 與 No Sharing 重複，因此不另列；Global Pool m=2 目前標記為
unverified diagnostic，不應直接視為已驗證的正式比較結果。

讀取包含多個 `summary.csv` 的目錄：

```bash
python3 scripts/plot_fault_model_vs_policy.py \
  reports/fault_model_vs_policy_demo/input \
  --output-dir reports/fault_model_vs_policy_demo/gallery
```

也可以指定單一 CSV：

```bash
python3 scripts/plot_fault_model_vs_policy.py \
  reports/example/raw/summary.csv \
  --output-dir reports/example/gallery
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
scripts/generate_DynamicSpareSharing_table_gallery.sh
```

此 wrapper 會先執行 `make dynamic_sharing_b`，再以
`build/bin/DynamicSpareSharing` 模擬 `Rs=Cs=2,3,4` 與 fault count
`16,20,24,28,32`（每組 `250` runs），最後產生 gallery。原始 CSV 與圖表
都位於 `reports/dynamic_spare_sharing/table_gallery/`。可以透過環境變數調整：

```bash
OUTPUT_ROOT=reports/dynamic_spare_sharing/table_gallery_run2 \
RS_VALUES="2 3" \
FAULT_COUNTS="16 20 24" \
RUNS=1000 \
SEED=20260820 \
SPATIAL_MODEL=mixed \
  scripts/generate_DynamicSpareSharing_table_gallery.sh
```

另可用 `GALLERY_OUTPUT_DIR` 將最終 gallery 放到不同目錄。

### 4. 傳統 SharedLine 分析

```bash
make sl_b
make analyze_spareline
```

`analyze_SpareLine.sh` 使用固定範圍：

- Fault number：8 到 16。
- Spare line：2 到 6。
- 每個 fault number 產生 100 組 patterns。

它會重建 `reports/SharedLine/repairRates.csv`，因此執行前應先保存需要保留的
舊結果。

若已有 CSV，只需要重新繪圖：

```bash
python3 scripts/plot_repair_rates_for_sharedline_cpp.py \
  reports/SharedLine/repairRates.csv \
  --output-dir reports/SharedLine/plots \
  --architecture-label SharedLine
```

### 5. SharedLine SRAM 分析

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
scripts/analyze_SpareLine_SRAM.sh --help
```

script 預設會透過 Makefile 建立 `build/bin/SharedLine_SRAM` 與
`build/bin/fault_generator`，並在 simulation 後呼叫目前的 SharedLine plotter。
使用 `--no-build` 時，這兩個 executable 必須已存在。

### 6. RedundantRate 分析

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
python3 scripts/plot_redundant_rate_for_redudant_rate_cpp.py \
  reports/RedundantRate/RedundantRate.csv \
  --output-dir reports/RedundantRate/plots
```

## 建議流程

首次測試可以使用約 `100` runs；確認流程與圖表後，再提高到 `1,000` 或
`10,000` runs。正式比較不同 sharing policy 時，應固定 seed，並保留原始 CSV
與 run configuration，確保結果可重現。
