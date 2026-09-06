# 實驗編排與結果管理交接文件

> 文件狀態：Handoff
> 適用範圍：cross-cutting / experiment-workflow
> 建立時間：2026-09-07T06:11:50+08:00
> 最後修改時間：2026-09-07T06:11:50+08:00
> 本文件權威主題：不修改 C++ 核心時的實驗 runner、manifest 與 run artifact 管理交接
> 注意：本文件是時間點快照；架構邊界與 `reports/` 規則仍分別以 `ARCHITECTURE.md` 與 `REPORTS.md` 為準。

## 1. 目的與決策

本 repository 的 C++ simulator 應視為穩定的計算核心：其 CLI、repair semantics
與 CSV contract 不因個別投稿圖表而隨意修改。實驗間的差異，包括 parameter
sweep、policy、seed、profile、輸出位置、結果合併和繪圖，應由 Python runner 與
JSON manifest 管理。

此決策的目標是：

- 不為了每一張圖修改基本 C++；
- 讓每一個實驗點可單獨追溯、重跑與檢查；
- 讓 plot 僅消費已完成的 CSV，不隱含重新 simulation；
- 保留 group、device、legacy 結果的 resource-ownership 邊界；
- 在投稿時間有限時，以小型 smoke/screen/confirm profile 控制計算成本。

現有的可參考實作是
[`scripts/device/moderate_repair_study/run.py`](../../scripts/device/moderate_repair_study/run.py)。
它讀取 manifest、展開 parameter points、建置 binary、保存每點 command/log/metadata、
再呼叫 collector 與 plotter。它是 pattern，不表示所有新研究都必須納入 moderate
study 或混合其三種 scope。

## 2. 建議的責任分層

```text
C++ binaries
    固定 CLI、repair model、原始 CSV contract
        ↓
experiments/<study>.json
    研究問題、固定控制項、sweep axes、seeds、profiles、policy labels
        ↓
scripts/run_<study>.py
    build、展開 points、執行、command/log/metadata、失敗處理
        ↓
scope-first reports directory
    raw simulator CSV → derived analysis CSV → plots
```

各層責任如下：

| 層 | 應負責 | 不應負責 |
|---|---|---|
| C++ binary | 一筆設定的 simulation、原始 metrics/CSV | 決定論文選哪條 curve、跨 run 平均、檔案搬移 |
| JSON manifest | 可機讀的研究設定與固定比較條件 | 實作迴圈、以手工複製 command 取代 provenance |
| Python runner | manifest expansion、執行、artifact 保存、可恢復的 run orchestration | 改寫 repair semantics 或混合 scope 的數值比較 |
| collector/plotter | 檢查輸入、產生 derived CSV、tables、figures | 呼叫 simulator 或默默補齊缺失資料 |

若目前 C++ CLI 無法表達某個研究設定，先在 Python 層檢查能否由多個既有 CLI
invocation 組合完成。只有 simulation model 本身缺少必要能力時，才另外提出最小、
可驗證的 C++ 變更；不可把 runner convenience 當成修改 repair core 的理由。

## 3. Manifest 與 runner 合約

每個新研究應新增一份 `experiments/<study>.json`。至少要記錄：

```json
{
  "scope": "group",
  "simulator": "build/bin/DynamicSpareSharing",
  "fixed": {
    "rs": 2,
    "cs": 2,
    "fault_model": "moderate",
    "spatial_model": "mixed",
    "solution_take": "legacy"
  },
  "axes": {
    "fault_counts_per_group": [8, 12, 16, 20, 24, 28, 32],
    "policies": ["no_sharing", "directional_m1", "directional_m2"]
  },
  "profiles": {
    "smoke": {"seeds": [20260820], "runs_per_seed": 20},
    "screen": {"seeds": [20260820], "runs_per_seed": 100},
    "confirm": {"seeds": [20260820, 20260821, 20260822], "runs_per_seed": 100}
  }
}
```

欄位名稱可依 study 調整，但 runner 必須將 resolved manifest（所選 profile、預設值與
實際 output path）寫入 run root 的 `run_config.json`。每個 point 應保存：

```text
point.json       point id、所有實際參數、seed、scope
command.txt      可直接重現的完整 command
run.log          stdout/stderr
raw simulator CSV
```

runner 預設必須拒絕既有的 run root，避免不同 execution 混入同一組 CSV。需要重跑時
使用新 `run-id`，或由未來明確實作的 resume policy 僅補跑「已知缺失且 metadata 完全
相同」的 point；不得無條件覆寫。

## 4. `reports/` 路徑：以 scope 為第一層

新 wrapper 必須遵守 [`REPORTS.md`](../REPORTS.md) 的 scope-first 路徑，而不是採用
舊的泛用 `reports/<study>/<run-id>/`：

```text
reports/
├── legacy/<experiment>/<run-id>/
├── group/<experiment>/<run-id>/
├── device/<experiment>/<run-id>/
└── studies/<study>/<run-id>/
```

單一 scope 的新 run 建議結構如下：

```text
reports/group/<experiment>/<run-id>/
├── run_config.json
├── raw/
│   └── <point-id>/
│       ├── point.json
│       ├── command.txt
│       ├── run.log
│       └── summary.csv            # 或 simulator 原始 CSV contract
├── derived/
│   ├── plot_data.csv
│   └── validation.json
└── plots/
    ├── figure.png
    └── figure.pdf
```

`studies/<study>/<run-id>/` 只用於編排多個 scope 的研究，例如先以 group policy
screen 選候選，再以 device canonical experiment 確認。它不是新的 simulator scope。
study 中可保留 `group/`、`sram/`、`device/` 子輸出及 policy-selection metadata；但
`combined/` 僅能保存 corpus 對應、invariant 驗證、選擇決策與索引，**不可**平均或
排序 group repair rate 與 device repair rate。

已有的 reports 目錄一律不因整理而搬移或重新解讀。scope-first 規則僅適用於後續新增
wrapper/runs。

## 5. Figure 1 已完成的範例

本次已完成一個 group-scope 的 Figure 1 exploratory run：

```text
reports/figure1_repair_rate_2x2_directional_v1/
├── run_config.json
├── seed_20260820/summary.csv
├── seed_20260821/summary.csv
├── seed_20260822/summary.csv
└── plots/
    ├── figure1_plot_data.csv
    ├── figure1_repair_rate_vs_faults.png
    └── figure1_repair_rate_vs_faults.pdf
```

此位置是 `REPORTS.md` 整理前的既有相容輸出；保留原位。若重跑或擴大此實驗，新
runner 應使用：

```text
reports/group/dynamic_spare_sharing/figure1_repair_rate_2x2_directional_v2/
```

Figure 1 固定條件：

| Item | Value |
|---|---|
| Scope | group：獨立 2×2 / 4-SA samples |
| Rs, Cs | 2, 2 |
| Fault severity | 8–32 faults per **whole 2×2 group**，step 4 |
| Fault model / spatial model | `moderate` / `mixed` |
| Policies | No Sharing；Directional m=1；Directional m=2 |
| Seeds / runs | 3 seeds；100 runs/seed；300 samples/curve point |
| Solver controls | paper CAM reuse、`local-first`、`max-borrows=3`、`solution-take=legacy` |

Simulation 使用 `DynamicSpareSharing --repair-rate-sweep`。在每個 `(fault count,
seed)` 中，該 mode 先生成一次 fault groups，再把同一批 groups 傳給 baseline 與
各 policy；因此三條 Figure 1 曲線使用 paired corpus。雖然內建 sweep 也計算 edge
與 global policy，Figure 1 plotter 只選取 `no_sharing`、directional `m=1` 與
directional `m=2`。

[`scripts/group/dynamic_spare_sharing/plot_figure1_repair_rate.py`](../../scripts/group/dynamic_spare_sharing/plot_figure1_repair_rate.py)
只讀取遞迴找到的 `summary.csv`，合併 seed，輸出 plot-data CSV 和 PNG/PDF；**它不會
執行 simulation，也不會呼叫 C++ binary**。目前它會在沒有 CSV、或完全找不到某一種
目標 policy 時停止；若某 policy 只缺少部分 fault-count，則可能畫出不完整 curve，
所以後續應先確認每條 curve 的 fault-count 集合一致。

初步結果顯示 directional sharing 相對 No Sharing 有小幅 repair-rate 改善，但 m=1
與 m=2 大多重疊。此結果不能直接支持很強的「m=2 明顯優於 m=1」主張；增加 runs
主要會縮小不確定性，未必改變此趨勢。

## 6. 投稿期間的最小執行流程

對每一張候選圖或表，採用以下順序：

1. 先寫下 research question、scope、baseline、candidate、固定控制項與唯一 sweep axis。
2. 將設定寫入 manifest；確認同一比較內 `Rs/Cs`、fault corpus、solver/backend 與 seed
   規則一致。
3. 先跑 `smoke`，確認 CSV metadata、point count、無重複 key 和圖表趨勢。
4. 跑 `screen` 取得候選設計；不要因單一 seed 結果就改寫核心模型。
5. 僅對論文保留的 candidate 跑 `confirm`，擴大 seed/runs 或 device groups。
6. collector 應先輸出 scope-specific derived CSV 與 validation，再由 plotter 讀取它。
7. 圖說與表註記 scope、fault-unit（例如 faults/group）、Rs/Cs、fault model、samples 和
   是否為 paired corpus。

對 CAM-to-SRAM 硬體比較，repair-rate functional equivalence 與 same-library synthesis
comparison 是不同驗證層。group simulation 的 storage/latency model 不能替代 gate-level
area 或 timing 結果；synthesis result 也不能替代 repair semantic equivalence。

## 7. 不可違反的比較規則

- 不將 legacy、group、device repair-rate 放進同一條 curve 或平均為同一數值。
- 不把 group-level online CAM 的每-run獨立語意，解釋成 device-wide CAM occupancy。
- 不因 sharing curve 較高而暗示增加 total physical spare；同 Rs/Cs 下必須維持 physical
  spare conservation。
- 不讓 plotter 隱含 simulation，或默默將不同 seeds/runs/geometry 混入同一點。
- 不將 gate count 視為主要 area metric；DFF、comparator、MUX、encoder cell area 不等價。
- 不預設 SRAM 比 CAM 小；主張應由 functionally equivalent、same-library synthesis 的
  area–latency Pareto comparison 支持。

## 8. 下一步建議

1. 若 Figure 1 要成為正式投稿圖，建立其 manifest + runner，使用新的 group scope
   output path，並讓 plotter 對缺失 fault-count 嚴格失敗。
2. 為 CAM/SRAM latency、gate-count 與 area 建立各自的 scope-aware manifest，不將
   simulation cost proxy 與 synthesis area 混在同一欄做排序。
3. 在 confirm 前先決定 Proposed DSS 是否維持 directional m=2；目前 exploratory data
   顯示它與 m=1 的區分有限。
