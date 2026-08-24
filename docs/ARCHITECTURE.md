# 架構說明

## 1. 文件範圍

本文件同時記錄：

1. repository 目前可執行的 legacy 架構；
2. 兩份重構 prompt 所描述的目標架構。

凡標示為「目標」的類別或檔案目前尚未存在，不代表已完成實作。

## 2. 目前架構總覽

```text
┌──────────────────────────────────────────────────────────┐
│ Apps                                                     │
│ basicPEarray | RedundantRate | SharedLine | 3way | SRAM │
└──────────────────────────┬───────────────────────────────┘
                           │ 直接組裝、迴圈、統計、寫檔
          ┌────────────────┼───────────────────┐
          ▼                ▼                   ▼
  FaultLoaderForSpares  FourWayPE       PatternRecorder
          │                │                   │
          ▼                ▼                   ▼
   Fault / FaultList    RECAM_PE          reports/
                           │
             ┌─────────────┼─────────────┐
             ▼             ▼             ▼
         RECAM_CAM  RECAM_bufferCAM  SolGenerator
                           │
                           ▼
                    ResSpareLines
                  （Ring 資源檢查）
```

目前是多入口、共享 solver 元件的單體式批次模擬器。主程式負責 CLI、候選配置建立、fault loading、solution 列舉、四 PE 組裝、統計及輸出；核心類別位於 `inc/` 與 `src/`。

## 3. 執行資料流

### 3.1 Fault 產生

`fault_generator/fault_generator.cpp` 或 `fault_generator_rand_amount.cpp` 根據 Logic Unit、fault 數、stack height、mode 與 seed 產生 `faults.faults`。Makefile 的標準流程使用固定總量版本。

### 3.2 載入與分類

`FaultLoaderForSpares` 對同一輸入建立不同 spare 配置的 `FaultList`。`Fault`／`FaultList` 與 loader 目前集中在 `inc/Fault.hpp`、`src/Fault.cpp`，包含解析、分類與部分狀態管理。

### 3.3 Solution 生成

`SolGenerator` 將 `Rs + Cs` 條 spare lines 的 row／column選擇順序列舉為 `SolutionVector`，再轉為 `SolutionMatrix`。結果依 `SpareConfig` 快取於 unordered map，供多個 PE 與 pattern 重用。

### 3.4 單 PE 求解

`RECAM_PE` 組合 Address／Hybrid CAM 與 Buffer CAM：

1. 載入分類後 faults；
2. 套用 must-rule 與 CAM 限制；
3. 產生 fault analysis matrix；
4. 逐一檢查 `SolutionMatrix`；
5. 設定 `RepairSuccess` 並保留有效 solution。

### 3.5 四配置封裝

`FourWayPE` 這個名稱容易誤解：每一個實例實際上包裝「同一個 PE 的四種 spare 配置」：

- `PE_RsCs`
- `PE_RsReduced`
- `PE_CsReduced`
- `PE_RsCsReduced`

SharedLine 主流程會建立四個 `FourWayPE` 實例，分別代表 PE0～PE3，再透過 `perWayRepairSuccessListForSparess` 查詢特定 `(rowCount,columnCount)` 是否可修復。

### 3.6 Ring 資源仲裁

`ResSpareLines` 保存四個 PE 的專屬 row／column 餘額與四段 shared-line 餘額。`occupySpareLines(PE_index, usedRow, usedCol)` 依 PE index 將 row 和 column 需求映射至相鄰共享段。

初始化規則：

- PE0、PE3：專屬 row 為 `Rs - sharedLine`，專屬 column 為 `Cs`。
- PE1、PE2：專屬 row 為 `Rs`，專屬 column 為 `Cs - sharedLine`。
- 每段共享餘額為 `sharedLine_num`。

需求先扣專屬資源，不足部分才扣對應 shared segment。SharedLine 目前以 PE0 → PE1 → PE2 → PE3 的順序檢查，因此仲裁順序是現行演算法語意的一部分。

### 3.7 彙總與輸出

各 app 在主程式內維護統計或 `PatternRecorder`。SharedLine 對每個 pattern 列舉最多 `4^4 = 256` 組候選配置，記錄成功配置、共享策略成功 pattern、固定 `(Rs,Cs)` baseline pattern，最後寫入 record、report 與 stdout summary。

## 4. 主要元件責任

| 元件 | 目前責任 | 架構問題 |
| --- | --- | --- |
| `Fault`／`FaultList` | fault 資料、分類狀態 | domain 與 loading/classification 邊界不清 |
| `FaultLoaderForSpares` | 對多配置解析相同輸入 | 與檔案格式及 solver 配置耦合 |
| `SolGenerator` | solution vector/matrix 列舉與快取 | legacy type alias 仍存在 |
| `RECAM_CAM` | CAM 規則與可達性 | debug／輸出責任尚未完全分離 |
| `RECAM_bufferCAM` | buffer CAM 邏輯 | 由 `RECAM_PE` 直接組裝 |
| `RECAM_PE` | 單 PE fault analysis 與 solution 驗證 | 可變狀態多，缺少單一 `solve` 結果介面 |
| `FourWayPE` | 同一 PE 的四種配置封裝 | 命名與「四個 PE」概念混淆 |
| `ResSpareLines` | 固定四 PE Ring 資源帳本 | topology 與 PE index 寫死 |
| 各 `main` | pipeline、策略、統計、I/O | 多入口重複且難以單元測試 |

## 5. 入口與變體

- `RedundantRate.cpp`：不進行 Ring 借用，觀察四種降低配置及四 PE 組合結果。
- `SharedLine.cpp`：完整四候選共享策略，輸出共享與固定 RECAM repair rate。
- `SharedLine_3way.cpp`：移除一個資源需求較高的候選路徑，縮小搜尋空間。
- `SharedLine_SRAM.cpp`：沿用共享求解，另產生完整與 simplified remap table，表達 address remap 與 latency。
- `basicPEarray.cpp`：較早期／基本 RECAM 流程，用於 baseline 與除錯。

## 6. 目前耦合與風險

- 策略配置、四 PE 拓樸及 I/O 直接寫在 app 中。
- `PatternRecorder` 在多個入口重複定義。
- Ring 資源分配依陣列 index 與 switch，擴充 N-way 容易破壞語意。
- fault analysis、solver debug output 和正式報表沒有一致介面。
- CLI 驗證不一致；部分程式在 `argc < 3` 時可能直接存取無效參數。
- Makefile 的 `_b` target 將 executable 集中到 `build/bin/`，一般 object
  放在 `build/obj/`，test executable 放在 `build/tests/`。
- 所有 simulation、log、CSV 與圖表結果集中在 `reports/`；`build/` 可安全
  重建，`reports/` 則不會被 `make clean` 移除。

## 7. 目標架構（重構方向）

```text
CLI / App
    │
    ▼
ExperimentRunner ───────────────► Reporter / CsvExporter
    │                                      │
    ├──► FaultLoader ──► Domain DTO ◄──────┘
    │
    ├──► StrategyEnumerator ──► RingTopology
    │
    └──► Solver interface ──► RECAM_PE / CAM / SolGenerator
```

建議分成七個責任區：

1. **Domain Model**：`Fault`、`FaultList`、`SpareConfig`、`RepairResult`，不進行檔案 I/O。
2. **Fault Ingestion**：清洗、解析及分類輸入。
3. **Solver Core**：單 PE 求解，提供類似 `solve(faults, config) -> RepairResult` 的介面。
4. **Spare Strategy**：候選配置生成、Ring topology 與資源規則。
5. **Experiment Orchestrator**：pattern／PE／strategy 迴圈及統計。
6. **Report/Export**：純粹將 in-memory result 寫為 txt／rpt／csv。
7. **CLI/App**：參數解析、組裝依賴與 legacy command 相容層。

依賴應由上層指向下層，Reporter 不應反向依賴 solver，Domain 不應依賴任何 I/O 或 app。

## 8. 建議遷移順序

1. 固定相同 input 與現有 output，建立 golden baseline。
2. 抽離 Domain 與 FaultLoader，比對分類數與 fault map。
3. 抽離 Solver Core，比對每個 pattern／配置的 `RepairSuccess` 與 matrix。
4. 抽離 Strategy 與 ExperimentRunner，比對每個 pattern 的成功配置集合。
5. 統一 Reporter、CLI 與 Makefile，同時保留一版 legacy 別名。

每一步都不應同時修改修復演算法。重構驗收標準以相同 fault input 得到相同成功 pattern 集合及 repair rate 為主，而不只比較格式化後的百分比。
