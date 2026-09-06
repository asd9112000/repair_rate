# 文件索引與整理規則

> 文件狀態：Current
> 適用範圍：cross-cutting
> 建立時間：2026-09-02T00:00:00+08:00
> 最後修改時間：2026-09-07T00:00:00+08:00
> 本文件權威主題：docs 內文件分類、責任分工與參數位置索引

本文件只管理 `docs/` 內的文本。它是導航與責任分工文件，不取代任何架構或行為規格。

## 文件責任分工

| 文件 | 狀態 | 適用範圍 | 唯一責任 |
|---|---|---|---|
| [README.md](README.md) | Current | cross-cutting | 入口、閱讀順序、快速開始 |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Current | cross-cutting | 架構分層、resource ownership、Legacy/Hierarchical 邊界 |
| [RECAM_SPEC.md](RECAM_SPEC.md) | Current | recam-core | RECAM fault、CAM、matrix、repair semantics、核心驗證契約 |
| [HIERARCHICAL_RECAM.md](HIERARCHICAL_RECAM.md) | Current | hierarchical-device | WoW hierarchy、CLI、輸入、輸出與 device-level 指標 |
| [EXPERIMENTS.md](EXPERIMENTS.md) | Current | cross-cutting | 實驗方法、可重現性、sweep、plotting、結果比較規則 |
| [REPORTS.md](REPORTS.md) | Current | cross-cutting | `reports/` scope、run artifact 與輸出位置規則 |
| [SRAM_RECAM_ARCHITECTURE_GUIDE.md](SRAM_RECAM_ARCHITECTURE_GUIDE.md) | Current | sram-group | SRAM-RECAM storage、search、latency、area、runtime variants |
| [handoff/MODERATE_REPAIR_STUDY_HANDOFF.md](handoff/MODERATE_REPAIR_STUDY_HANDOFF.md) | Handoff | moderate-study | 特定研究階段的狀態、結果摘要與交接 |
| [handoff/EXPERIMENT_WORKFLOW_HANDOFF.md](handoff/EXPERIMENT_WORKFLOW_HANDOFF.md) | Handoff | experiment-workflow | C++ core、Python runner、manifest 與 scope-first reports workflow 的交接 |
| [archive/](archive/README.md) | Archive | historical | 已取代規格與歷史設計快照 |

## 單一真實來源規則

同一個定義只能在一份 Current 文件中完整維護。其他文件只保留必要摘要，並連回權威文件。

- RECAM 共通規則以 `RECAM_SPEC.md` 為準。
- 整體 ownership 與架構邊界以 `ARCHITECTURE.md` 為準。
- Hierarchical 專屬參數、CLI 與 CSV 欄位以 `HIERARCHICAL_RECAM.md` 為準。
- 實驗流程與比較限制以 `EXPERIMENTS.md` 為準。
- `reports/` 的 scope 定義與新輸出位置以 `REPORTS.md` 為準。
- SRAM 文件中的 RECAM 基礎規則不得另立版本；該文件只補充 SRAM 特有內容。
- Handoff 是時間點快照，不取代 Current 文件。
- Archive 只供追溯，不可作為目前操作依據。

## 硬體架構參數位置

目前採用的 WoW-v1.0 hierarchical baseline 集中記錄在
[HIERARCHICAL_RECAM.md](HIERARCHICAL_RECAM.md) 的 **第 2 節 Reference architecture**。

| 參數 | 目前記錄值 | 文件位置 |
|---|---:|---|
| DRAM/device size | 32 Gbit / 4 GiB | `HIERARCHICAL_RECAM.md` §2，`Total capacity` |
| Channel-like domains | 8 | `HIERARCHICAL_RECAM.md` §2 |
| Banks | 4 banks/domain；32 banks/device | `HIERARCHICAL_RECAM.md` §2 |
| Bank size | 1 Gbit/bank（由 64 groups × 16 Mbit 推導） | `HIERARCHICAL_RECAM.md` §2，`Capacity check` |
| Repair group size | 16 Mbit/group | `HIERARCHICAL_RECAM.md` §2 |
| Subarrays | 4 subarrays/group | `HIERARCHICAL_RECAM.md` §2 |
| Subarray size | 512 rows × 8192 cell columns = 4 Mbit/SA | `HIERARCHICAL_RECAM.md` §2，`Capacity check` |

歷史版本另保存在
[archive/WoW_DRAM_REFERENCE_ARCHITECTURE_v1.md](archive/WoW_DRAM_REFERENCE_ARCHITECTURE_v1.md)，
但該文件不是目前參數的權威來源。

## 日期規則

每份 Markdown 文件標題後都必須保留以下四項：

```text
文件狀態
適用範圍
建立時間
最後修改時間
```

日期使用 `YYYY-MM-DDTHH:MM:SS+08:00`。`建立時間` 一經確認不可修改；若歷史文件無法
由 Git 或其他紀錄確認，使用 `Unknown`，不可自行推測。只有實質內容修改才更新
`最後修改時間`；只有檢視而未修改內容時，不更新該欄位。

## 後續文本整理順序

1. 先維持本索引中的責任分工。
2. 清除其他文件中重複的完整公式、CLI 表格與硬體參數。
3. 改為短摘要加連結，並標示「以某文件為準」。
4. 將 RECAM 規格中的驗證內容與核心行為分開整理時，保留原有章節連結。
5. 將 SRAM 文件內的內容明確標成「目前模型」或「研究提案」。
6. 修正文件連結後，再考慮檔名或目錄搬移。
