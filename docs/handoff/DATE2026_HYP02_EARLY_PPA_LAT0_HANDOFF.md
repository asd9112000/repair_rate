# DATE2026 HYP02 EARLY PPA 與 LAT0 交接文件

> 文件狀態：Handoff
> 適用範圍：DATE2026 2×2 directional HYP02 hardware / PPA / LAT0 前置作業
> 建立時間：2026-09-23T06:00:00+08:00
> 最後修改時間：2026-09-23T06:00:00+08:00
> 本文件權威主題：近期 HYP02 EARLY/GROUP 硬體比較、決策歷程、可重現證據與後續交接；架構與實驗一般規則仍以 `ARCHITECTURE.md`、`EXPERIMENTS.md` 為準。

## 1. 一句話狀態

目前 thesis-primary 硬體比較已固定為 **HYP02-compatible EARLY** 對
**HYP02 GROUP/GLOBAL**：兩者都使用 H=7 / 204-bit shared analyzer 與同一個
static 81-path resource abstraction。EARLY 已完成 RTL、獨立 oracle、DC PPA、
final archive 與 thesis area table 更新；下一步是只做
`DATE2026-LAT0-FINAL-HYP02-RTL-CONTRACT-AUDIT`，**尚未開始 latency 統計或 repair-rate sweep**。

```text
工作目錄: /home/asd9112000/repair_rate_date2026_canonical
分支: integration/date2026-directional-canonical-v1
目前 HEAD: d1c7e6d53b59b443c3b3f2da5a7720a23e216744
RTL commit: 46e1d3a82bf948dc3e357823cbc50869be964023
封存/文件 commit: d1c7e6d53b59b443c3b3f2da5a7720a23e216744
工作樹（建立本文件前）: CLEAN
```

## 2. 最重要的語意決策

### 已採用的 hardware contract

```text
HYP02_STATIC_81_PATH_CONTRACT
candidate = {slot-valid, PatternID}
resource legality = final HYP02 GROUP RTL 的固定 81 個合法 slot paths
```

- slot 0/1/2/3 分別是 local / release / borrow / release+borrow。
- SA 順序固定為 A → B → C → D；每個 SA 的 slot 優先序固定為 `1,0,3,2`
  （`R,L,RB,B`）。
- EARLY 只接受「已選 prefix 加上此 candidate 後，仍至少存在一條 81-path completion」
  的第一個 valid slot；接受後立即 commit，後續失敗沒有 rollback。
- PatternID 是 shared analyzer 的不變 4-bit 輸出；ConfigID 為 A/D `0,4,5,6`、
  B/C `0,1,2,3` 對應 slot `0,1,2,3`。

### 明確未採用的語意

下列皆**不是**本次 matched PPA 的 hardware contract：

- `PhysicalResourceLedger` 的 `usedRows` / `usedColumns` physical-demand 模型；
- Model-B2 shared-collector 的 H=11 / 四摘要擴張；
- 舊的固定 four-token controller；
- 推導新的 donor/borrow demand model。

這是研究邊界決定，不表示 C++ ledger 是錯的。它與 frozen HYP02 RTL 使用不同的
candidate/resource input contract，尚未被證明等價。

## 3. 近期時間線與做過的事

| 時間（+08:00） | Commit | 工作 | 結果 / 決策 |
| --- | --- | --- | --- |
| 2026-09-23 01:58 | `b95030f` | shared collector simulator 對齊 | 完成 Model-B2 方向性 EARLY 的 simulator alignment。 |
| 2026-09-23 02:18 | `2fe7ce1` | C1R4 static-action closure | 補強舊 Model-B2 路線的 directed evidence。 |
| 2026-09-23 03:43 | `7fdaf0a` | corrected Model-B2 EARLY RTL | 完成 H=11 / 4-summary EARLY；後來降為 exploratory。 |
| 2026-09-23 04:02 | `b5d02ac` | B2 PPA archive + compatibility audit | 發現 Model-B2 與 archived HYP02 GROUP 不可直接當 matched PPA。 |
| 2026-09-23 04:53 | `1e64201` | area inversion RCA | 證明 H=11 frontend / summary boundary 是 B2 面積反轉主因。 |
| 2026-09-23 05:14 | `54e74a0` | Path-C semantic blocker | 暫停於「static HYP02 vs physical ledger」衝突，要求人類決策。 |
| 2026-09-23 05:42 | `46e1d3a` | HYP02 EARLY RTL + oracle + DC scripts | 實作最小 static-81-path immediate EARLY。 |
| 2026-09-23 05:43 | `d1c7e6d` | final archive / PPA / thesis table | 完成新 archive 和 matched thesis PPA 表。 |

人類後續選擇 Path-C 的 Option 1：以 final HYP02 GROUP RTL 的 static 81-path universe
作為最終 hardware boundary。因此 `54e74a0` 的「blocked」是歷史記錄，不是目前狀態。

## 4. 最新實驗與 PPA 數據

所有下列 PPA 數字採相同條件：Synopsys DC W-2024.09-SP2、TSMC018 `slow.db`、slow
corner、20.0 ns clock、zero I/O delay、NAND2X1 `9.979200 um2` 正規化。DC 對要求的
`compile -map_effort low` 發出 OPT-1303，實際採用 effective/default medium effort；
兩個主比較點均以此相同 methodology 報告。

| 架構 | Hardware semantic boundary | Area (um2) | GE | Comb / Seq area (um2) | Critical / WNS / TNS (ns) | 用途 |
| --- | --- | ---: | ---: | --- | --- | --- |
| HYP02 GROUP/GLOBAL | H=7 static 81-path、deferred atomic select | 106341.682630 | 10656.33 | 98498.0313 / 7843.6514 | 19.82 / 0.00 / 0.00 | frozen mother |
| HYP02-compatible EARLY | H=7 static 81-path、immediate prefix select | 78216.970295 | 7838.00 | 75748.781464 / 2468.188831 | 19.72 / +0.01 / 0.00 | thesis-primary matched EARLY |
| corrected Model-B2 EARLY | H=11 / four expanded summaries / physical-demand-related route | 200824.749004 | 20124.33 | 198107.0802 / 2717.6688 | 19.72 / 0.00 / 0.00 | exploratory only |

HYP02 EARLY 比 HYP02 GROUP 少 `28124.712335 um2`，即 `-26.447496%`。EARLY 的 hierarchy
為 shared analyzer `73154.1894 um2` 與 core `5046.1489 um2`；GROUP 的相對 core
為 `33180.8404 um2`。EARLY critical path 為
`core/sa_q_reg[1] → core/selected_pattern_flat_o_reg[15]`。

## 5. 驗證證據與可重現入口

新 EARLY 的 independent oracle 不呼叫 RTL legality helper、`PhysicalResourceLedger`
或 Model-B2 state。它枚舉 256 個 slot tuple，依四個 HYP02 implication 過濾為 81 paths，
再依 prefix completion 判斷合法性。

```text
HYP02_LEGAL_PATH_COUNT=81
EXHAUSTIVE_VALIDITY_MAPS=65536
PREFIX_LEGALITY_MISMATCHES=0
RANDOMIZED_VECTORS=1000
1000_VECTOR_MISMATCHES=0
HYP02_SHARED_ANALYZER_ALL_LOCAL=PASS
HYP02_SHARED_ANALYZER_OVERFLOW_REJECTION=PASS
```

已實際執行的命令：

```bash
scripts/simulation/run_verilator_test.sh \
  recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core \
  rtl/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core.sv \
  tb/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core_test.cpp

scripts/simulation/run_verilator_test.sh \
  recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top \
  rtl/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top.sv \
  tb/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top_test.cpp \
  rtl/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core.sv \
  rtl/recam/recam_shared_config_analyzer.sv

make test_p3_synb_hyp02_exact_81path_proof
```

DC 重跑入口：

```bash
scripts/synthesis/run_recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_dc.sh
```

新 final archive：
`dss_final/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch/`。
shared analyzer 的 SHA-256 為
`b4aa07117af410e84c809ae833f7e926ef440a880523c78e11cd1a7df3dd5b6c`，與 GROUP mother
byte-identical。

## 6. 已知問題、限制與避免踩雷

1. **HYP02 RTL 與 C++ physical ledger 尚未橋接。** 不得把既有
   `DynamicRepairSimulator` / `PhysicalResourceLedger` repair-rate 或 latency 結果標成
   HYP02 EARLY 硬體結果。若要做 matched repair-rate，必須新增並驗證 HYP02 static policy；
   本交接不授權該工作。
2. **Model-B2 不可回到主表。** compatibility audit 有 1,000 vectors 中 259 個差異、
   41 個 repairability 差異，且 Vector216 GROUP difference 為 YES。Model-B2 的
   `200824.749004 um2` 只能放 exploratory 區。
3. **Vector216 的兩種意義不可混用。** 新 RTL oracle 的 `0x5a6c` 是 static
   slot-map boundary fixture，PASS；歷史 fault-level Vector216 是 load=16/index=216，
   在 GROUP 與 full Model-B2 都 repair、但 ConfigID 不同。歷史 corpus 沒有可直接送入
   H=7 top 的 analyzer input fixture，故不可宣稱已完成 fault-level H=7 RTL replay。
4. **原始 DC reports 保留 tool whitespace。** archive 的 `.rpt` 是逐字 copy，
   所以完整 `git diff --check` 會對其 trailing whitespace 提醒；手寫 RTL/文件的
   staged check 已通過。不要為消除 warning 改寫原始報告。
5. **metadata 的 Git head 早於 RTL commit。** synthesis 在 `46e1d3a` commit 前以同一份
   RTL 執行，所以 archive `synthesis/metadata.txt` 的 `GIT_HEAD=54e74a0...`。追溯合成
   source 時以 archive 的 three source SHA-256、`source_manifest.txt` 和 RTL commit
   `46e1d3a` 為準；source content 已 hash-verified。
6. **`dss_final/` 不可直接開發。** 它是 packaged archive；新功能只可在 `rtl/` / `tb/`
   開發，通過 verification 與 synthesis 後建立新 sibling archive。
7. **目前開啟的 `scripts/run_date2026_group_sweep.sh` 不是下一步。** 它是 frozen R3
   group-scope repair-rate adapter。LAT0 前禁止啟動它的 long/formal sweep。

## 7. 下一位接手者的最小工作清單

1. 先確認 `git status --short` 為空，讀本文件與
   `docs/dss_execution/FINAL_EARLY_PATH_C2_HYP02_MATCHED_IMPLEMENTATION_AND_PPA.md`。
2. 僅啟動 `DATE2026-LAT0-FINAL-HYP02-RTL-CONTRACT-AUDIT`：逐一讀 final HYP02 EARLY
   與 GROUP RTL，抽取 start、candidate evaluation、commit、failure、done 的
   cycle-accurate contract。
3. 可重用既有 T1 / Phase4I 的 event queue、BIST timing、fault timestamp mapping、
   statistics 和 RTL/C++ trace comparison；**不可**未驗證地沿用舊 V2 EARLY/GROUP latency
   constants。
4. LAT0 結果需先產生 parameter contract 供 human review；不要先跑統計 latency simulation、
   repair-rate long sweep、1×4 或 RS=CS=3 實驗。
5. 若有人要求把 C++ physical ledger 結果與 HYP02 PPA 合併，先停止並要求明確的
   semantics bridge / human authorization。

## 8. 主要文件導航

- matched implementation/PPA：`docs/dss_execution/FINAL_EARLY_PATH_C2_HYP02_MATCHED_IMPLEMENTATION_AND_PPA.md`
- initial semantic conflict：`docs/dss_execution/FINAL_EARLY_PATH_C_GROUP_COMPATIBLE_IMPLEMENTATION_AND_PPA.md`
- Model-B2 vs GROUP audit：`docs/dss_execution/GROUP_MODEL_B2_COMPATIBILITY_AUDIT.md`
- area inversion RCA：`docs/dss_execution/FINAL_EARLY_PPA_RCA1_AREA_INVERSION_ROOT_CAUSE_AUDIT.md`
- thesis table：`docs/dss_execution/THESIS_AREA_FASTTRACK_TABLE.md`
- final archive index：`dss_final/INDEX.md`
- general experiment workflow：`docs/handoff/EXPERIMENT_WORKFLOW_HANDOFF.md`

## 9. 交接完成條件

```text
HYP02_PRIMARY_PPA: COMPLETE
HYP02_EARLY_FINAL_ARCHIVE: COMPLETE
LAT0_STATISTICAL_SIMULATION: NOT_STARTED
REPAIR_RATE_HYP02_POLICY: NOT_IMPLEMENTED
NEXT_AUTHORIZED_PHASE: DATE2026-LAT0-FINAL-HYP02-RTL-CONTRACT-AUDIT
```
