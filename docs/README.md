# repair_rate 文件入口

> 文件狀態：Current
> 適用範圍：cross-cutting
> 建立時間：Unknown
> 最後修改時間：2026-09-07T00:00:00+08:00
> 本文件權威主題：文件索引、閱讀順序與文件管理規則

本專案以 C++17 模擬 RECAM、subarray spare-line sharing，以及 logic-die
BIRA/CAM reuse。現在同時保留兩類架構：

- **Legacy／group-level 路徑**：重現原始 RECAM 與四 subarray sharing 實驗。
- **Hierarchical-RECAM-v2 路徑**：依 WoW-v1.0 hierarchy，由 logic die 上的
  BIRA engine 跨 repair groups 重用 scratch，所有 Tier-2 repairs 競爭同一個
  device-wide online CAM pool。

兩類結果的硬體 ownership 不同，不能放在同一組 repair-rate 比較中。

## 建議閱讀順序

完整的文件責任、狀態與硬體參數位置，見 [DOCUMENT_INDEX.md](DOCUMENT_INDEX.md)。

| 文件 | 用途 |
|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | repository 現況、程式分層、resource ownership 與各執行入口的邊界 |
| [HIERARCHICAL_RECAM.md](HIERARCHICAL_RECAM.md) | `HierarchicalRECAM` 架構、完整 CLI、輸入、輸出與指標解讀 |
| [EXPERIMENTS.md](EXPERIMENTS.md) | 實驗設計、dynamic sharing policies、sweep、plotting 與可重現性 |
| [REPORTS.md](REPORTS.md) | `reports/` scope 分類、輸出位置與 artifact 保留規則 |
| [handoff/MODERATE_REPAIR_STUDY_HANDOFF.md](handoff/MODERATE_REPAIR_STUDY_HANDOFF.md) | 本次 moderate smoke／screen／confirm 的目的、矩陣、code flow、結果與後續交接 |
| [RECAM_SPEC.md](RECAM_SPEC.md) | paper-faithful RECAM fault classification、CAM、matrix 與 solver 驗證細節 |
| [SRAM_RECAM_ARCHITECTURE_GUIDE.md](SRAM_RECAM_ARCHITECTURE_GUIDE.md) | SRAM-RECAM search、latency、area 與 runtime variants |

已被合併的舊規格保留在 [archive/README.md](archive/README.md)，僅供追溯，
不再作為目前操作說明。

文件分類、權威來源與硬體參數位置請先看
[DOCUMENT_INDEX.md](DOCUMENT_INDEX.md)。

原始論文 PDF：`Yoon-2025-A-built-in-self-repair-with-maximum.pdf`。

## 可執行程式

| 程式 | 層級 | 用途 |
|---|---|---|
| `basicPEarray` | 單 PE／legacy | 基本 RECAM solver 與除錯 |
| `RedundantRate` | legacy pattern | 比較 `(Rs,Cs)` 及降低一條 R/C 的配置 |
| `SharedLine`、`SharedLine_3way` | legacy 4-PE | 固定 Ring sharing baseline |
| `SharedLine_SRAM` | legacy 4-PE | remap table 與 latency 輸出 |
| `DynamicSpareSharing` | 4-SA group | configurable sharing policy、Monte Carlo 與 CSV；不累積跨 group online CAM occupancy |
| `DynamicSpareSharing_SRAM_RECAM` | 4-SA group | 比較 Serial／Chunked／Wide SRAM-RECAM search policies |
| `HierarchicalRECAM` | device | WoW hierarchy、shared BIRA scratch、finite global online CAM pool |

## 快速開始

需求：GNU Make、支援 C++17 的 `g++`；plotting 額外需要
`requirements-plot.txt` 中的 Python 套件。

```bash
# 建置全部 simulator 與工具
make all

# 建置並執行 hierarchical v2 baseline
make hierarchical_recam_b
./build/bin/HierarchicalRECAM 2 2 \
  --groups 32 \
  --fault-count 8 \
  --topology edge \
  --shared-rows 1 \
  --shared-columns 0 \
  --bira-engines 1 \
  --output-dir reports/device/hierarchical_recam/example
```

輸出：

```text
reports/device/hierarchical_recam/example/
├── device_summary.csv
├── groups.csv
└── bira_engines.csv
```

完整參數與欄位定義見 [HIERARCHICAL_RECAM.md](HIERARCHICAL_RECAM.md)。

## 測試

```bash
# Hierarchical v2 規格測試
make test_hierarchical_recam
make test_hierarchical_fault_models

# SRAM FIFO delay、address/BIST boundary、B0–B3 regression
make test_dynamic_spare_sharing_layout
make test_layout_2x2_regression
make test_sram_fault_collection_fifo
make test_fault_address_bist
make test_canonical_experiment
make test_canonical_sweep

# 全部測試
make test
```

Hierarchical 測試涵蓋：

- CAM hardware 不按 8192 SAs 倍增；
- BIRA scratch 在 groups 之間清除並重用；
- persistent CAM assignments 不因 scratch clear 消失；
- capacity contention 與 overflow；
- full-address tag disambiguation；
- 256-bit same-word deduplication；
- Tier-1 sharing 降低 Tier-2 CAM demand。

SRAM FIFO regression 另以手算 golden 固定：`Rs=Cs=2`、shadow + chunked P=2、
三個依序新增的 physical pivot faults 採 occupied-prefix rounds `{0,1,1}`，因此
collection work=5、solution ready=6、after-BIST latency=1。
address/BIST test 固定 external word+syndrome 到 internal physical cell 的轉換，
以及 A→B→C→D serial scan 邊界。

## 結果版本規則

Hierarchical v2 CSV 必須含：

```text
architecture_version = WoW-v1.0
simulator_version = Hierarchical-RECAM-v2.1
cam_scope = GLOBAL_LOGIC_DIE
```

缺少這些欄位、或由 `SharedLine`／`DynamicSpareSharing` 產生的結果，應視為
legacy/group-level 結果。舊結果若曾把 CAM capacity 對每個 SA 或 group
獨立重置，必須重新執行後才能用於 global-CAM repair-rate 分析。

## 目錄

```text
.
├── *.cpp                 # simulator／tool 入口
├── inc/                  # 公開資料結構與介面
├── src/                  # solver、scheduler、resource model 與 reporter
├── tests/                # C++ 與 Python tests
├── fault_generator/      # fault 產生與格式說明
├── scripts/              # batch experiment scripts
├── docs/                 # 目前文件、handoff 與 archive
├── build/                # 可重建產物；make clean 會移除
└── reports/              # 實驗輸出；make clean 不會移除
```
