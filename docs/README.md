# repair_rate

`repair_rate` 是一套以 C++17 撰寫的模擬工具，用來評估 Processing Element（PE）陣列在不同 spare row／spare column 配置下的修復率。專案目前可比較單 PE、四 PE 配置、四子陣列共享線（Ring）、3-way 共享策略，以及以 SRAM remap table 表示並納入 latency 的版本。

> 本文件描述 repository 的目前實作。重構後的目標架構另見 [ARCHITECTURE.md](ARCHITECTURE.md#目標架構重構方向)，尚未存在的模組不應視為已完成。

## 文件導覽

- [PROJECT_CONTEXT.md](PROJECT_CONTEXT.md)：研究背景、問題定義、實驗假設與評估指標
- [ARCHITECTURE.md](ARCHITECTURE.md)：目前程式流程、模組責任、Ring 資源模型與重構方向
- [SPECIFICATION.md](SPECIFICATION.md)：建置、命令列、輸入輸出格式及驗收規格
- [`fault_generator/README_fault_generator`](fault_generator/README_fault_generator)：fault generator 的模式與參數細節

## 實驗模式

| 程式 | 用途 | 主要輸出 |
| --- | --- | --- |
| `basicPEarray.cpp` | 四個 PE 都採固定 `(Rs,Cs)` 的 RECAM baseline | `reports/basicPEarray/` |
| `RedundantRate.cpp` | 對同一 FaultList 比較 `(Rs,Cs)`、`(Rs-1,Cs)`、`(Rs,Cs-1)`、`(Rs-1,Cs-1)` | `reports/RedundantRate/` |
| `SharedLine.cpp` | 四子陣列共享 spare line，並與固定 `(Rs,Cs)` RECAM 配置比較 | `reports/SharedLine/` |
| `SharedLine_3way.cpp` | SharedLine 的 3-way 策略變體 | `reports/SharedLine_3way/` |
| `SharedLine_SRAM.cpp` | SRAM remap table 與 latency 版本 | `reports/SharedLine_SRAM/` |

## 快速開始

需求：GNU Make、支援 C++17 的 `g++`，以及 Bash（批次分析腳本另使用 GNU `grep -P`）。

```bash
# 編譯主要模擬器
make sl_b
make rdr_b
make sl_3way_b
make sl_sram_b

# 產生 4 個 Logic Unit、每個基準 10 faults、100 層的輸入
make gen_fault f=10 s=100

# 執行 SharedLine；Rs=Cs=3
./build/bin/SharedLine 3 3 \
  --rptName SL_RepairReport_r3_c3_f10.rpt \
  --faultNum 10

# 執行其他模式
./build/bin/RedundantRate 3 3
./build/bin/SharedLine_3way 3 3 --rptName SL3_RepairReport_r3_c3_f10.rpt --faultNum 10
./build/bin/SharedLine_SRAM 3 3 --rptName SRAM_RepairReport_r3_c3_f10.rpt
```

`make gen_fault` 會先按需要建立 `build/bin/fault_generator`，不需要手動編譯
generator。

## 批次實驗

```bash
make analyze_spareline
make analyze_redundantrate
```

- `analyze_spareline` 目前掃描 fault 數 `8..16`、spare line `2..6`，輸出 `reports/SharedLine/repairRates.csv`。
- `analyze_redundantrate` 目前掃描 fault 數 `4..16`、spare line `2..6`，輸出多份 RedundantRate report。

腳本會覆寫或追加既有 log／CSV；進行可重現實驗前，請自行保存所需結果並固定 generator 的 `--seed`。Makefile 的 `gen_fault` 目前未轉傳 seed。

## 核心流程

```text
fault generator
    │  faults.faults
    ▼
FaultLoaderForSpares ── 同一 fault pattern 轉成多種 spare 配置
    ▼
RECAM_PE / RECAM_CAM / RECAM_bufferCAM
    │  單 PE 各配置是否可修復
    ▼
FourWayPE + ResSpareLines
    │  四 PE 配置列舉與 Ring 資源占用檢查
    ▼
PatternRecorder / report files / repair rate
```

修復率定義為：

```text
repair rate = 成功修復的 fault pattern 數 / fault pattern 總數
```

SharedLine 額外輸出 `RepairRate_RECAM`，表示四個 PE 都採用固定 `(Rs,Cs)` 配置時的 baseline。

## 目錄摘要

```text
.
├── *.cpp                    # 各實驗入口
├── inc/                     # 資料結構與求解器介面
├── src/                     # Fault、CAM、PE、solution 實作
├── fault_generator/         # fault 產生與整理工具
├── scripts/                 # 批次實驗腳本
├── build/
│   ├── obj/                 # C++ 中間 object 與 dependency files
│   ├── bin/                 # simulator／tool executables（無 .o 副檔名）
│   └── tests/               # test executables
├── reports/                 # simulation、CSV、log 與圖表結果
└── Makefile
```

`build/` 全部是可由 source 重建的產物，不應提交版本控制；不同參數的
simulation 共用同一 simulator，並以 `reports/` 下不同目錄保存結果。

## 已知限制

- 目前多個入口各自包含流程控制與報表程式，存在重複邏輯。
- Ring 拓樸、四個 PE、`buf_num = 2` 與 `sharedLine_num = 1` 主要為程式內固定值。
- 多數入口直接讀取 `./fault_generator/faults.faults`，執行時應位於專案根目錄。
- `SharedLine.cpp`、`RedundantRate.cpp` 等程式在讀取 `argv[1]`、`argv[2]` 前未一致檢查參數數量；請務必提供正整數 `Rs Cs`，且共享模式應使用至少為 1 的值。
- `make clean` 只移除整個 `build/`；`reports/` 中的 simulation 結果會保留。
