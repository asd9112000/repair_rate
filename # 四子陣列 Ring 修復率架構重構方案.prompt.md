# 四子陣列 Ring 修復率架構重構方案

**專案路徑：** `/home/asd9112000/repair_rate`
**基礎版本：** RECAM_ori.txt（單 subarray）→ 4 個 subarray 互相共用冗餘 spare row/col
**目標：** 提升代碼維護性、支援 (Rs, Cs), (Rs-1, Cs), (Rs, Cs-1), (Rs-1, Cs-1) 策略比較

---

## 1. 高層掃描（入口、編譯、腳本、I/O）

### 主要可執行入口
- **SharedLine.cpp**：4-subarray 共用 spare line 的主流程
- **RedundantRate.cpp**：四種 spare 配置比較
- **basicPEarray.cpp**：單純 4 個 PE baseline
- **fault_generator/fault_generator.cpp**：fault pattern 產生器

### 編譯目標 (Makefile)
- 主要二進制：`rdr_b`、`sl_b`、`gen_fault`、`rdr_r`、`sl_r`
- Object files：`RedundantRate.o` 等核心物件
- 風險：根目錄 Makefile 與 fault_generator/ 內 Makefile 路徑不一致

### 執行腳本
- `analyze_SpareLine.sh`：批次 `gen_fault` + `sl_r` 或 `sl_b`
- `analyze_RedundantRate.sh`：批次 `gen_fault` + `rdr_r` / `rdr_b`，嘗試抽 RepairRate CSV

### 主要 I/O 路徑
| 階段 | 輸入 | 輸出 |
|------|------|------|
| Fault 生成 | `fault_generator/faults.faults` config | `faults.faults` |
| SharedLine 求解 | 四種 spare 配置 | `SharedLineReport/` (txt/rpt) |
| RedundantRate 求解 | 四種 spare 配置 + ring 共用 | `RedundantRate/` (txt/rpt) |
| 基線比較 | 單一配置 | `basicPEarrayReport/` |

---

## 2. 目前流程釐清（fault 生成 → 載入 → 4-subarray ring 求解 → repair rate 輸出）

### 2.1 Fault 生成階段
- **檔案**：`fault_generator/fault_generator.cpp`
- **流程**：
  1. 參數：邏輯單元數、固定 fault 比例、stack 高度、seed
  2. 機率模型：cluster 機率 0.2、同行機率 0.3
  3. 輸出格式：先寫 pattern fault 總數，再逐筆 `LU Layer Bank Row Col`

### 2.2 Fault 載入與分類
- **檔案**：`inc/Fault.hpp`、`src/Fault.cpp`
- **流程**：
  1. 讀檔 + 去註解
  2. 分類為 `pivot`、`nonPivot`、`buffer`、`overflowPivot`
  3. 建立 FaultList 容器

### 2.3 單 PE 求解核心
- **檔案**：`inc/RECAM_PE.hpp`、`src/RECAM_PE.cpp`、`inc/SolGenerator.hpp`、`src/SolGenerator.cpp`
- **流程**：
  1. 裝載 fault 到 Address/Hybrid/Buffer CAM
  2. 計算 must-rule 與 CAM 可達性
  3. 生成 fault analyze matrix
  4. 列舉全部可行 solution，驗證每組是否 repair 成功

### 2.4 4-subarray Ring + 共用 Spare Row/Col
- **檔案**：`inc/FourWayPE.hpp`、`inc/ResSpareLines.hpp`、`SharedLine.cpp`
- **流程**：
  1. 每 PE 先以單一求解器檢查各 spare 配置
  2. 再以 ring 借用規則檢查整體可行性
  3. 記錄每組配置成功與否

### 2.5 Repair Rate 統計輸出
- **檔案**：`SharedLine.cpp`、`RedundantRate.cpp`
- **流程**：
  1. 彙總成功 pattern 與配置組合
  2. 計算 repair rate = (successful patterns) / (total patterns)
  3. 寫 txt/rpt/csv 報表至 `mainReport/`、`SharedLineReport/`、`RedundantRate/`

---

## 3. 耦合點與可分層切點（目前混在一起的位置）

### 3.1 資料模型 + I/O 混合
- `Fault` 既承載資料又負責分類狀態
- 主程式直接寫檔而未透過服務層

### 3.2 求解核心 + 報表混合
- 求解邏輯與統計輸出在同一函數中
- matrix 生成與 debug 輸出未分離

### 3.3 策略硬編碼在主程式
- 4-way ring 位置、spare 配置枚舉直接在 main 中
- 無配置文件或 strategy pattern

### 3.4 Ring 資源策略與 Solver 緊耦合
- 借用規則依 PE index + switch 寫死
- 難以擴充為 N-way 或其他拓樸

### 3.5 多入口程式碼重複
- SharedLine.cpp、RedundantRate.cpp、basicPEarray.cpp 幾乎複製
- Makefile 產生多個相同的二進制

### 3.6 可維護性風險
- **懸空指標**：區域變數位址 push 進容器
- **邏輯反向**：判斷與輸出訊息語意反向
- **腳本相容**：期待 `RepairRate:` log 標籤，但實際輸出已更改

---

## 4. 建議模組切分（7 個模組）

### 4.1 Domain Model 模組
**職責**：Fault、FaultList、Solution、PE 配置 DTO
**輸入**：parsed fault / config parameter
**輸出**：純資料物件（無檔案 I/O）
**依賴**：無（最底層）
**涉及檔案**：
- `inc/Fault.hpp`
- `inc/SolGenerator.hpp` (Solution DTO)
- 新增：`inc/Config.hpp` (PE/Ring 配置參數)

### 4.2 Fault Ingestion 模組
**職責**：fault 檔 parsing、清洗、分類流程封裝
**輸入**：faults 檔路徑、分類規則
**輸出**：FaultList 集合、統計（pivot/nonPivot 數量）
**依賴方向**：依賴 Domain
**涉及檔案**：
- `src/Fault.cpp`
- 新增：`inc/FaultLoader.hpp`、`src/FaultLoader.cpp`

### 4.3 RECAM Solver Core 模組
**職責**：Address/Hybrid/Buffer CAM 裝載、must-rule、matrix 轉換、solution 檢查
**輸入**：單 PE FaultList + solution 集合
**輸出**：repair success flag、debug matrix (optional)
**依賴方向**：依賴 Domain
**涉及檔案**：
- `inc/RECAM_PE.hpp`、`src/RECAM_PE.cpp`
- `inc/RECAM_CAM.hpp`、`src/RECAM_CAM.cpp`
- `inc/RECAM_bufferCAM.hpp`、`src/RECAM_bufferCAM.cpp`
- `inc/SolGenerator.hpp`、`src/SolGenerator.cpp`

### 4.4 Spare Strategy 模組
**職責**：策略生成 (Rs, Cs), (Rs-1, Cs), ... 與 ring 共享約束
**輸入**：base PE config、ring topology (4-way / N-way)
**輸出**：可行配置集合、資源占用結果
**依賴方向**：依賴 Solver Core 介面，不碰 I/O
**涉及檔案**：
- `inc/FourWayPE.hpp`、`src/FourWayPE.cpp` (現有 cpp 未見，需整理)
- `inc/ResSpareLines.hpp`、`src/ResSpareLines.cpp`
- 新增：`inc/StrategyEnumerator.hpp` (Rs/Cs 降階組合生成)

### 4.5 Experiment Orchestrator 模組
**職責**：批次 pattern 執行、PE 組裝、統計彙總
**輸入**：策略、fault source、solver
**輸出**：in-memory 指標 (repair rate、成功 pattern 清單、配置對應表)
**依賴方向**：依賴 Strategy + Ingestion + Solver
**涉及檔案**：
- 新增：`inc/ExperimentRunner.hpp`、`src/ExperimentRunner.cpp`

### 4.6 Report/Export 模組
**職責**：txt/rpt/csv 輸出、格式轉換
**輸入**：Orchestrator 統計結果
**輸出**：檔案
**依賴方向**：只依賴 Domain DTO；無反向依賴
**涉及檔案**：
- 新增：`inc/Reporter.hpp`、`src/Reporter.cpp`
- 新增：`inc/CsvExporter.hpp`、`src/CsvExporter.cpp`

### 4.7 CLI/App 模組
**職責**：參數解析、pipeline 組裝、舊二進制相容
**輸入**：CLI args
**輸出**：執行結果 code / log
**依賴方向**：最上層，依賴所有服務介面
**涉及檔案**：
- `SharedLine.cpp` (重構後精簡)
- `RedundantRate.cpp` (重構後精簡)
- `basicPEarray.cpp` (重構後精簡)

---

## 5. 可執行重構工作流（5 階段）

### Phase 1：建立穩定基線（不改演算法，僅檢查）
**目標**：把「目前輸入→輸出」固定成 regression baseline

**涉及檔案**：
- Makefile
- analyze_SpareLine.sh
- analyze_RedundantRate.sh
- SharedLine.cpp、RedundantRate.cpp、basicPEarray.cpp

**流程**：
1. 用相同 faults 檔執行現有 `sl_b`、`rdr_b`
2. 記錄 `SharedLineReport/` 與 `RedundantRate/` 的 repair rate 數值
3. 建立 golden files (git 追蹤)

**風險**：腳本與程式輸出欄位不一致會造成後續驗證失敗

**回滾點**：保留現有報表目錄快照

---

### Phase 2：抽離 Domain + Fault Ingestion
**目標**：把 fault parsing/classification 從主程式抽成獨立服務

**涉及檔案**：
- `inc/Fault.hpp` (補充 DTO)
- `src/Fault.cpp` (重構分類邏輯)
- 新增：`inc/FaultLoader.hpp`、`src/FaultLoader.cpp`
- 新增：`inc/Config.hpp` (PE 配置參數 DTO)

**流程**：
1. 將 `Fault` 改為純資料物件，去掉 I/O
2. 建立 `FaultLoader` 服務，負責檔案讀取與分類
3. SharedLine/RedundantRate 改呼叫 FaultLoader，驗證結果一致

**風險**：分類邏輯微變造成 repair rate 漂移

**回滾點**：以相同 `faults.faults` 對比 pivot/nonpivot 數量與 `fault_map.txt`

---

### Phase 3：抽離 Solver Core（單 PE 求解）
**目標**：讓 RECAM_PE/CAM 成為純求解引擎，輸入 FaultList，輸出 repair success

**涉及檔案**：
- `inc/RECAM_PE.hpp`、`src/RECAM_PE.cpp` (新增統一介面)
- `inc/RECAM_CAM.hpp`、`src/RECAM_CAM.cpp` (去掉 debug 輸出)
- `inc/RECAM_bufferCAM.hpp`、`src/RECAM_bufferCAM.cpp`
- `inc/SolGenerator.hpp`、`src/SolGenerator.cpp` (轉為 pure service)

**流程**：
1. RECAM_PE 定義 `solve(FaultList, SpareConfig) → RepairResult`
2. RepairResult 包含：success flag、debug matrix (optional)、統計資訊
3. 移除 RECAM_PE 中的檔案寫操作
4. 用 SharedLine/RedundantRate 驗證：相同 fault + spare 組合的結果一致

**風險**：must-rule 與 matrix 擴張行為被意外改動

**回滾點**：固定數組 pattern 對比 `fault_analyze_matrix_PE.txt` 完全一致

---

### Phase 4：抽離 Spare Strategy + 4-way Ring Orchestrator
**目標**：把 (Rs,Cs) 降階枚舉、4-way ring 共享規則從主程式移出

**涉及檔案**：
- `inc/FourWayPE.hpp` (現有，補充清晰介面)
- `inc/ResSpareLines.hpp` (現有，補充清晰介面)
- 新增：`inc/StrategyEnumerator.hpp`、`src/StrategyEnumerator.cpp`
- 新增：`inc/ExperimentRunner.hpp`、`src/ExperimentRunner.cpp`

**流程**：
1. StrategyEnumerator：列舉 (Rs, Cs), (Rs-1, Cs), (Rs, Cs-1), (Rs-1, Cs-1)
2. ExperimentRunner：
   - 迴圈遍歷每個 fault pattern
   - 迴圈遍歷每個 spare 策略
   - 組裝 4 PE + ring 借用規則
   - 呼叫 Solver Core 求解
   - 統計成功率
3. 主程式（SharedLine.cpp 等）僅呼叫 ExperimentRunner 與 Reporter

**風險**：拓樸策略誤接導致可行配置縮減

**回滾點**：比對每 pattern 的成功 config 數與基線 RepairReport.txt

---

### Phase 5：統一 CLI 與報表層
**目標**：整合 SharedLine / RedundantRate / basicPEarray 成共用 pipeline + mode 差異

**涉及檔案**：
- `SharedLine.cpp` (新增 mode selector)
- `RedundantRate.cpp` (重構)
- `basicPEarray.cpp` (重構)
- 新增：`inc/Reporter.hpp`、`src/Reporter.cpp`
- Makefile (統一編譯規則)

**流程**：
1. 新增 `Reporter` 類別，支援多種輸出格式 (txt/rpt/csv)
2. SharedLine.cpp main：
   - 參數 `--mode {baseline | redundant | compare}` 選擇策略
   - 參數 `--format {txt | rpt | csv}` 選擇輸出
   - 統一呼叫 ExperimentRunner + Reporter
3. 保留舊二進制名稱別名（sl_b → SharedLine --mode baseline）
4. 移除重複的 main 函數

**風險**：舊腳本相容性破壞

**回滾點**：保留舊 binary 名稱與參數別名一版，新增 `--legacy` flag 維持舊行為

---

## 6. 關鍵檔案與用途（12 個）

| 序號 | 檔案路徑 | 用途 | 目前狀態 |
|------|---------|------|---------|
| 1 | `SharedLine.cpp` | 4-subarray ring 主流程 + 報表 | 主程式，需重構 |
| 2 | `RedundantRate.cpp` | 四種配置比較 | 重複代碼，需合併 |
| 3 | `fault_generator/fault_generator.cpp` | Fault pattern 產生 | 穩定，最少改動 |
| 4 | `inc/Fault.hpp` + `src/Fault.cpp` | Fault 資料結構與分類 | 需拆分為 DTO + Service |
| 5 | `inc/RECAM_PE.hpp` + `src/RECAM_PE.cpp` | 單 PE 求解介面 | 需統一介面定義 |
| 6 | `inc/SolGenerator.hpp` + `src/SolGenerator.cpp` | Solution 列舉與驗證 | 需轉為 pure service |
| 7 | `inc/RECAM_CAM.hpp` + `src/RECAM_CAM.cpp` | Address/Hybrid CAM 與 must-rule | 需去掉 debug I/O |
| 8 | `inc/RECAM_bufferCAM.hpp` + `src/RECAM_bufferCAM.cpp` | Buffer CAM 特殊邏輯 | 需確認依賴 |
| 9 | `inc/FourWayPE.hpp` | 4-subarray 配置容器 | 需提供清晰介面 |
| 10 | `inc/ResSpareLines.hpp` | Ring 共享 spare 規則 | 需提供清晰介面 |
| 11 | `Makefile` | 編譯規則 | 需統一，減少重複 |
| 12 | `analyze_*.sh` | 批次執行腳本 | 需對應新 CLI 參數 |

---

## 7. 模組化後的架構圖

```
┌─────────────────────────────────────────────────────────────┐
│ CLI/App 層                                                   │
│  └─ SharedLine.cpp (unified main + mode selector)           │
└────────────┬────────────────────────────────────────────────┘
             │
┌────────────┴────────────────────────────────────────────────┐
│ Orchestrator 層                                              │
│  └─ ExperimentRunner (批次迴圈、統計彙總)                    │
└────────────┬────────────────────────────────────────────────┘
             │
      ┌──────┴──────────────┬──────────────┐
      │                     │              │
┌─────┴──────────┐ ┌──────┴─────────┐ ┌─┴──────────────┐
│ Ingestion 層   │ │ Strategy 層    │ │ Reporter 層    │
│ FaultLoader    │ │ Strategy       │ │ Reporter       │
│ FaultList DTO  │ │ Enumerator     │ │ CsvExporter    │
└─────┬──────────┘ └──────┬─────────┘ └─┬──────────────┘
      │                   │            │
      │        ┌──────────┴──────┐     │
      │        │                 │     │
      │  ┌─────┴────────────┐ ┌──┴───┐ │
      │  │ Solver Core 層   │ │Domain│ │
      │  │ RECAM_PE         │ │Layer │ │
      │  │ CAM/bufferCAM    │ └──────┘ │
      │  │ SolGenerator     │          │
      │  └─────┬────────────┘          │
      │        │                       │
      └────────┴───────────────────────┘
            (Data flow)
```

---

## 8. 成功標準與驗證清單

### Baseline 驗證 (Phase 1)
- [ ] 記錄現有 `sl_b` 與 `rdr_b` 的 repair rate 輸出
- [ ] 將輸出保存為 golden files

### 各 Phase 驗證
- [ ] Phase 2：FaultLoader 讀出的 pivot/nonpivot 數與 fault_map.txt 一致
- [ ] Phase 3：RECAM_PE::solve() 結果與舊 RECAM_PE.cpp 完全相同
- [ ] Phase 4：ExperimentRunner 統計的成功 pattern 數與基線相同
- [ ] Phase 5：新 SharedLine --mode 輸出與舊二進制輸出一致（csv/txt 格式可不同，但數值相同）

### 代碼質量指標
- [ ] 模組間依賴向下（無循環依賴）
- [ ] 主程式行數 < 100（不含註解）
- [ ] 每個模組職責單一，類別 < 500 行
- [ ] 新增 unit test（至少涵蓋 Domain DTO、FaultLoader、StrategyEnumerator）

---

## 9. 延伸考慮

### Option A：先固定 4-way 單環
- 優點：最小改動、驗證快速
- 缺點：無法擴充為 N-way

### Option B：預留 N-way 拓樸抽象
- 優點：未來易於擴充
- 缺點：目前代碼複雜度提升

### 建議：選 Option A，Phase 4+ 再預留 Strategy pattern 插件點

---

## 10. 下一步行動

1. **確認方向**：選擇「最小風險分層」(5 Phase) 或「一次性完整模組化」
2. **Baseline 拍攝**：執行 Phase 1，記錄 golden files
3. **Phase 2 開始**：抽離 FaultLoader，測試一致性
4. **迭代進行**：每 Phase 完成後執行回滾點驗證
5. **反饋循環**：若出現意外漂移，回到上一 Phase 檢查

---

*最後更新：2026-06-08*
*方案版本：v1.0 - 初次規劃*
