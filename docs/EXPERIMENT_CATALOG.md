# 實驗目錄與操作入口

> 文件狀態：Current
> 適用範圍：cross-cutting
> 建立時間：2026-09-09T03:52:57+08:00
> 最後修改時間：2026-09-09T13:47:18+08:00
> 本文件權威主題：正式實驗 workflow 的選擇、執行入口與資料定位

本文件回答三個日常問題：要跑什麼指令、既有資料在哪裡、plot/table 應使用哪份
CSV。它只列正式且可重現的 workflow；架構語意以
[`ARCHITECTURE.md`](ARCHITECTURE.md) 為準，輸出 scope 與新 run 路徑以
[`REPORTS.md`](REPORTS.md) 為準。

## 1. 先選 scope，再選 workflow

| 想回答的問題 | Scope | 使用 workflow | 不可混用的結果 |
|---|---|---|---|
| 一個 2×2 / 4-SA group 內 sharing 是否改善 repairability？ | group | DATE-2X2 或 DSS-SWEEP | device canonical repair rate |
| 比較一般 Dynamic Spare Sharing topology、spare count 與 fault severity？ | group | DSS-SWEEP | legacy SharedLine 或 device 結果 |
| 比較 CAM/SRAM-RECAM search policy 的 group-level modeled latency/cost？ | group | SRAM-RECAM-GROUP | RTL cell area/timing |
| 比較 B0–B3、finite global CAM 與 device-level hierarchy？ | device | CANONICAL-FOUR | group Monte Carlo repair rate |
| 對 moderate study 的 group/SRAM/device 做分階段研究？ | studies | MODERATE-STUDY | 不可將其 `combined/` 做跨 scope repair-rate 平均 |
| 取得 technology-mapped cell area、delay 與 gate/cell breakdown？ | RTL results | RTL-SYNTHESIS | C++ hardware-cost proxy |

`group` 的一筆 run 是獨立 4-SA sample；不會跨 group 累積 online CAM occupancy。
`device` 則是同一 modeled device 下的 global CAM pool。兩者的 repair rate 必須分開
讀取與繪圖。

## 2. 工作方式：一份 manifest、一個 runner、一個 run bundle

執行新實驗時，先從下表選定 workflow，不要手動拼接舊 command，也不要讓 plotter
遞迴掃描整個 `reports/`。每個正式 run 都必須使用新的 `run-id`，並保留：

```text
run_config.json       resolved manifest、profile、git revision、output root
raw/<point>/          原始 simulator CSV、point.json、command.txt、run.log
derived/ 或 csv/      僅供 figure/table 使用的已驗證彙整 CSV
plots/ 或 figures/    PNG、PDF、caption
tables/               publication table 的 CSV/Markdown/LaTeX
summary.md / README.md 結果摘要、重新執行方式與 artifact index
```

既有舊資料可保留原路徑；不要為了符合新結構搬移或混入新 run。若不知道一份資料的
設定，先讀同層的 `run_config.json`；若沒有該檔，該資料只能作 exploratory/legacy
參考，不應直接作為投稿主圖來源。

## 3. 正式 workflow 清單

### DATE-2X2 — DATE 投稿的 2×2 DSS 主實驗

**何時使用：** 本次投稿的 Figure 1–4、DSS repair-rate、gain、fault imbalance、
spare utilization 與 Table 1。固定比較 No Sharing、Directional m=1、Directional m=2；
不包含 1×4。

| 項目 | 位置／指令 |
|---|---|
| Manifest | [`experiments/date_submission_2x2.json`](../experiments/date_submission_2x2.json) |
| Multi-spare manifest | [`experiments/date_submission_2x2_spare_sweep.json`](../experiments/date_submission_2x2_spare_sweep.json) (`Rs=Cs=2,3,4,5`) |
| Runner | [`scripts/group/date_submission/run.py`](../scripts/group/date_submission/run.py) |
| Analyzer / plotter | [`scripts/group/date_submission/analyze.py`](../scripts/group/date_submission/analyze.py) |
| 新 run root | `reports/group/date_submission_2x2/<run-id>/` |

```bash
# 快速確認：build + focused regression + 小樣本 simulation + analysis
python3 scripts/group/date_submission/run.py \
  --profile smoke --run-id date_submission_smoke_<date>

# 正式資料：三個固定 seeds、每 seed 1000 runs
python3 scripts/group/date_submission/run.py \
  --profile confirm --run-id date_submission_confirm_<date>

# Rs=Cs=2,3,4,5；每個 configuration 產生獨立 Figure 1–4 與跨-spare heatmaps
python3 scripts/group/date_submission/run.py \
  --manifest experiments/date_submission_2x2_spare_sweep.json \
  --profile smoke --run-id date_submission_spare_sweep_smoke_<date>

# simulation 已完成時，只重新整理 CSV/table/figure
python3 scripts/group/date_submission/analyze.py \
  reports/group/date_submission_2x2/<run-id>
```

**正式圖表資料：**

```text
csv/fig1_repair_rate_summary.csv
csv/fig1_repair_rate_long.csv
csv/fig2_dss_gain.csv
csv/fig3_fault_imbalance.csv
csv/fig4_spare_utilization.csv
tables/table1_dss_repairability.{csv,md,tex}
```

**已存在 run：**

```text
reports/group/date_submission_2x2/date_submission_smoke_20260907/
reports/group/date_submission_2x2/date_submission_confirm_20260907/
```

若使用確認版 Figure 1，應讀取
`reports/group/date_submission_2x2/date_submission_confirm_20260907/`
下的 `csv/fig1_repair_rate_summary.csv`，而非任意其他 `summary.csv`。

### DSS-SWEEP — 一般 4-SA Dynamic Spare Sharing sweep

**何時使用：** 探索 fault count、Rs/Cs、fault model、topology 或 layout 的 group-level
sweep。它會產生 No Sharing、Directional、Pairwise Edge、Global 等比較；不是 DATE
主圖的唯一正式入口。

| 項目 | 位置／指令 |
|---|---|
| Runner | [`scripts/group/dynamic_spare_sharing/sweep.sh`](../scripts/group/dynamic_spare_sharing/sweep.sh) |
| 一般 plotter | [`scripts/group/dynamic_spare_sharing/plot_sweep.py`](../scripts/group/dynamic_spare_sharing/plot_sweep.py) |
| Figure 1 standalone plotter | [`scripts/group/dynamic_spare_sharing/plot_figure1_repair_rate.py`](../scripts/group/dynamic_spare_sharing/plot_figure1_repair_rate.py) |
| 新 run root | `reports/group/dynamic_spare_sharing/<run-id>/` |

```bash
scripts/group/dynamic_spare_sharing/sweep.sh \
  --layout 2x2 --fault-min 16 --fault-max 32 --fault-step 4 \
  --spare-min 2 --spare-max 2 --fault-model moderate --spatial mixed \
  --runs 100 --seed 20260820 --run-id exploratory_2x2_<date>
```

主要原始資料是 `<run-root>/raw/summary.csv`。一般 plotter 只接受一份指定 CSV；
Figure 1 standalone plotter 接受一個包含各 seed `summary.csv` 的指定 input root，
但不會執行 simulation。它只適合 No Sharing、Directional m=1/m=2 的 paired 2×2
CAM sweep。

### SRAM-RECAM-GROUP — group-level CAM/SRAM search-policy model

**何時使用：** 比較 Serial、Chunked、Wide 等 SRAM-RECAM search policies 的
functional result、modeled latency 與 cost metrics。這不是 same-library RTL synthesis。

| 項目 | 位置／指令 |
|---|---|
| Runner | [`scripts/group/sram_recam/sweep.sh`](../scripts/group/sram_recam/sweep.sh) |
| 新 output | `reports/group/sram_recam/<run-id>/`；若 wrapper 未提供 run-id，先以明確 `--output-dir` 建立新目錄 |
| 原始資料 | 該 output root 的 simulator CSV；先讀其 metadata 再作圖 |

執行前先看 help 或 script 內預設，因為目前 wrapper 的預設 configuration 是代表性
single point；要做投稿 sweep 時應由 DATE manifest/runner 的 hardware phase 或新 manifest
明確列出 entries、width、policy 與 seed，不可把多次覆寫的預設輸出當成正式資料。

### CANONICAL-FOUR — device B0/B1/B2/B3 hierarchy 實驗

**何時使用：** 研究 WoW-v1.0 device-level sharing、CAM-vs-SRAM backend、finite global
CAM capacity 與 BIRA scheduling。

| 項目 | 位置／指令 |
|---|---|
| Sweep runner | [`scripts/device/canonical_four/sweep.py`](../scripts/device/canonical_four/sweep.py) |
| Plotter | [`scripts/device/canonical_four/plot.py`](../scripts/device/canonical_four/plot.py) |
| 新 run root | `reports/device/canonical_four/<run-id>/` |

先執行 `python3 scripts/device/canonical_four/sweep.py --help` 取得必填 axes；每個 point
的原始 B0–B3 output 及 command 位於 run root，後續圖表只讀其 canonical merged CSV。

### MODERATE-STUDY — 三 scope 的編排式研究

**何時使用：** 需要 group policy screen、SRAM policy screen 與 device confirm 的完整
moderate study。它會保存 selection decision，但不允許將 group/device repair rate 合併。

| 項目 | 位置／指令 |
|---|---|
| Manifest | [`experiments/moderate_repair_study.json`](../experiments/moderate_repair_study.json) |
| Runner | [`scripts/device/moderate_repair_study/run.py`](../scripts/device/moderate_repair_study/run.py) |
| Collector | [`scripts/device/moderate_repair_study/collect.py`](../scripts/device/moderate_repair_study/collect.py) |
| Plotter | [`scripts/device/moderate_repair_study/plot.py`](../scripts/device/moderate_repair_study/plot.py) |
| 新 run root | `reports/studies/moderate_repair/<profile-or-run-id>/` |

```bash
python3 scripts/device/moderate_repair_study/run.py --profile smoke
python3 scripts/device/moderate_repair_study/run.py --profile screen
python3 scripts/device/moderate_repair_study/run.py --profile confirm \
  --selection-from reports/studies/moderate_repair/screen/selected_policies.json
```

### RTL-SYNTHESIS — technology-mapped hardware area/timing

**何時使用：** 對 functionally equivalent CAM/register/SRAM-like RTL 實作做
same-library cell area、critical delay、lookup-cycle 的 hardware comparison。

| 項目 | 位置／指令 |
|---|---|
| Synthesis entry directory | [`scripts/synthesis/`](../scripts/synthesis/) |
| Technology-mapped DC flow | [`scripts/synthesis/run_multi_config_phase2_dc.sh`](../scripts/synthesis/run_multi_config_phase2_dc.sh) |
| RTL simulation | [`scripts/simulation/`](../scripts/simulation/) |
| Verified flow / library / PVT / result location | [`docs_verilog/TSMC018_SYNTHESIS_FLOW.md`](../docs_verilog/TSMC018_SYNTHESIS_FLOW.md) |
| Output scope | `results/`，不可放入 C++ `reports/` |

RTL synthesis 的 parsed report、raw report、constraints、library/PVT 與 RTL revision
必須一起保存。C++ `hardware_cost_proxy`、BIRA modeled bits 或 search-cycle model 可當
architecture context，但不得取代 actual cell area 或 critical delay。

## 4. 找既有資料的順序

1. 先由本目錄選 workflow 和 scope。
2. 在該 workflow 的 run-root pattern 下列出 run-id；不要全 repository 搜尋 `*.csv`。
3. 讀 `run_config.json`，確認 manifest、seed、Rs/Cs、fault model、topology、git revision。
4. 若是正式 figure/table，讀 `csv/`、`derived/` 或 `tables/` 指定的檔案；`raw/summary.csv`
   只用於重新 collect/analyze。
5. 若 run root 的 `README.md`、`summary.md` 或 `completion_report.md` 存在，先讀它以取得
   已知限制、重畫 command 與 artifact index。

舉例：DATE confirm Figure 2 的唯一資料來源是：

```text
reports/group/date_submission_2x2/date_submission_confirm_20260907/
    csv/fig2_dss_gain.csv
```

不是 `reports/dynamic_spare_sharing/` 中的歷史 sweep，也不是舊的
`reports/figure1_repair_rate_2x2_directional_v1/` exploratory output。

## 5. 新實驗或新圖表的登錄規則

新 workflow 必須同時新增：

1. `experiments/<study>.json`：machine-readable settings；
2. `scripts/<scope>/<study>/run.py` 或明確 runner：唯一 simulation 入口；
3. `analyze.py`／plotter：僅接受指定 run root 或指定 derived CSV；
4. 本文件的一列：問題、scope、command、manifest、run root、正式 CSV；
5. run root 的 `README.md`：這次 execution 的精確重跑與 re-analysis command。

不要只因為「未來可能會用到」就跑一份巨大全域 sweep。只有在所有 points 具有相同
scope、fault corpus policy、solver/backend contract、unit 與比較語意時，才可做一份
大型 sweep；之後的圖表仍必須以 manifest 選出明確 subset，不可任意抓取。
