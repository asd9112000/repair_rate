# SRAM-RECAM 架構與演算法設計說明

> 文件狀態：Current
> 適用範圍：sram-group
> 建立時間：Unknown
> 最後修改時間：2026-09-02T00:00:00+08:00
> 本文件權威主題：SRAM-RECAM storage、search、latency、area 與 runtime variants
> 內容注意：本文件同時包含目前模型與研究提案；提案不可視為已實作規格

RECAM 的共通 fault、CAM、matrix 與 repair semantics 以
[RECAM_SPEC.md](RECAM_SPEC.md) 為準；本文件只補充 SRAM 儲存、搜尋、延遲、面積與
runtime 組織的差異。

`--solution-take legacy|early|group` 與本文件的 SRAM search policy 是正交設定：
前者決定四個 SA 如何從既有 RECAM-valid solutions 中選出 group solution；後者只
決定 SRAM lookup/search parallelism 與 latency。相同 solution-take policy 下，
Serial／Chunked／Wide 必須保持相同 repair result。

## 0. 文件目的

本文件提出一個**以 RECAM 為功能基準、以 SRAM 取代 CAM 儲存與搜尋功能**的研究架構，暫稱 **SRAM-RECAM**。

這不是 Yoon et al. 原論文中已存在的架構，而是本研究的衍生設計。設計目標是：

1. **保持 RECAM 的 repair semantics 與 repair rate**：
   - pivot / non-pivot fault 分類不變；
   - must-repair 規則不變；
   - Address CAM / Hybrid CAM 的容量限制不變；
   - CAM-reuse 額外 pivot fault 容量不變；
   - matrix conversion 與 fault-free-area analysis 不變。

2. 把 CAM 的「內容比對」功能拆成：
   - SRAM：儲存 fault information；
   - Comparator network：執行 row / column / full-address compare；
   - Controller：控制 serial / chunked / wide-word search。

3. 分別研究兩個階段：
   - **BIRA / fault-collection 階段**
   - **Runtime remapping / CAM-reuse 階段**

4. 可切換多種 SRAM search policy：
   - Serial SRAM
   - Wide-word SRAM + parallel comparators
   - Chunked SRAM + parallel comparators
   - Direct-index / hash SRAM
   - Bloom-filter-assisted lookup
   - optional recent-entry fast path

5. 最終比較：
   - repair rate
   - fault-collection latency
   - BIRA latency
   - runtime remapping latency
   - SRAM bits
   - comparator count
   - estimated area / energy

---

# 1. Source-derived RECAM baseline

原始 RECAM 使用兩種 CAM：

- Address CAM
- Hybrid CAM

RECAM 的核心行為為：

```text
Fault detected
    |
    v
Search Address CAM
    |
    +-- no row/column match --> pivot
    |
    +-- row/column match ----> non-pivot
```

原論文定義：

```text
NA = Address CAM entries
   = Rs + Cs
```

```text
NH = Hybrid CAM entries
   = Rs(Cs - 1) + Cs(Rs - 1)
```

例：

| Rs | Cs | Address entries | Hybrid entries |
|---:|---:|---:|---:|
| 2 | 2 | 4 | 4 |
| 2 | 3 | 5 | 7 |
| 3 | 3 | 6 | 12 |
| 4 | 4 | 8 | 24 |
| 5 | 5 | 10 | 40 |

RECAM 的 Address CAM 在 offline mode 儲存 pivot 與分析 metadata；Hybrid CAM 儲存 non-pivot information。

在 online mode，RECAM 重新利用 CAM：
- Address CAM 儲存 CAM-reuse fault address；
- Hybrid CAM 儲存對應 data word；
- request address 命中後，輸出 Hybrid CAM data 取代 faulty DRAM data。

因此 SRAM-RECAM 的設計不能只處理 BIRA；如果要保留原 RECAM 的 CAM-reuse repair capability，runtime 也必須提供等價的 address lookup + data substitution。

---

# 2. 為什麼 SRAM 不能直接一對一取代 CAM？

CAM 的關鍵能力是：

```text
search_key
   |
   v
all entries compare simultaneously
   |
   v
match vector / matched index
```

SRAM 則是：

```text
index/address
   |
   v
read one SRAM word
```

所以如果直接把 CAM bit cell 換成 SRAM bit cell，功能會缺失。

SRAM-RECAM 的真正替代關係應該是：

```text
CAM
=
storage
+
parallel associative comparison
```

改成：

```text
SRAM
+
external comparator network
+
search controller
```

也就是：

```text
Address CAM
        |
        v
Address SRAM
+
Row comparator(s)
+
Column comparator(s)
+
Search FSM
```

這個想法與 Oh et al. 2017 的設計精神一致：該論文把 fault information 放在一般 memory region，而 BIRA controller 使用 comparator、controller、analyzer 逐步存取及分析資料。該論文不是 SRAM-RECAM，也不是直接提出「SRAM 取代 RECAM CAM」，但它證明 BIRA fault information 並不一定必須存在 CAM 中。

---

# 3. Parameter definitions

定義：

```text
M       = number of rows in repair domain
N       = number of columns in repair domain

Rs      = spare rows
Cs      = spare columns

CH      = number of channels represented by one lookup structure
WL      = runtime data word width

NA      = Rs + Cs
NH      = Rs(Cs - 1) + Cs(Rs - 1)
```

位址寬度：

```text
AR  = ceil(log2(M))
AC  = ceil(log2(N))
ACH = ceil(log2(CH))       # CH=1 時可視為 0
```

其他欄位：

```text
K       = Rs + Cs
APTR    = max(1, ceil(log2(K)))
ALINE   = max(AR, AC)
```

本 project 的 saturating-counter modeling convention：

```text
ARFC = max(1, ceil(log2(Cs + 1)))
ACFC = max(1, ceil(log2(Rs + 1)))
```

理由：
- row counter 只需要保存到 Cs；
- 下一顆 fault 來時若目前已等於 Cs，則可直接觸發 RowMust；
- column 同理。

注意：這是 SRAM-RECAM simulator 的明確 bit-width convention，用於 RTL / area estimation；不要把它寫成 Yoon paper 唯一可能的 transistor-level encoding。

---

# 4. Original RECAM logical entry width

## 4.1 Address structure — offline

功能欄位：

```text
valid / enable        1
row address           AR
column address        AC
row must              1
column must           1
row fail counter      ARFC
column fail counter   ACFC
```

所以：

```text
WA_OFF =
    1
  + AR
  + AC
  + 1
  + 1
  + ARFC
  + ACFC
```

---

## 4.2 Address structure — online

```text
valid / enable   1
row address      AR
column address   AC
channel          ACH
```

所以：

```text
WA_ON = 1 + AR + AC + ACH
```

---

## 4.3 Hybrid structure — offline

```text
valid / enable          1
Address pointer         APTR
descriptor              1
row-or-column address   ALINE
```

所以：

```text
WH_OFF = 1 + APTR + 1 + ALINE
```

---

## 4.4 Hybrid structure — online

```text
valid / enable      1
Address pointer     APTR
data word           WL
```

所以：

```text
WH_ON = 1 + APTR + WL
```

---

# 5. Strict SRAM replacement physical storage

如果同一組 SRAM 要像 RECAM 一樣在 offline / online 兩階段重複使用，實體 SRAM word width 至少要能容納兩個 mode 中較大的格式：

```text
WA_SRAM = max(WA_OFF, WA_ON)
WH_SRAM = max(WH_OFF, WH_ON)
```

因此：

```text
Address SRAM bits =
    NA * WA_SRAM
```

```text
Hybrid SRAM bits =
    NH * WH_SRAM
```

```text
B_SRAM_CORE =
    NA * WA_SRAM
  + NH * WH_SRAM
```

另外，若要完整 reproduce RECAM 的 CAM-reuse overflow 行為，還需要 temporary CAM-reuse fault buffer：

```text
NTEMP = Rs + Cs
```

一個 full fault address 大約需要：

```text
WTEMP = AR + AC + ACH
```

所以：

```text
B_TEMP = NTEMP * WTEMP
```

完整 logical storage：

```text
B_SRAM_TOTAL =
    B_SRAM_CORE
  + B_TEMP
```

此公式只是「storage bits」，不是 SRAM macro 面積。

---

# 6. Example storage size

假設一個 repair domain：

```text
M  = 512
N  = 512
CH = 16
WL = 16 bits
```

因此：

```text
AR  = 9
AC  = 9
ACH = 4
```

## Rs = Cs = 2

```text
K  = 4
NA = 4
NH = 4
APTR = 2

ARFC = 2
ACFC = 2

WA_OFF  = 25 bits
WA_ON   = 23 bits
WA_SRAM = 25 bits

WH_OFF  = 13 bits
WH_ON   = 19 bits
WH_SRAM = 19 bits
```

所以：

```text
Address SRAM = 4 * 25 = 100 bits
Hybrid SRAM  = 4 * 19 =  76 bits

B_SRAM_CORE  = 176 bits
```

Temporary buffer：

```text
WTEMP = 9 + 9 + 4 = 22 bits
B_TEMP = 4 * 22 = 88 bits
```

總 logical storage：

```text
264 bits
```

## Rs = Cs = 3

```text
NA = 6
NH = 12

B_SRAM_CORE = 390 bits
B_TEMP      = 132 bits
Total       = 522 bits
```

## Rs = Cs = 4

```text
NA = 8
NH = 24

B_SRAM_CORE = 696 bits
B_TEMP      = 176 bits
Total       = 872 bits
```

重要：

> 這些 bit 數非常小，因此「6T SRAM bit cell 比 CAM cell 小」不代表實際 SRAM macro 一定比較小。

小 depth SRAM 很容易被 decoder、sense amp、precharge、word-line driver 等 peripheral area 主導。

因此 area evaluation 必須至少做兩層：

```text
Level 1:
storage-bit / transistor proxy

Level 2:
actual SRAM macro + synthesized comparator/controller
```

建議 SRAM macro 用 OpenRAM；comparator / FSM / mux 用 RTL synthesis。

---

# 7. SRAM-RECAM 的核心架構

```text
                    BIST / Fault Stream
                           |
                           v
                 +------------------+
                 | Fault Controller |
                 +------------------+
                           |
                           v
          +--------------------------------+
          | Address SRAM Search Engine     |
          |                                |
          | Address SRAM                   |
          | + P row comparators            |
          | + P column comparators         |
          | + match reduction logic        |
          +--------------------------------+
                   |              |
            pivot / match      matched index
                   |              |
                   v              v
          +----------------+  +----------------+
          | Address SRAM   |  | Hybrid SRAM    |
          | update/write   |  | insert/update  |
          +----------------+  +----------------+
                   \              /
                    \            /
                     v          v
                    +------------+
                    | Matrix     |
                    | Converter  |
                    +------------+
                          |
                          v
                  Fault-Free Analyzer
                          |
                          v
                    Repair Solution
```

核心設計原則：

> **只有 associative search 被移到 SRAM + comparator。RECAM repair algorithm 本身不改。**

因此相同 fault sequence、相同 NA/NH/cam-reuse capacity 下：

```text
CAM-RECAM repair result
==
SRAM-RECAM repair result
```

search latency 可以不同，但 functional result 必須相同。

---

# 8. BIRA Stage — Address SRAM

Address SRAM 是 BIRA 階段最需要 search 的結構。

每一顆 fault：

```text
f = (row, col)
```

需要知道：

```text
row_match_index
column_match_index
```

因為 pivot 的判斷條件是：

```text
row not present
AND
column not present
```

所以 SRAM search engine 必須對 occupied prefix
`[0,address_entry_count)` 中的 Address SRAM entries 做：

```text
row_match[i] =
    valid[i] && (entry[i].row == fault.row)

col_match[i] =
    valid[i] && (entry[i].col == fault.col)
```

因為 pivot entries 彼此 row / column 唯一，所以正常情況：

```text
popcount(row_match) <= 1
popcount(col_match) <= 1
```

但是：

```text
row_match_index
```

與：

```text
col_match_index
```

可能是不同 entry。

目前採 `strict_occupied_scan`：不能在第一個 row 或 column match 就停止；只有
row 與 column match 都已找到時才可提早終止。若只找到其中之一，仍須掃完 occupied
prefix，以保持 RECAM-equivalent 的 counter、must 與 matrix semantics。

---

# 9. BIRA fault collection algorithm

```text
New fault (r,c)
      |
      v
Search occupied Address SRAM entries
      |
      +--------------------------------+
      |                                |
no row/col match                 row or col match
      |                                |
      v                                v
    Pivot                          Non-pivot
      |                                |
      v                                v
Address SRAM free?             update fail count(s)
   /       \                          |
 yes       no                         v
  |         |                    check must
  v         v                       /    \
insert   temp buffer?             yes    no
           /   \                   |      |
         yes    no                 v      v
          |      |             set must  Hybrid SRAM
          v      v             flag      free?
       store   FAIL                         / \
                                          yes no
                                           |   |
                                           v   v
                                         store FAIL
```

這裡的 `FAIL` semantics 應完全沿用目前 RECAM_SPEC：

- Address SRAM full，且 CAM-reuse temporary buffer full：
  - UNREPAIRABLE
- non-pivot 需要 Hybrid entry、沒有觸發 must、Hybrid SRAM full：
  - UNREPAIRABLE
  - 這是 project conservative rule，因為原 paper 沒有明確定義 Hybrid CAM overflow branch。
- non-pivot 觸發 must：
  - triggering fault 不需要新的 Hybrid entry；
  - 後續同 line fault 不再需要新的 Hybrid entry；
  - 已存在且屬於該 must line 的 Hybrid entries 依目前 RECAM core 規則 reclaim。

---

# 10. Serial SRAM search

## 10.1 Architecture

```text
Address SRAM
depth = NA
width = WA_SRAM

one SRAM read port
one row comparator
one column comparator
```

每 cycle：

```text
read entry[i]
compare row
compare column
accumulate match
i++
```

---

## 10.2 BIRA latency

定義：

```text
P_A = number of Address entries compared per SRAM read
```

Serial：

```text
P_A = 1
```

令 `Nused=address_entry_count`。Serial Address scan rounds：

```text
D_A = Nused
```

所以 serial：

```text
D_A = Nused
```

如果 SRAM read + compare 被 simulator 抽象為一個 search round：

```text
L_ADDR_SEARCH = Nused cycles
```

額外 write：

```text
L_WRITE = 1 cycle
```

因此只看 classification + update：

```text
L_FAULT_COLLECTION_BASE = Nused + 1
```

例如：

```text
Rs = Cs = 2
NA = 4
```

若當下四個 entries 都已 occupied，則：

```text
4 read/compare rounds
+ 1 write/update
= 5 cycles / detected fault
```

若 `Nused=0`，controller 由 occupancy counter 直接判定 miss，search latency 為
0；第一顆 pivot 只計 1 cycle write/update。

注意：

> 這不是每個 BIST tested cell 都增加 5 cycles，而是每個「送進 BIRA 的 detected fault」所需的 service latency。

如果 BIST fault arrival rate 高於 SRAM engine throughput，simulation 應加入 queue / BIST stall。

---

# 11. Wide-word SRAM + parallel comparators

## 11.1 Concept

把多個 logical entries pack 在同一個 SRAM word：

```text
SRAM word:
+---------+---------+---------+---------+
| Entry 0 | Entry 1 | Entry 2 | Entry 3 |
+---------+---------+---------+---------+
```

如果：

```text
P_A = NA
```

則一次 SRAM read 就取得全部 Address entries。

外部使用：

```text
P_A row comparators
+
P_A column comparators
```

同時 compare。

---

## 11.2 Latency

full-wide 的 physical parallelism 仍按 provisioned capacity 配置：

```text
P_A = NA
D_A = 0, if Nused = 0
D_A = 1, if Nused > 0
```

因此非空表只需一次：

```text
SRAM read
    |
    v
parallel compare
    |
    v
row/col match vector
```

可以做到接近 CAM 的 lookup behavior。

但代價是：
- SRAM word 很寬；
- comparator 數量接近 CAM parallel compare；
- routing / mux / reduction logic 增加。

所以 Wide-word 不一定是 area 最佳解，但它是很好的 **latency upper-performance point**。

---

# 12. Chunked SRAM — 建議主方案

Chunked 是 Serial 與 Wide-word 的折衷。

例如：

```text
NA = 8
P_A = 2
```

SRAM：

```text
depth = 4 physical words

word0 = entry0, entry1
word1 = entry2, entry3
word2 = entry4, entry5
word3 = entry6, entry7
```

每一個 read：

```text
2 row comparators
2 column comparators
```

搜尋：

```text
round 0 -> entries 0..1
round 1 -> entries 2..3
round 2 -> entries 4..5
round 3 -> entries 6..7
```

所以：

```text
D_A = ceil(Nused / P_A)
```

若 row 與 column 分別在 `i_row`、`i_col` 命中，且兩者都找到，則可在包含較晚
match 的 chunk 後停止：

```text
D_A = floor(max(i_row,i_col)/P_A) + 1
```

只有單一 match 或完全 miss 時仍須掃完整 occupied prefix。`Nused=0` 時為 0 rounds。

這個 `P_A` 就是最重要的 architecture sweep knob。

建議 simulation 至少 sweep：

```text
P_A = 1
P_A = 2
P_A = 4
P_A = NA
```

分別代表：

```text
Serial
2-way Chunked
4-way Chunked
Full Wide
```

---

# 13. Hybrid SRAM 在 fault collection 中不需要 associative insertion search

Hybrid SRAM 的主要功能是保存 non-pivot compressed information。

如果 free entry 由：

```text
free pointer
or
valid bitmap
```

管理，insert 不需要 content search：

```text
free_idx = allocate_hybrid_entry()
HybridSRAM[free_idx] = encoded_nonpivot
```

所以 fault collection 的 critical search 主要是：

```text
Address SRAM
```

而不是 Hybrid SRAM。

Hybrid SRAM 需要 sequential/chunked read 的地方主要在：

```text
Matrix Conversion
```

因為 matrix builder 需要讀出所有有效 Hybrid entries。

---

# 14. Matrix Conversion：兩種 SRAM implementation

## Option A — Rebuild Matrix by SRAM Scan

最簡單、最容易驗證。

每次 fault state 改變：

```text
clear K x K matrix
      |
      v
scan Address SRAM
      |
      v
apply pivot + must rules
      |
      v
scan Hybrid SRAM
      |
      v
apply Hybrid cases
      |
      v
matrix ready
```

定義：

```text
P_A_M = Address entries read per matrix cycle
P_H_M = Hybrid entries read per matrix cycle
```

則：

```text
D_AM = ceil(NA / P_A_M)
D_HM = ceil(NH / P_H_M)
```

matrix rebuild latency：

```text
L_MATRIX_SCAN =
    D_AM + D_HM + L_FINALIZE
```

優點：
- simulator 最容易寫；
- 與目前 RECAM matrix conversion code 容易對照；
- 不容易因 incremental update 漏掉 case。

缺點：
- latency 大。

---

## Option B — Shadow Matrix

SRAM 只當 fault-information backing store，同時保留：

```text
K x K bit shadow matrix
```

每次 fault insertion / must transition 時直接更新 matrix。

目前 `shadow` 實驗採 collection-only latency profile：shadow matrix 的功能仍由
RECAM golden solver 驗證，但 `T_matrix=0`、`T_analyzer=0` modeled cycles；不再
額外收取舊模型的每-fault shadow update cycle。

優點：
- 不需要每次重掃 SRAM；
- BIRA latency 大幅下降；
- matrix 本身很小。

例如：

```text
Rs = Cs = 2 -> 16 bits
Rs = Cs = 3 -> 36 bits
Rs = Cs = 4 -> 64 bits
```

缺點：
- incremental update logic 更複雜；
- 必須確保 Fig.7 的所有 conversion cases 都正確；
- 如果 pivot representation 會重構，shadow matrix 維護更麻煩。

**建議研究流程：**

```text
Phase 1:
SRAM Scan Matrix
-> correctness baseline

Phase 2:
Shadow Matrix
-> latency optimization
```

---

# 15. Fault-Free Area Analyzer

這部分建議完全保留 RECAM。

不要因為 CAM 換成 SRAM 就更改 repair algorithm。

```text
NumSolutions =
    C(Rs + Cs, Rs)
```

例如：

```text
Rs=Cs=2 -> 6
Rs=Cs=3 -> 20
Rs=Cs=4 -> 70
```

可定義 candidate parallelism：

```text
P_SOL
```

如果完全模擬 RECAM parallel analyzer：

```text
P_SOL = NumSolutions
```

則 functional analyzer latency 可視為：

```text
L_ANALYZE = 1 combinational evaluation stage
```

如果研究 area/latency tradeoff：

```text
L_ANALYZE =
ceil(NumSolutions / P_SOL) * L_EVAL
```

但是：

> 第一版 SRAM-vs-CAM 實驗最好固定 analyzer 不變，只改 fault-storage/search architecture，否則無法分辨結果來自 SRAM replacement 還是 analyzer redesign。

---

# 16. BIRA total latency model

`shadow + strict_occupied_scan` 對每顆 detected fault：

```text
L_FAULT =
    L_ADDR_SEARCH
  + L_STATE_UPDATE
```

其中：

```text
L_ADDR_SEARCH =
    strict occupied-prefix rounds * L_CHUNK

L_STATE_UPDATE = 1 cycle
T_MATRIX = 0
T_ANALYZER = 0
```

CAM collection baseline 固定為每 fault 1 cycle compare + 1 cycle update，共 2
cycles；同樣不計 matrix conversion 與 fault-free-area analyzer latency。

`L_CHUNK` 可有兩種 simulation mode：

### Abstract search-round model

```text
L_CHUNK = 1 cycle
```

表示 SRAM read + compare 被視為一個 architecture round。

### Registered SRAM model

```text
SRAM read = 1 cycle
compare/reduction = 1 cycle
```

若沒有 pipeline：

```text
L_CHUNK = 2 cycles
```

若 pipeline：

```text
first result latency = 2
throughput = 1 chunk/cycle
```

建議 simulator 同時輸出：

```text
search_rounds
modeled_cycles
```

不要把兩者混在一起。

## 16.1 BIST overlap 與 after-BIST latency

aggregate `modeled_cycles` 代表 BIRA 工作量，不等於測試完成後可見的等待時間。
event-driven model 應使用同一個 group-relative timeline：

```text
t_arrive(fault)       BIST 發現 fault 並送到 BIRA
t_collection_done    最後一筆 fault 完成 search/update
t_bist_done          BIST 完成整個 subarray group
t_solution_ready     最終 repair solution 可用
```

正式比較採：

```text
latency_after_bist = max(0, t_solution_ready - t_bist_done)
```

若 SRAM search service rate 低於 fault arrival rate，必須另外回報
`maximum_fault_queue_depth`、`total_fault_queue_wait_cycles` 與
`bist_backpressure_cycles`。未建立 address scan schedule 與 queue model 時，CSV
必須標記 `bist_timeline_modeled=0`，不可用 aggregate cycles 代替。

## 16.2 Decoupled FIFO baseline

第一版 event model 採 **unbounded decoupled FIFO + single BIRA server**：BIST
偵測到 fault 後只把事件送入 FIFO，不等待 BIRA 完成 fault collection。因此即使
service rate 低於 arrival rate，也只會增加 queue depth 與 after-BIST latency，
不會延長 BIST scan：

```text
bist_backpressure_cycles = 0
```

令第 `i` 個 fault 的到達時間為 `a_i`，collection service time 為 `s_i > 0`，
依 `(a_i, input_sequence)` 排序後：

```text
service_start_i = max(a_i, service_done_(i-1))
service_done_i  = service_start_i + s_i
queue_wait_i    = service_start_i - a_i
```

group end signal 到達後才能開始 final analysis 的預設模型為：

```text
t_collection_done = service_done_last
t_analysis_start   = max(t_collection_done, t_bist_done)
t_solution_ready   = t_analysis_start + L_final_analysis
latency_after_bist = max(0, t_solution_ready - t_bist_done)
```

`maximum_fault_queue_depth` 只計算等待中的 entries，不包含正由 BIRA service 的
fault；`total_fault_queue_wait_cycles` 是所有 fault 的 `queue_wait_i` 總和。此核心
實作於 `BiraLatency::simulateDecoupledFifo()`。Hierarchical path 的上游 adapter
已固定為 `FaultAddress::SerialBistSchedule`：A→B→C→D 串行、row-major、
word-column-major。external BIST packet 的 `WordCol + failMask` 會先解碼為
internal physical-cell faults，再個別排入 FIFO；這能保留 line-repair/remap 的
cell-level coverage，同時讓同一 word 的 events 擁有相同 arrival cycle。

## 16.3 已凍結的 SRAM fault-collection golden

`make test_sram_fault_collection_fifo` 會比較
`tests/golden/sram_fault_collection_fifo_expected.csv`。它的基本手算 case 是：

```text
Rs=2, Cs=2 -> K=4
SRAM chunked address search P=2, state update=1 cycle
3 physical faults at arrival {1,2,3}; BIST group done at cycle 5
```

其中 `fifo_shadow_matrix` variant 的三筆 fault 都是依序新增的 pivot，因此 search
rounds 為 `{0,1,1}`，每 fault service 為 `{1,2,2}`：

```text
start={1,2,4}, done={2,4,6}, wait={0,0,1}
collection work=5, final analysis=0, solution ready=6
max queue depth=1, total wait=1, after-BIST latency=1
```

同一 golden 也涵蓋 serial、wide、registered search 與 shadow-matrix variants。
它是 architecture-level delay regression，不是 SRAM macro 的 post-layout timing。

---

# 17. Runtime：必須先區分兩種 repair

Runtime 有兩類不同的 repair path：

## A. Spare-line remapping

BIRA 最後產生：

```text
faulty row -> spare row
faulty col -> spare col
```

這是一個 repair-map lookup 問題。

## B. RECAM CAM-reuse cell substitution

Address CAM overflow 後的 additional pivot faults 在 online mode 由 CAM 當 redundancy storage。

這是：

```text
full fault address -> replacement data word
```

如果 SRAM-RECAM 要宣稱「保持 RECAM repair rate」，B 類一定要保留。

---

# 18. Runtime strict SRAM-RECAM path

Power-up：

```text
eFuse / NVM
    |
    v
load CAM-reuse fault addresses
    |
    v
Address SRAM
```

Data SRAM / Hybrid SRAM 為每個 CAM-reuse fault 準備 data slot。

Runtime request：

```text
request address
     |
     +--------------------------+
     |                          |
     v                          v
normal DRAM access       SRAM repair lookup
                                |
                           match / miss
                                |
                  +-------------+-------------+
                  |                           |
                 hit                         miss
                  |                           |
                  v                           v
            repair data                  DRAM data
                  |                           |
                  +-------------+-------------+
                                |
                                v
                              MUX
```

DRAM 與 SRAM search 應平行進行。

---

# 19. Runtime Serial SRAM lookup

最簡單：

```text
P_R = 1
```

逐 entry：

```text
read entry 0 -> compare
read entry 1 -> compare
...
```

如果 runtime table 有：

```text
NR = Rs + Cs
```

entries：

```text
L_LOOKUP_MISS = NR search rounds
```

Hit latency：

```text
L_LOOKUP_HIT(index i) =
    floor(i / P_R) + 1
```

若 match 後還要另外讀 data SRAM：

```text
L_HIT =
    L_LOOKUP_HIT
  + L_DATA
```

Serial 很適合做：
- 最小 comparator baseline；
- area lower-bound；
- latency worst-case baseline。

但不建議當最終 runtime architecture。

---

# 20. Runtime Wide-word SRAM + parallel comparators

如果：

```text
P_R = NR
```

則一次讀取所有 runtime repair entries：

```text
wide SRAM read
    |
    v
NR full-address comparators
    |
    v
priority/select
```

理論 search round：

```text
1
```

如果 data 與 tag 同時被讀出，可直接：

```text
match vector
    |
    v
data mux
```

不用第二次 SRAM read。

它最接近 CAM 的 parallel lookup，但把 associative compare 從 bitcell 移到 SRAM 外部 comparator network。

---

# 21. Runtime Chunked SRAM — 建議 baseline

```text
P_R = 2, 4, ...
```

一個 SRAM word 包含 P_R 個：

```text
{valid, key, optional data}
```

key：

```text
{channel, row, column}
```

每次 SRAM read：
- P_R 個 full-key comparators；
- OR / priority encoder；
- 如果 hit，直接選該 entry data。

Worst-case：

```text
L_LOOKUP =
ceil(NR / P_R)
```

若 data 同 word：

```text
不需要額外 data SRAM read
```

這是 runtime 最值得優先 sweep 的版本。

---

# 22. Recommended runtime organization：Tag + Data direct indexing

如果仍希望保持 Address SRAM / Hybrid SRAM 分離：

```text
Address SRAM search
      |
      v
matched Address index = i
      |
      v
Data SRAM[i]
```

也就是：

```text
Address index
```

直接成為 data SRAM index。

這樣不需要像 CAM 結構一樣再用 Hybrid pointer 做第二次 associative lookup。

這是 SRAM architecture 很自然的優化。

Power-up 時：

```text
AddressSRAM[i] = fault_address_i
DataSRAM[i]    = repair_data_i
```

write hit：

```text
DataSRAM[i] = input_data
```

read hit：

```text
output = DataSRAM[i]
```

此版本保持：

```text
CAM-reuse repair capacity = Rs + Cs
```

但簡化了 online pointer search。

這屬於 SRAM-RECAM proposal，不是原 RECAM paper 的硬體。

---

# 23. Direct-index SRAM

最極端的方法是：

```text
address -> SRAM index
```

不搜尋。

但完整 DRAM address space 很大，直接：

```text
SRAM[full_DRAM_address]
```

通常不合理。

例如 `M=N=512`：

```text
row+col = 18 address bits
```

完整 direct table 可能需要：

```text
2^18 entries
```

只為保存少數 repair records，浪費極大。

因此不建議 naive direct-index。

---

# 24. Hash-indexed SRAM

較合理：

```text
bucket = H(full_address)
```

每個 bucket 放 W ways：

```text
bucket:
way0 {valid, full_tag, data}
way1 {valid, full_tag, data}
...
```

lookup：

```text
hash
 |
 v
read one SRAM bucket
 |
 v
W tag comparators
 |
 v
hit / miss
```

完整 tag compare 可以避免 hash collision 導致錯誤 remap。

推薦：

```text
2-way
or
4-way
```

set-associative hash SRAM。

因為 repair table 在 power-up 後基本固定，所以可以在 NVM programming / power-up 時選擇 hash seed。

---

# 25. Hash overflow 必須有 fallback

不能因為 hash collision 就降低 repair rate。

因此：

```text
Hash table
    |
    +-- placed successfully
    |
    +-- bucket overflow
            |
            v
        overflow stash
            |
            v
        if stash full:
            fallback to chunked table
```

或在 power-up：

```text
try seed0
try seed1
try seed2
...
```

直到所有 repair entries 可放入。

Simulation invariant：

```text
Hash policy must never convert a repairable RECAM case
into unrepairable merely because of hash collision.
```

否則比較的已經不是「SRAM 取代 CAM」而是新的 lossy repair policy。

---

# 26. Bloom Filter fast bypass

Bloom filter 最適合處理：

> **「這個 request 一定不在 repair table」**

而不是用來直接判定 hit。

Bloom filter 性質：

```text
negative:
    definitely not present

positive:
    may be true hit
    or false positive
```

所以：

```text
request
   |
   v
Bloom Filter
   |
   +-- negative --> skip SRAM repair search
   |               use normal DRAM path
   |
   +-- positive --> exact SRAM lookup
```

只要 Bloom filter 正確建構：

```text
no false negative
```

所以不會漏掉真正需要 repair 的 request。

---

# 27. Bloom filter size model

定義：

```text
n = number of runtime repair entries
m = Bloom-filter bits
k = number of hash functions
```

standard approximate false-positive probability：

```text
FPR ≈ (1 - exp(-k*n/m))^k
```

接近最佳：

```text
k ≈ (m/n) * ln(2)
```

Simulation 不需要一開始真的做複雜 hash RTL。

可以先 sweep：

```text
m = 16, 32, 64, 128 bits
k = 2, 3
```

並計算：
- Bloom negative rate
- false-positive rate
- SRAM searches avoided
- average runtime lookup latency
- average comparator activations

---

# 28. Bloom Filter 也可以用在 BIRA fault collection

Address SRAM search 的目的是判斷：

```text
fault.row 是否存在 pivot rows
fault.col 是否存在 pivot columns
```

可以維護：

```text
RowBloom
ColumnBloom
```

新 fault：

```text
RowBloom(row)
ColumnBloom(col)
```

如果：

```text
RowBloom = negative
AND
ColumnBloom = negative
```

則可以直接知道：

```text
row 不存在
column 不存在
=> new pivot
```

不需要掃 Address SRAM。

如果任一 Bloom positive：

```text
exact Address SRAM search
```

以排除 false positive。

因此：

```text
Bloom negative
-> fast pivot path

Bloom positive
-> SRAM exact search
```

這對「大量 unique pivot / sparse fault」可能很有用。

但它是 SRAM-RECAM optimization，不是 RECAM 原論文機制。

---

# 29. Recently-used fast path

Bloom filter 解決的是：

```text
definite miss
```

不是：

```text
recent hit
```

如果 fault model 有 line / cluster locality，可以額外放：

```text
Recent Pivot Buffer
```

例如 1、2、4 entries：

```text
recent pivot registers
+
small comparator network
```

流程：

```text
new fault
   |
   v
Recent Pivot Compare
   |
   +-- hit --> immediately classify non-pivot
   |
   +-- miss --> Bloom / SRAM search
```

這不是 CAM macro，而是少數 registers + comparators。

Simulation 可以量：
- recent-hit rate
- avoided SRAM scans
- extra comparator cost

---

# 30. Runtime average latency model

假設：

```text
p_hit = probability request targets repair entry
```

Chunked search：

```text
D_R = ceil(NR / P_R)
```

miss：

```text
L_miss = D_R
```

hit：

```text
L_hit(index) =
floor(index / P_R) + 1
```

若 entries 隨機：

```text
average hit rounds
≈ (D_R + 1) / 2
```

近似：

```text
L_avg =
p_hit * L_hit_avg
+
(1-p_hit) * L_miss
```

加入 Bloom：

```text
p_bf_positive =
p_hit
+
(1-p_hit) * FPR
```

大致 exact-SRAM-search 次數：

```text
SearchRate =
p_bf_positive
```

因此 Bloom 對 repair hit 非常低的系統，主要降低的是：
- SRAM dynamic energy
- comparator switching
- average search work

是否降低可見 request latency，取決於 Bloom lookup 是否能和 normal DRAM access 平行。

---

# 31. Runtime exposed latency

RECAM 原 paper 的重要觀念是：

```text
DRAM operation
and
repair lookup
```

平行。

因此 SRAM lookup latency 不應直接全部加到 memory latency。

定義：

```text
L_NORMAL_DRAM
=
從 request 到正常 DRAM data ready 的時間
```

```text
L_REPAIR_PATH
=
SRAM tag search
+
optional data access
+
MUX
```

真正 externally exposed additional latency：

```text
L_EXTRA =
max(0, L_REPAIR_PATH - L_NORMAL_DRAM)
```

也就是：

```text
如果 SRAM lookup 在 DRAM data ready 前完成：
    extra latency = 0

如果 SRAM lookup 太慢：
    extra latency = lookup overrun
```

這是 runtime simulator 應該實作的 latency model。

---

# 32. Serial / Chunked / Wide latency example

假設：

```text
Rs = Cs = 4
NR = 8
```

而 repair data 直接和 tag 同一 SRAM chunk 讀出。

| Policy | P_R | worst search rounds |
|---|---:|---:|
| Serial | 1 | 8 |
| Chunked-2 | 2 | 4 |
| Chunked-4 | 4 | 2 |
| Wide | 8 | 1 |

如果 tag / data 分離且 data 另讀 1 cycle：

| Policy | worst hit cycles, simple model |
|---|---:|
| Serial | 9 |
| Chunked-2 | 5 |
| Chunked-4 | 3 |
| Wide | 2 |

所以對 runtime，我優先建議：

```text
Chunked-4
```

或：

```text
Wide
```

作為第一批模擬點。

---

# 33. Area model：不要只比較「bits」

## 33.1 First-order bit-cell proxy

可以先做一個粗略 proxy：

```text
SRAM cell ~ 6T
binary CAM cell ~ 約 9T 級
```

但是這只能說：

```text
storage bitcell density trend
```

不能直接說：

```text
總 macro 面積一定縮小 33%
```

因為 CAM / SRAM 都還有 peripheral circuits，SRAM-RECAM 還要額外 comparator network。

---

## 33.2 Recommended area equation

SRAM-RECAM：

```text
Area_SRAM_RECAM =
    Area(Address SRAM macro)
  + Area(Hybrid/Data SRAM macro)
  + Area(temp buffer)
  + Area(row comparators)
  + Area(column comparators)
  + Area(full-address comparators)
  + Area(priority encoder)
  + Area(search FSM)
  + Area(matrix logic)
  + Area(fault-free analyzer)
  + optional Bloom/hash logic
```

RECAM baseline：

```text
Area_RECAM =
    Area(Address CAM)
  + Area(Hybrid CAM)
  + Area(temp buffer)
  + Area(matrix logic)
  + Area(fault-free analyzer)
```

對 SRAM search policy：

```text
Comparator count roughly scales with P
```

BIRA：

```text
# row comparators    = P_A
# column comparators = P_A
```

Runtime：

```text
# full-key comparators = P_R
```

所以：

```text
higher P
-> lower latency
-> higher comparator area / switching
```

這就是 SRAM-RECAM 最重要的 Pareto sweep。

---

# 34. OpenRAM / RTL synthesis 建議

對 SRAM：

```text
OpenRAM:
depth  = number of physical SRAM words
width  = packed word width
ports  = 1R1W or 1RW
```

例如 Address Chunked：

```text
physical depth =
ceil(NA / P_A)

physical width =
P_A * WA_SRAM
```

Hybrid Chunked：

```text
physical depth =
ceil(NH / P_H)

physical width =
P_H * WH_SRAM
```

Runtime combined RMT：

```text
physical depth =
ceil(NR / P_R)

physical width =
P_R * (valid + key + data)
```

對 comparator / FSM：
- Verilog RTL
- synthesize with same technology library
- report:
  - cell area
  - critical path
  - power estimate if available

非常重要：

> 對非常小的 NA/NH，OpenRAM macro 可能不是最佳實體實作。  
> 因此還應加入「register-file / FF array + comparator」作為第三個 area baseline。

---

# 35. 建議的 architecture variants

## Variant S0 — Serial SRAM

```text
P_A = 1
P_R = 1
```

目的：
- minimum comparator count
- area-oriented baseline
- maximum lookup latency

---

## Variant S1 — Full Wide SRAM

```text
P_A = NA
P_R = NR
```

目的：
- SRAM version performance upper bound
- closest to CAM parallel lookup

---

## Variant S2 — Chunked SRAM

推薦主方案。

```text
P_A ∈ {2,4}
P_R ∈ {2,4}
```

目的：
- area / latency Pareto

---

## Variant S3 — Hash Runtime SRAM

Runtime only：

```text
set-associative hash
2-way / 4-way
```

目的：
- near-O(1) lookup
- smaller comparator network than full CAM

必須保留 collision fallback。

---

## Variant S4 — Bloom + Chunked

```text
Bloom negative
-> skip SRAM

Bloom positive
-> Chunked exact lookup
```

適合：
- runtime repair-hit rate 低
- sparse fault
- BIRA sparse pivot membership checking

---

# 36. 我最建議先做的版本

如果目標是兩個月內把 architecture、simulator、hardware model 都做完，我會先固定：

```text
BIRA:
Address SRAM = Chunked
Hybrid SRAM  = indexed storage
Matrix       = scan-rebuild baseline
Analyzer     = current RECAM analyzer unchanged

Runtime:
Chunked Address SRAM
+
direct-index Data SRAM

Optional:
Bloom filter
```

先不要同時把：
- hash
- Bloom
- shadow matrix
- recent cache
全部設為 default。

建議實驗順序：

```text
1. RECAM CAM baseline
2. Serial SRAM
3. Chunked-2
4. Chunked-4
5. Full Wide
6. Bloom + best Chunked
7. Hash runtime only
```

---

# 37. 最重要的 simulation invariants

## Repair equivalence

相同：
- fault sequence
- Rs / Cs
- NA / NH
- CAM-reuse capacity

必須：

```text
repair_result(SRAM_RECAM)
==
repair_result(RECAM)
```

除非明確開啟新的 lossy policy。

---

## Classification equivalence

每顆 fault：

```text
pivot/non-pivot
must-row/must-col
Address entry state
Hybrid entry state
matrix
RA solution
```

都必須和 RECAM baseline 相同。

---

## Search policy only changes timing / area

Serial、Chunked、Wide：

```text
不能改 repair semantics
```

只允許改：

```text
search rounds
modeled cycles
comparator count
SRAM organization
energy / area
```

---

## Hash / Bloom correctness

Bloom：
- negative 可直接 bypass；
- positive 一定做 exact check；
- 不允許 false negative。

Hash：
- full tag compare；
- collision overflow 必須 fallback；
- 不允許 hash collision 降低 repair rate。

---

# 38. 建議 simulator 輸出

每次 experiment：

```text
policy
Rs
Cs
M
N
channels
word_bits

address_entries
hybrid_entries

address_sram_bits
hybrid_sram_bits
temp_buffer_bits

P_A
P_H
P_R
P_SOL

fault_collection_search_rounds
address_entry_count
hybrid_entry_count
address_search_cycles_total / avg / max
early_termination_hit_count
hybrid_bitmap_allocations
hybrid_full_count
pivot / nonpivot / must-triggering search cycles
matrix_read_rounds
analysis_rounds
total_bira_cycles

runtime_hit_rounds_avg
runtime_miss_rounds
runtime_latency_avg
runtime_extra_latency_avg

row_comparator_count
column_comparator_count
runtime_comparator_count

bloom_bits
bloom_hashes
bloom_false_positive_rate
sram_searches_avoided

repair_rate
repair_rate_matches_recam
```

---

# 39. 最終研究問題

SRAM-RECAM 最適合回答的研究問題不是：

> SRAM 比 CAM 小多少？

而是：

> **當 RECAM 的 associative storage 從 CAM 拆成 SRAM + selectable comparator parallelism 後，能否在保持相同 repair rate 的前提下，找到 area、BIRA latency、runtime remapping latency 之間更好的 Pareto point？**

核心 sweep：

```text
Search Parallelism P
```

就是：

```text
P = 1
   -> 最少 comparator
   -> 最慢

P = entries
   -> 最多 comparator
   -> 最快

1 < P < entries
   -> Chunked Pareto region
```

這也是最容易被 RTL synthesis 與 simulator 一起驗證的研究故事。

---

# 40. References / source boundary

### Source-derived behavior

1. J. Yoon et al.,  
   “A Built-In Self-Repair With Maximum Fault Collection and Fast Analysis Method for HBM,”  
   IEEE TCAD, 2025.  
   Used for:
   - RECAM Address CAM / Hybrid CAM organization;
   - entry counts;
   - pivot / non-pivot behavior;
   - matrix conversion;
   - fault-free-area analysis;
   - offline/online CAM reuse.

2. C.-H. Oh, S.-E. Kim, and J.-S. Yang,  
   “BIRA With Optimal Repair Rate Using Fault-Free Memory Region for Area Reduction,”  
   IEEE TCAS-I, 2017.  
   Used as architectural evidence that fault information can reside in ordinary memory and be processed by a comparator/controller/analyzer rather than requiring CAM-resident fault collection.

3. K. Pagiamtzis and A. Sheikholeslami,  
   “Content-Addressable Memory (CAM) Circuits and Architectures: A Tutorial and Survey,”  
   IEEE JSSC, 2006.  
   Used for CAM architectural background and bit-cell-area motivation.

### Proposed by this project

The following are **not original RECAM mechanisms**:

- Serial SRAM search
- Wide-word SRAM + external comparators
- Chunked SRAM
- SRAM direct-index data organization
- set-associative hash runtime table
- Bloom-filter bypass
- recent-entry fast path
- shadow matrix
- SRAM-specific cycle and area models

These must be labeled as SRAM-RECAM proposal / simulation policies in thesis writing.
