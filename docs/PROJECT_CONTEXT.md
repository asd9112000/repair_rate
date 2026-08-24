# 研究背景

## 1. 研究動機

記憶體或運算陣列中的 Processing Element（PE）可能因製程、老化或操作環境產生 row／column fault。若每個 PE 都配置足以涵蓋最壞情況的專屬備援列與欄，硬體面積與控制成本會增加；若配置不足，則 fault pattern 無法被完整映射，良率與可用性下降。

本專案以模擬方式研究「有限備援資源如何配置及共享」對 repair rate 的影響。核心問題是：在相同 fault patterns 下，四個相鄰子陣列是否能藉由 Ring 形式共享 spare lines，以比固定、互不共享的 `(Rs,Cs)` 配置修復更多 pattern；並進一步觀察降低資源組合或改用 SRAM remap table 後的差異。

## 2. 問題定義

令：

- `Rs`：基準 spare row 數量。
- `Cs`：基準 spare column 數量。
- fault pattern：某個 Logic Unit／layer 中，由 `(Bank, Row, Col)` 座標構成的一組 faults。
- repair success：存在至少一個 row／column 選擇方案，能覆蓋該 PE 的所有必要 fault，且共享資源占用不超出 Ring 可用量。
- repair rate：成功修復 pattern 數除以全部 pattern 數。

單 PE 配置比較的核心集合為：

```text
(Rs, Cs)
(Rs-1, Cs)
(Rs, Cs-1)
(Rs-1, Cs-1)
```

SharedLine 實驗則讓四個 PE 位於 2×2 位置：

```text
PE0 ─ PE1
 │      │
PE2 ─ PE3
```

目前程式以四段共享資源形成固定 Ring。PE0、PE3 的候選配置偏向共享 row；PE1、PE2 的候選配置偏向共享 column。對每一個 pattern，程式列舉四個 PE 的候選配置，先驗證各 PE 的 RECAM solution，再依固定 PE 順序扣除專屬與共享資源；只要找到至少一組四 PE 皆成功的配置，該 pattern 即計為修復成功。

## 3. 研究比較組

| 實驗 | 比較目的 | 目前入口 |
| --- | --- | --- |
| 單 PE baseline | 驗證 RECAM 求解與基本修復率 | `basicPEarray.cpp` |
| 四配置冗餘比較 | 比較降低一條 row／column 後的可修復性 | `RedundantRate.cpp` |
| 四子陣列 Ring | 評估共享 spare line 相對固定 `(Rs,Cs)` 的提升 | `SharedLine.cpp` |
| 3-way | 排除一種高資源候選後，觀察策略空間縮減影響 | `SharedLine_3way.cpp` |
| SRAM | 產生 remap table，將 latency 納入結果表示 | `SharedLine_SRAM.cpp` |

## 4. Fault 模型

`fault_generator` 以 Logic Unit 和 stack layer 產生 fault patterns。每個 fault 的資料欄位為：

```text
LogicUnitID LayerID BankID Row Col
```

目前位置範圍為 `1024 × 1024`，Bank ID 固定為 `0`。位置生成比例為：

- 20%：靠近既有 fault 的 cluster fault。
- 30%：與既有 fault 位於相同 row 或 column。
- 50%：全陣列隨機位置。

數量分布支援 `fixed`、`normal`、`extreme`。標準 generator 會在 `normal`／`extreme` 下重新分配，使每層總 fault 數維持 `fixed_faults × logic_units`；`fault_generator_rand_amount.cpp` 則允許各層總數浮動。完整定義見 [`fault_generator/README_fault_generator`](fault_generator/README_fault_generator)。

## 5. 求解概念

Fault 載入後會依 RECAM 規則分類為 pivot、non-pivot、buffer、overflow pivot 等集合。求解器將 faults 裝入 Address／Hybrid／Buffer CAM，建立 must-rule 與 fault analysis matrix，再由 `SolGenerator` 產生 row／column 選擇順序及對應矩陣，逐一判定是否存在有效 solution。

四 PE 實驗不只要求每個 PE 個別可解，也要求 `ResSpareLines` 的共享資源帳本能依 Ring 關係滿足四個 PE 的 row／column 使用量。因此研究結果同時受到以下因素影響：

- fault 數量與空間分布；
- pivot／non-pivot 分類及 CAM 容量；
- `(Rs,Cs)` 候選集合；
- Ring 的共享方向與共享線數；
- 四 PE 資源占用的檢查順序；
- SRAM 版本的 remap 與 latency 規則。

## 6. 評估指標

主要指標：

```text
RepairRate = |共享策略成功的 patterns| / |全部 patterns|
RepairRate_RECAM = |固定 (Rs,Cs) 成功的 patterns| / |全部 patterns|
```

建議衍生指標：

- 絕對提升量：`RepairRate - RepairRate_RECAM`。
- 相對提升率：`(RepairRate - RepairRate_RECAM) / RepairRate_RECAM`（baseline 非 0 時）。
- 每個 pattern 的成功配置數與配置分布。
- 不同 fault mode、fault 數、`Rs/Cs` 下的趨勢。
- SRAM 模式的 remap 數量與 latency 分布。

## 7. 實驗可重現性

研究結論應至少記錄：

- source revision／commit；
- 使用的執行入口；
- `Rs`、`Cs`、buffer 數與共享線數；
- generator 類型與完整參數；
- 亂數 seed；
- `faults.faults` 或其 checksum；
- pattern 總數；
- compiler、編譯旗標與平台；
- 輸出 report／CSV。

目前 Makefile 的 `gen_fault` 固定使用 `--fault_mode normal`，但沒有轉傳 seed；需要可重現結果時，應直接執行 generator 並指定 `--seed`，或在後續重構中把 seed 納入統一 CLI。

## 8. 研究範圍與限制

- 現況固定為四個 PE 的 2×2／Ring 模型，尚未抽象為任意 N-way topology。
- 共享策略是由候選配置與 `switch (PE_index)` 硬編碼，不是一般化的圖模型。
- `sharedLine_num` 與 buffer 數在主要 SharedLine 程式中固定為 1 與 2。
- repair rate 是演算法模擬結果，不直接等同於晶片實測良率。
- 報表目錄內的既有檔案只能代表當時輸入與程式版本；缺少完整 metadata 時，不宜跨版本直接比較。

## 9. 重構研究目標

兩份既有重構 prompt 內容相同，提出五階段方向：先固定 regression baseline，再依序抽離 Fault ingestion、Solver core、Spare strategy／Experiment runner、Reporter／統一 CLI。重構的研究原則是「演算法與數值結果先保持不變，再改善模組邊界」，每階段都應用相同 fault input 比對成功 pattern、配置與 repair rate。
