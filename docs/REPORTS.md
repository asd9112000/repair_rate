# 實驗輸出與 Scope 分類

> 文件狀態：Current
> 適用範圍：cross-cutting
> 建立時間：2026-09-07T00:00:00+08:00
> 最後修改時間：2026-09-07T00:00:00+08:00
> 本文件權威主題：`reports/` 目錄、輸出 scope 與 run artifact 保留規則

## 1. 目錄規則

新實驗一律寫入下列 scope-first 目錄；已存在的舊目錄保留原位，不以這次整理為由
搬移或重新解讀。

```text
reports/
├── legacy/<experiment>/<run-id>/
├── group/<experiment>/<run-id>/
├── device/<experiment>/<run-id>/
└── studies/<study>/<run-id>/
```

一個可重現 run 應將原始 simulator CSV 放在 `raw/`，將可由原始資料重建的合併 CSV
放在 `derived/`，圖片放在 `plots/`，並保留 `manifest`、完整 command 與 log。
既有輸出格式若已有不同的子目錄命名，先維持相容；新 wrapper 應採這個慣例。

## 2. Scope 定義

| Scope | 代表 simulator／用途 | 結果語意 |
|---|---|---|
| `legacy` | `SharedLine`、`SharedLine_SRAM`、`RedundantRate` | 原始 paper 或固定-sharing baseline；不具 hierarchical device ownership。 |
| `group` | `DynamicSpareSharing`、`DynamicSpareSharing_SRAM_RECAM` | 一個獨立 4-SA repair group 的 Monte Carlo／policy analysis；不會跨 groups 保留 online CAM occupancy。 |
| `device` | `HierarchicalRECAM`、canonical B0–B3 | 一個 WoW-v1.0 device 的 hierarchical model；Tier-2 requests 競爭同一有限、device-wide global CAM pool。 |
| `studies` | moderate repair 等編排式研究 | 不是第四種 simulator scope；它保存一個研究的設定、policy selection 與 scope-separated outputs。其 `group/`、`sram/`、`device/` 子輸出仍各自保持原本語意。 |

因此 `group` repair rate 不可作為 `device` canonical B0–B3 的其中一列，也不可用
`studies/.../combined/` 把兩者平均或排序。`combined/` 僅可存 cross-scope 的 corpus
對應、invariant 驗證、選擇決策與索引。

## 3. 新輸出位置

| 實驗 | 新預設位置 |
|---|---|
| SharedLine | `reports/legacy/sharedline/` |
| SharedLine SRAM | `reports/legacy/sharedline_sram/` |
| RedundantRate | `reports/RedundantRate/`（legacy compatibility output） |
| Dynamic Spare Sharing | `reports/group/dynamic_spare_sharing/<run-id>/` |
| SRAM-RECAM group comparison | `reports/group/sram_recam/` |
| Hierarchical RECAM | `reports/device/hierarchical_recam/` |
| Canonical B0–B3 | `reports/device/canonical_four/<run-id>/` |
| Moderate repair study | `reports/studies/moderate_repair/<profile-or-run-id>/` |

每份 hierarchical device CSV 仍須保留 `architecture_version`、`simulator_version` 與
`cam_scope`；scope-first 路徑是輔助導航，不可取代 CSV/manifest 中的版本 metadata。
