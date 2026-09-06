# 歷史文件

> 文件狀態：Archive
> 適用範圍：cross-cutting
> 建立時間：Unknown
> 最後修改時間：2026-09-02T00:00:00+08:00
> 本文件權威主題：歷史文件索引與現行替代文件對照

本目錄保存已被整合或取代的設計草稿與規格快照，供需求追溯使用。它們可能描述
尚未實作的目標、舊 CLI，或把 group-level CAM accounting 與 device-level global
CAM reuse 混在一起，因此不應作為目前程式的操作依據。

目前有效的入口：

- [../README.md](../README.md)：文件索引與快速開始；
- [../DOCUMENT_INDEX.md](../DOCUMENT_INDEX.md)：文件責任、狀態與參數位置；
- [../ARCHITECTURE.md](../ARCHITECTURE.md)：目前 repository 架構；
- [../HIERARCHICAL_RECAM.md](../HIERARCHICAL_RECAM.md)：device-level simulator；
- [../EXPERIMENTS.md](../EXPERIMENTS.md)：實驗、sweep 與 plotting；
- [../RECAM_SPEC.md](../RECAM_SPEC.md)：RECAM solver 詳細規格；
- [../SRAM_RECAM_ARCHITECTURE_GUIDE.md](../SRAM_RECAM_ARCHITECTURE_GUIDE.md)：
  SRAM-RECAM 詳細規格。

## 封存對照

| 歷史文件 | 現行替代文件 |
|---|---|
| `PROJECT_CONTEXT.md`、`SPECIFICATION.md` | `../ARCHITECTURE.md`、`../README.md` |
| `DYNAMIC_SPARE_SHARING.md`、`DYNAMIC_SPARE_SHARING_POLICY.md` | `../EXPERIMENTS.md` |
| `PLOT_GENERATION.md` | `../EXPERIMENTS.md` |
| `CODEX_HIERARCHICAL_RECAM_SIM_SPEC_v2.md` | `../HIERARCHICAL_RECAM.md` |
| `WoW_DRAM_REFERENCE_ARCHITECTURE_v1.md` | `../HIERARCHICAL_RECAM.md` 的 reference architecture；原文保留供追溯 |

封存不代表內容錯誤；它只表示該文件不再是 current source of truth。
