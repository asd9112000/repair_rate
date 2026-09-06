# Solution Take Policy Research Handoff

## 1. 本階段研究問題

目前 Dynamic Spare Sharing 使用四個 repair tiles / subarrays：

```text
A B
C D
```

未來亦會研究：

```text
A — B — C — D
```

四個 tile 依序接受 BIST / RECAM analysis。

目前主要研究問題不再只是：

```text
哪些 spare line 可以分享？
```

而是：

```text
當一個 tile 有多個 valid RECAM solutions 時，
應該在什麼時間點決定最終 solution？
```

這稱為：

```text
Solution Take Policy
```

---

# 2. RECAM Solution 的重要語意

對：

```text
K = Rs + Cs
```

RECAM 建立：

```text
K × K fault analysis matrix
```

並產生：

```text
S = C(Rs + Cs, Rs)
```

個固定 Row/Column repair candidates。

例如：

```text
Rs = Cs = 2
K = 4
S = 6
```

六個 solution pattern：

```text
RRCC
RCRC
RCCR
CRRC
CRCR
CCRR
```

其中：

```text
R at index i
```

代表：

```text
使用 matrixRowAddresses[i]
```

而：

```text
C at index i
```

代表：

```text
使用 matrixColumnAddresses[i]
```

因此：

```text
solution ID
```

本身不包含實際 DRAM address。

例如：

```text
Solution = R C R C
```

只代表：

```text
index 0 -> Row
index 1 -> Column
index 2 -> Row
index 3 -> Column
```

必須搭配：

```text
matrixRowAddresses[]
matrixColumnAddresses[]
```

才能還原真正 physical repair lines。

---

# 3. Tile Analysis 完成後保留的資訊

本研究決定：

```text
Address CAM
Hybrid CAM
Fault Analysis Matrix
```

皆屬於：

```text
temporary BIRA analysis state
```

在該 tile 的 valid solutions 已經產生之後，不需要為了 Group Decision 繼續保存 Hybrid CAM。

原因是：

```text
Pivot information
        +
Hybrid CAM nonpivot information
        +
Must-repair information
        ↓
Fault Analysis Matrix
        ↓
Valid Solution Check
```

當：

```text
validSolutionBitmap[i] = 1
```

時，solution `i` 已經通過完整 matrix fault-free-area verification。

因此該 solution 已經考慮：

```text
pivot faults
nonpivot faults
must-repair constraints
```

Group-level analyzer 不需要重新分析原始 Hybrid CAM。

---

# 4. Compressed Tile Solution State

每個 tile analysis 完成後，保留：

```cpp
TileSolutionState {
    matrixRowAddresses[K];
    matrixColumnAddresses[K];

    validSolutionBitmap[S];
}
```

概念上：

```text
Temporary analysis state
─────────────────────────
Address CAM
Hybrid CAM
Matrix
Must state
        |
        | RECAM analysis complete
        v
Compressed retained state
─────────────────────────
matrixRowAddresses[K]
matrixColumnAddresses[K]
validSolutionBitmap[S]
        |
        v
clear CAM / Hybrid CAM / Matrix scratch
```

這是目前 Group Decision 的主要 storage architecture。

---

# 5. 為什麼不能只保存 Address CAM

不能假設：

```text
Address CAM pivot addresses
+
validSolutionBitmap
```

一定足夠。

原因是 RECAM matrix conversion 中，Hybrid CAM information 可能填入原本沒有 pivot 的 matrix address slot。

因此正確保存對象是：

```text
matrixRowAddresses[]
matrixColumnAddresses[]
```

而不是只保存：

```text
Address CAM rows / columns
```

---

# 6. Valid Solution Bitmap

不保存 variable-length solution ID list。

改用：

```text
validSolutionBitmap
```

例如：

```text
Rs = Cs = 2
S = 6
```

假設：

```text
Solution 0 valid
Solution 1 invalid
Solution 2 valid
Solution 3 invalid
Solution 4 invalid
Solution 5 valid
```

則：

```text
validSolutionBitmap = 101001
```

實際 bit ordering 必須完全跟既有 `SolGenerator` solution index ordering 一致。

禁止重新排列 solution numbering。

---

# 7. Solution Take Policy A — EARLY

流程：

```text
Analyze A
   ↓
generate valid solution bitmap
   ↓
choose one valid solution immediately
   ↓
commit physical spare resources
   ↓
discard A temporary state

Analyze B
   ↓
choose according to remaining resources
   ↓
commit

Analyze C
Analyze D
```

所以：

```text
A decision
   ↓
Remaining Resource State
   ↓
B decision
   ↓
Remaining Resource State
   ↓
C
   ↓
D
```

### 優點

- 最小 retained state。
- solution 可以提早 commit。
- analyzer scratch 可以立即 reuse。
- group 不需要保留四組完整 solution state。
- group-level final analyzer 很簡單。

### 缺點

- greedy。
- order-sensitive。
- A/B/C/D 前面的 tile 可能消耗後面更需要的 shared spare。
- local optimum 不保證 group optimum。

---

# 8. EARLY Solution Ranking

第一版不要只使用：

```text
minimum total spare lines
```

推薦採 lexicographic ranking：

```text
1. minimum borrowed spare lines
2. minimum total spare lines used
3. deterministic smallest solution ID
```

row-only sharing 時：

```text
1. minimum borrowed rows
2. minimum total rows used
3. minimum solution ID
```

不需要真的建立 weighted arithmetic cost。

直接做：

```text
compare borrowed_rows
then compare total_rows
then compare solution_id
```

這樣 deterministic 且容易驗證。

---

# 9. Solution Take Policy B — GROUP_COMPRESSED

A/B/C/D 依序分析，但不立即 commit solution。

流程：

```text
Analyze A
   ↓
save TileSolutionState[A]
   ↓
clear RECAM scratch

Analyze B
   ↓
save TileSolutionState[B]
   ↓
clear scratch

Analyze C
   ↓
save TileSolutionState[C]

Analyze D
   ↓
save TileSolutionState[D]

        ↓

Group Solution Analyzer
        ↓
search valid combination
        ↓
commit A/B/C/D together
```

Group analyzer 搜尋：

```text
solutionA
× solutionB
× solutionC
× solutionD
```

但只枚舉 bitmap 中：

```text
valid = 1
```

的 solutions。

---

# 10. GROUP_COMPRESSED 不保存什麼

不保存：

```text
4 × Address CAM state
4 × Hybrid CAM state
4 × Fault Analysis Matrix
4 × complete fault list
```

只保存：

```text
4 × matrixRowAddresses[K]
4 × matrixColumnAddresses[K]
4 × validSolutionBitmap[S]
```

所以 Group Decision 並不等於 Full-State Group Decision。

本研究目前主要比較：

```text
EARLY
vs
GROUP_COMPRESSED
```

---

# 11. Group Combination Feasibility

對每組：

```text
(A_solution,
 B_solution,
 C_solution,
 D_solution)
```

先 decode：

```text
solution pattern
+
matrixRowAddresses[]
+
matrixColumnAddresses[]
```

得到各 tile 所需：

```text
physical source rows
physical source columns
```

接著送入既有 sharing / resource-allocation mechanism。

必須遵守：

```text
local spare availability
shared spare topology
minimum reserve
max borrows
physical spare ownership
one spare cannot be allocated twice
```

只有四個 tile 同時 feasible 才算 group valid。

---

# 12. Group Objective Function

第一版 GROUP_COMPRESSED 不需要設計複雜 optimizer。

在所有 feasible combinations 中使用：

```text
Priority 1:
minimum total borrowed spare lines

Priority 2:
minimum total spare lines consumed

Priority 3:
lexicographically smallest
(A_solution_id,
 B_solution_id,
 C_solution_id,
 D_solution_id)
```

row-only experiment：

```text
Priority 1:
minimum total borrowed rows

Priority 2:
minimum total rows consumed

Priority 3:
deterministic solution IDs
```

這樣：

```text
GROUP_COMPRESSED
```

可以作為 EARLY policy 的高品質 / near-global reference。

---

# 13. Physical Address 與 Resource Identity 要區分

`matrixRowAddresses[i]` 表示：

```text
faulty/source DRAM row address
```

它不是：

```text
physical spare row ID
```

Group analyzer 的工作有兩層：

```text
Selected RECAM solution
        ↓
source rows / columns that require replacement
        ↓
PhysicalResourceLedger
        ↓
assign local / borrowed physical spare resource
```

禁止把：

```text
matrixRowAddresses[]
```

直接當成 spare ownership identifier。

---

# 14. CAM Reuse / Buffer Repair 必須獨立處理

本 handoff 中：

```text
validSolutionBitmap
```

只描述：

```text
row/column spare-line candidate validity
```

RECAM additional pivot CAM-reuse mappings 屬於另一個 repair resource。

因此若現有 solver 的某個成功 solution 還需要：

```text
BUFFMAP / CAM reuse
```

則 corresponding persistent CAM-reuse mapping 仍必須保留。

不要因為 Hybrid CAM analysis state 被清除，就一起遺失 final CAM-reuse assignment。

需區分：

```text
Hybrid CAM during BIRA
    = temporary nonpivot analysis state

Final CAM-reuse mapping
    = persistent repair decision
```

---

# 15. 第一階段研究範圍

先只實作：

```text
EARLY
GROUP_COMPRESSED
```

不要先加入：

```text
TOP-K
beam search
dynamic programming
branch-and-bound
complex heuristic
```

等 EARLY vs GROUP_COMPRESSED repair-rate 差異確定之後，再考慮 TOP-K。

---

# 16. 優先實驗配置

先固定：

```text
Layout:
1×4

A — B — C — D

Sharing:
row only

Rs = 2
Cs = 2

shared_rows = 1
shared_columns = 0
```

比較：

```text
EARLY
vs
GROUP_COMPRESSED
```

並使用完全相同 fault maps。

觀察：

```text
repair rate
borrowed rows
spare utilization
decision latency proxy
retained storage bits
order sensitivity
```

---

# 17. 最重要的新指標：Greedy Loss

定義：

```text
group_optimal_success = GROUP_COMPRESSED success

early_success = EARLY success
```

則可統計：

```text
greedy_loss_event =
GROUP_COMPRESSED succeeds
AND
EARLY fails
```

並計算：

```text
greedy_loss_rate
=
greedy_loss_events / total_groups
```

這可以直接回答：

```text
提早決策節省 storage，
到底犧牲多少 repairability？
```

---

# 18. Order Sensitivity

EARLY policy 應保留 A→B→C→D 作為主要硬體流程。

另外可做 diagnostic：

```text
A B C D
D C B A
```

甚至所有 24 permutations。

但這是分析 experiment，不需要第一版硬體支援 configurable physical scan order。

如果同一 fault group 在不同 order 產生不同 repairability：

```text
order_sensitive = true
```

這正是 EARLY greedy policy 的特性之一。

---

# 19. Storage Cost 初步公式

令：

```text
K = Rs + Cs
S = C(K, Rs)

Br = row address bits
Bc = column address bits
```

則每 tile compressed state：

```text
Storage_tile
=
K × Br
+
K × Bc
+
S
```

四 tile：

```text
Storage_group
=
4 × [K(Br + Bc) + S]
```

例如：

```text
Rs = Cs = 2
K = 4
S = 6
```

則：

```text
Storage_tile
=
4Br + 4Bc + 6 bits
```

這可以直接與：

```text
full matrix
full CAM retention
```

做 analytical comparison。

---

# 20. Current Research Hypothesis

目前假設：

```text
EARLY
```

具有：

```text
low storage
low control complexity
possible greedy repair-rate loss
```

而：

```text
GROUP_COMPRESSED
```

具有：

```text
higher retained state
higher combination-search work
better group-level resource allocation
```

真正值得量測的問題是：

```text
GROUP_COMPRESSED 比 EARLY
能增加多少 repair rate？

這個 repair-rate gain
是否值得 compressed state + group analyzer cost？
```

這是目前 Solution Take Policy 研究的核心。