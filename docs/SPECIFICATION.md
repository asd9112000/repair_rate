# 系統規格

## 1. 系統目的

系統應讀取一組 fault patterns，依指定 spare row／column 資源執行 RECAM 修復求解，並針對單 PE、四配置、四子陣列共享 Ring、3-way 或 SRAM 模式輸出成功 pattern 與 repair rate。

本規格以目前程式可觀察行為為準；「目標規格」小節則供後續重構使用。

## 2. 執行環境

| 項目 | 規格 |
| --- | --- |
| 語言 | C++17 |
| 建置 | GNU Make、`g++` |
| 標準編譯旗標 | `-std=c++17 -DPRINTTIME=true -DNDEBUG` |
| 最佳化 | `-flto -funroll-loops -finline-functions -O3 -ffast-math -march=native` |
| 警告／除錯 | `-g -Wall -Wextra` |
| 腳本 | Bash；CSV 擷取使用 GNU `grep -P` |
| 工作目錄 | 專案根目錄 |

## 3. 建置規格

| Make target | 產物／行為 |
| --- | --- |
| `make rdr_b` | 建立 executable `build/bin/RedundantRate` |
| `make sl_b` | 建立 executable `build/bin/SharedLine` |
| `make sl_3way_b` | 建立 executable `build/bin/SharedLine_3way` |
| `make sl_sram_b` | 建立 executable `build/bin/SharedLine_SRAM` |
| `make dynamic_sharing_b` | 建立 executable `build/bin/DynamicSpareSharing` |
| `make validate_remap_b` | 建立 executable `build/bin/validate_remap` |
| `make fault_generator_b` | 建立 executable `build/bin/fault_generator` |
| `make gen_fault f=N s=H` | 執行 generator，基準 faults=`N`、stack height=`H`、mode=`normal` |
| `make organize_fault [g=N]` | 編譯並執行 fault organizer；可選 subarray group count |
| `make analyze_spareline` | 執行 SharedLine 批次實驗 |
| `make analyze_redundantrate` | 執行 RedundantRate 批次實驗 |

所有 executable 都集中在 `build/bin/` 且不使用 `.o` 副檔名；一般編譯
object 與 dependency files 位於 `build/obj/`，test executables 位於
`build/tests/`。`make clean` 移除 `build/`，但不移除 `reports/`。

## 4. 命令列介面

### 4.1 SharedLine

```text
./build/bin/SharedLine <Rs> <Cs> [--rptName <filename>] [--faultNum <N>]
```

- `Rs`、`Cs`：基準 spare row／column，必要正整數。
- `--rptName`：只接受檔名，輸出仍位於 `reports/SharedLine/`。
- `--faultNum`：實驗 metadata，寫入 stdout summary；不會改變輸入檔內容。
- 固定內部值：`buf_num=2`、`sharedLine_num=1`。

### 4.2 SharedLine 3-way

```text
./build/bin/SharedLine_3way <Rs> <Cs> [--rptName <filename>] [--faultNum <N>]
```

介面大致與 SharedLine 相同，但目前程式末尾的 `RepairRate:` stdout summary 被註解，既有 CSV 擷取流程不可直接套用。

### 4.3 SharedLine SRAM

```text
./build/bin/SharedLine_SRAM <Rs> <Cs> [--rptName <filename>]
```

程式會輸出 repair report、record，以及 remap table。其 main 已有 `argc < 3` 的 usage 檢查。

### 4.4 RedundantRate

```text
./build/bin/RedundantRate <Rs> <Cs> [--rptName <suffix>]
```

輸出位於 `reports/RedundantRate/`。`--rptName` 用來產生個別 report
名稱；Makefile 的 `rdr_r` 另將 stdout 導向同一目錄的
`RedundantRate.log`。

### 4.5 Fault generator

```text
./build/bin/fault_generator
  [--num_subarrays <N>]
  [--fixed_faults <N>]
  [--stack_height <N>]
  [--fault_mode fixed|normal|extreme]
  [--seed <N>]
  [--output <path>]
```

預設值及兩種 generator 的數量分布差異，以 [`fault_generator/README_fault_generator`](fault_generator/README_fault_generator) 為準。

## 5. 輸入格式

主要模擬器固定讀取：

```text
./fault_generator/faults.faults
```

檔案可含設定資訊與註解；清洗後的邏輯資料由多個 pattern 組成。pattern 的第一筆為 fault 數，後續每筆 fault 為：

```text
LogicUnitID LayerID BankID Row Col
```

約束：

- 四 PE 流程預期每個 layer／group 有四個 Logic Unit，並以連續四個 `FaultList` 組成一個 pattern。
- Row、Col 應落在 generator／solver 支援範圍；現行 generator 使用 `0..1023`。
- 現行 generator 的 Bank ID 固定為 `0`。
- SharedLine 的 `patNum` 取 `(Rs,Cs)` 對應 FaultList 數量除以 4；各配置必須由同一輸入生成且數量一致。

## 6. 修復策略規格

### 6.1 單 PE 配置

基準四配置為：

```text
(Rs, Cs)
(Rs-1, Cs)
(Rs, Cs-1)
(Rs-1, Cs-1)
```

`SolGenerator` 必須為每個實際使用的 `(rowCount,columnCount)` 產生 solution matrices。`RECAM_PE` 只在至少一個 solution 通過 fault analysis 與 CAM 限制時回報成功。

### 6.2 SharedLine 候選

令 `S = sharedLine_num`（目前為 1）：

- PE0、PE3 使用 LessRow 候選：`(Rs,Cs+S)`、`(Rs-S,Cs+S)`、`(Rs,Cs)`、`(Rs-S,Cs)`。
- PE1、PE2 使用 LessCol 候選：`(Rs+S,Cs)`、`(Rs,Cs)`、`(Rs+S,Cs-S)`、`(Rs,Cs-S)`。

完整 SharedLine 對四個 PE 做 Cartesian product，現況為 `4^4` 組。候選成功必須同時符合：

1. 每個 PE 在該配置下的 `RepairSuccess` 為真；
2. `ResSpareLines` 依 PE0、PE1、PE2、PE3 順序占用後皆未超額。

同一 pattern 可有多個成功配置，但計算 repair rate 時只計一次。

### 6.3 RECAM baseline

若某個成功組合的四個 PE 都恰為 `(Rs,Cs)`，該 pattern 同時加入 `RepairRate_RECAM` 的成功集合。

## 7. 輸出規格

### 7.1 SharedLine

| 路徑 | 內容 |
| --- | --- |
| `reports/SharedLine/RepairRecord.txt` | 逐候選配置的布林成功記錄 |
| `reports/SharedLine/RepairReport.rpt` | 成功 pattern、RECAM baseline pattern、各 pattern 成功配置 |
| 自訂 report 名稱 | `--rptName` 取代預設 report 檔名 |
| stdout／`SharedLine.log` | pattern 進度與單行 repair-rate summary |
| `reports/SharedLine/repairRates.csv` | 批次腳本彙整結果 |

stdout summary 格式：

```text
RepairRate: <ratio> RepairRate_RECAM: <ratio> SpareLine: <Rs> faultNum: <N>
```

CSV header：

```text
FaultNum,SpareLine,RepairRate,RepairRate_RECAM
```

### 7.2 RedundantRate

主要輸出目錄為 `reports/RedundantRate/`，包含 repair report、repair record、各配置 fault maps、solution vectors／matrices 及 fault analysis matrix 等除錯或分析產物。實際產物會受程式內啟用的 write call 影響。

### 7.3 SRAM

主要輸出目錄為 `reports/SharedLine_SRAM/`：

- `RepairRecord.txt`
- `RepairReport.rpt`
- `RemapTable.txt`
- `RemapTable_simplified.txt`

Remap table header 定義為：

```text
MAP <HBMID> <ChannelID> <BankID> <SubarrayGroupID> <SubarrayID> <r> <c> <R|C> <new_address> <latency>
```

## 8. 數值與錯誤處理

- repair rate 使用 `double`，分母為 `patNum`。
- 若 fault file 無法開啟或配置超過 loader 可處理範圍，SharedLine 顯示 load failed 並以目前實作的 return code 結束。
- `Rs-S`、`Cs-S` 不得為負；目前並非所有入口都主動驗證，因此呼叫端必須提供合法值。
- report directory 由 `std::filesystem::create_directories` 建立。
- 相同 report 名稱通常會被覆寫；SharedLine CSV 腳本使用 append，重跑前應清理或另存。

## 9. 批次實驗規格

`scripts/analyze_SpareLine.sh`：

- fault number：8 到 16；
- stack height：100；
- spare line：2 到 6，且以 `Rs=Cs` 執行；
- 每個 fault number 重新產生一次輸入，供該輪所有 spare line 共用；
- 從 `SharedLine.log` 擷取 summary 寫入 CSV。

`scripts/analyze_RedundantRate.sh`：

- fault number：8 到 14；
- stack height：250；
- spare line：3 到 5，且以 `Rs=Cs` 執行；
- 透過 `make rdr_r` 執行並指定 report 名稱。

## 10. 驗收規格

重構前應保存固定 seed 的輸入與以下 golden data：

- 每個配置的 FaultList／分類數量；
- 每個 PE、pattern、配置的 `RepairSuccess`；
- 每個 pattern 的成功配置集合；
- SharedLine 與 RECAM 成功 pattern 集合；
- repair-rate 數值；
- SRAM remap entries 與 latency（若重構 SRAM 路徑）。

重構完成的必要條件：

1. 相同 input 與參數得到相同成功 pattern 集合，而不只是相近百分比。
2. 四 PE Ring 的資源映射及占用順序若有改變，必須作為演算法版本變更另行驗證。
3. 舊命令列或 batch script 若不再相容，須提供 migration 說明或相容 alias。
4. 新增 unit tests 至少涵蓋 FaultLoader、solution generation、單 PE solver、strategy enumeration 與 Ring resource accounting。

## 11. 目標 CLI／報表規格（尚未實作）

重構方案建議統一為單一 app：

```text
repair-rate --mode baseline|redundant|shared|3way|sram \
  --rows <Rs> --cols <Cs> --fault-file <path> \
  [--shared-lines <N>] [--buffers <N>] \
  [--format txt|rpt|csv] [--output <path>]
```

目標輸出應附帶 input checksum、seed、參數、版本與 pattern count，使研究結果可獨立重現。此介面只是一致化方向，現階段請使用第 4 節所列 legacy commands。
