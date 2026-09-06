# RECAM_SPEC.md

> 文件狀態：Current
> 適用範圍：cross-cutting / recam-core
> 建立時間：Unknown
> 最後修改時間：2026-09-07T00:00:00+08:00
> 本文件權威主題：RECAM fault classification、CAM、matrix、repair semantics 與核心驗證契約

## 文件分區

- 第 1–17 節：目前 RECAM 行為、資料模型與 repair semantics 的權威規格。
- 第 18–43 節：驗證策略、測試案例、回歸目標、invariants 與規格邊界。

本文件同時保存行為規格與驗證契約；其他文件若描述 RECAM 共通規則，均以本文件為準。

## 1. Purpose

This document defines the **behavioral specification and verification contract** for the implemented RECAM simulator.

RECAM refers to the BISR architecture proposed in:

> J. Yoon, H. Lee, Y. Moon, S. H. Shin, and S. Kang,
> “A Built-In Self-Repair With Maximum Fault Collection and Fast Analysis Method for HBM,”
> IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, vol. 44, no. 5, May 2025.
> DOI: 10.1109/TCAD.2024.3499903.

The simulator is assumed to be **already implemented**. Therefore, this file is not an implementation tutorial. Its purpose is to:

1. define the RECAM behavior that the simulator must reproduce;
2. define the hardware abstractions that must be represented;
3. define observable intermediate states for debugging;
4. define deterministic and statistical validation tests;
5. provide a golden-reference contract for Codex when reviewing or modifying the implementation.

---

## 2. Scope

The simulator shall model the RECAM mechanisms required to evaluate **repairability and repair rate**.

Primary scope:

- offline BISR / BIRA behavior;
- pivot and nonpivot fault classification;
- Address CAM behavior;
- Hybrid CAM behavior;
- must-repair conditions;
- CAM-reuse overflow handling;
- matrix conversion;
- fault-free-area repair analysis;
- final repairability decision;
- repair solution generation;
- optional online CAM-reuse behavior;
- Monte Carlo repair-rate evaluation.

The simulator does **not** need transistor-level CAM timing, physical DRAM timing, gate-level NOR delays, eFuse programming physics, or HBM protocol timing unless a separate timing model explicitly requires them.

For repair-rate verification, hardware-parallel operations may be represented functionally in software.

---

## 3. RECAM Architectural Goal

RECAM combines three main ideas:

1. **Maximum fault collection through CAM reuse**
   - fault information is collected using Address CAM and Hybrid CAM;
   - additional pivot faults that would otherwise overflow the conventional CAM-based BISR can be preserved for CAM reuse.

2. **Matrix-based redundancy analysis**
   - collected fault relationships are converted into an `(Rs + Cs) × (Rs + Cs)` matrix.

3. **Fault-free-area analysis**
   - instead of sequentially checking every fault against every repair candidate, RECAM determines whether each repair candidate leaves any fault in the region that should remain fault-free.

RECAM is classified by the paper as a **dynamic BIRA**, because fault collection and repair analysis proceed concurrently and no additional post-test analysis cycles are required.

---

### 3.1 Paper-faithful baseline mode (normative for repair-rate reporting)

This project uses RECAM as a research baseline.  Therefore, the baseline result
shall reproduce the algorithm that can be derived from the published RECAM
paper; it shall **not** silently replace a RECAM failure with a physically
optimal brute-force repair.

Paper-faithful mode requires all of the following:

```text
1. consume faults in their original BIST/input order;
2. classify pivots greedily against previously collected pivots;
3. do not reorder faults, repack Address-CAM entries, or reselect pivots;
4. build the K x K RECAM matrix, K = Rs + Cs;
5. generate exactly C(K, Rs) fixed-index Row/Column candidates;
6. assign exactly one orientation to every matrix index:
       index i -> Row_i XOR Column_i;
7. accept a candidate only when every uncovered matrix cell is zero;
8. report RECAM's result even when an independent physical oracle finds a
   repair that RECAM's candidate representation cannot express.
```

The brute-force line-repair solver remains valuable, but only as a diagnostic
oracle.  It shall report algorithmic false negatives separately:

```text
paper_success
physical_oracle_success
false_negative = physical_oracle_success && !paper_success
```

It must never be used to overwrite `paper_success` when calculating the RECAM
baseline repair rate.

Source classification used by this specification:

| Rule | Paper source | Status |
|---|---|---|
| Greedy pivot/nonpivot classification | printed p. 2016, Sec. II-B | explicit |
| Address/Hybrid CAM capacities | printed p. 2017, Table I | explicit |
| Diagonal, must-line, and Hybrid matrix conversion | printed p. 2018, Figs. 6–7 | explicit |
| `C(Rs+Cs,Rs)` fixed repair lists and fault-free-area check | printed p. 2019, Fig. 8 and Sec. III-E | explicit |
| A feasible repair may be absent from those fixed lists | derived from the published representation | derived limitation, not an author statement |
| Same Address-CAM entry simultaneously row-must and column-must | not discussed | underspecified edge case |
| Repacking/re-pivoting after a must condition | not described | must not be invented in baseline mode |

---

## 4. Memory and Redundancy Model

For each repair domain/channel, define:

```text
M  = number of normal rows
N  = number of normal columns

Rs = number of spare rows
Cs = number of spare columns
```

A physical fault is represented as:

```text
Fault {
    channel
    row
    column
}
```

A single-channel implementation may omit `channel` internally, but the external interface should retain the concept if multi-channel RECAM experiments are supported.

The conventional line-repair resources are:

```text
Rs spare rows
Cs spare columns
```

RECAM additionally permits CAM entries to become online redundancy resources.

---

## 5. Core Fault Definitions

### 5.1 Pivot fault

A newly detected fault is a **pivot fault** when both its row address and its column address are unique with respect to the currently stored pivot faults.

For a new fault:

```text
f = (r, c)
```

and existing pivot set:

```text
P = {(rp, cp)}
```

the fault is pivot when:

```text
for every (rp, cp) in P:
    r != rp AND c != cp
```

Equivalently:

```text
r is absent from pivot rows
AND
c is absent from pivot columns
```

Pivot faults do not share row or column addresses with each other.

---

### 5.2 Nonpivot fault

A fault is a **nonpivot fault** when it shares at least one row or column address with an existing pivot fault.

```text
same row
OR
same column
```

Every nonpivot fault must therefore be representable relative to at least one Address CAM pivot entry.

---

## 6. Must-Repair Rules

The simulator shall implement the following mandatory-line conditions.

### 6.1 Must-repair row

If a row contains more faults than the number of available spare columns:

```text
row_fault_count > Cs
```

then that row must be repaired by a spare row.

```text
MustRepairRow(row) = true
```

### 6.2 Must-repair column

If a column contains more faults than the number of available spare rows:

```text
column_fault_count > Rs
```

then that column must be repaired by a spare column.

```text
MustRepairColumn(column) = true
```

These conditions are logical constraints on all legal repair solutions.

### 6.3 Simultaneous row-must and column-must

The Address CAM shown in Fig. 3 contains separate row-must and column-must
fields.  Under the literal threshold rules, both fields may become active for
the same pivot entry.  For example, with `Rs=Cs=1` and faults
`(0,0),(0,1),(1,0)`, row 0 and column 0 are each mandatory.

The intersection fault `(0,0)` is then covered by both selected lines.  This is
not by itself redundant-resource waste: row 0 is required for `(0,1)`, while
column 0 is required for `(1,0)`.  Suppressing either must condition would hide
a real constraint.

The paper does not specify a special repacking rule when both must bits belong
to one Address-CAM index.  Paper-faithful baseline mode therefore retains the
two must flags and the resulting fixed-list false negative described in
Section 11.2.

---

## 7. Address CAM

### 7.1 Entry count

The number of Address CAM entries is:

```text
AddressCAMEntries = Rs + Cs
```

Golden values:

| Rs | Cs | Address CAM entries |
|---:|---:|---:|
| 2 | 2 | 4 |
| 2 | 3 | 5 |
| 3 | 3 | 6 |
| 4 | 4 | 8 |
| 5 | 5 | 10 |

---

### 7.2 Offline fields

Each Address CAM entry functionally contains:

```text
enable
row_address
column_address
row_must
column_must
row_fail_count
column_fail_count
```

For an `M × N` memory, the paper describes address widths based on:

```text
row_address_bits    = log2(M)
column_address_bits = log2(N)
```

The simulator may use native integer values instead of literal packed bit fields.

---

### 7.3 Required observable state

For verification, each valid entry should expose at minimum:

```text
index
enabled
row
column
row_must
column_must
row_fail_count
column_fail_count
```

If implementation details differ internally, a debug/export function should normalize them into this logical representation.

---

## 8. Hybrid CAM

### 8.1 Entry count

The number of Hybrid CAM entries is:

```text
HybridCAMEntries =
    Rs * (Cs - 1)
  + Cs * (Rs - 1)
```

Golden values:

| Rs | Cs | Hybrid CAM entries |
|---:|---:|---:|
| 2 | 2 | 4 |
| 2 | 3 | 7 |
| 3 | 3 | 12 |
| 4 | 4 | 24 |
| 5 | 5 | 40 |

---

### 8.2 Offline function

Hybrid CAM stores nonpivot fault information relative to Address CAM entries.

The logical fields are:

```text
enable
address_cam_pointer
address_descriptor
row_or_column_address
```

The paper's worked example explicitly uses:

```text
descriptor = 0  => stored address represents a row address
```

The complementary encoding may be represented as:

```text
descriptor = 1  => stored address represents a column address
```

The implementation must keep this encoding internally consistent.

---

### 8.3 Functional representation example

If Address CAM entry 0 contains:

```text
(4, 7)
```

and a new fault is:

```text
(2, 7)
```

then column `7` already exists in the pivot entry.

The Hybrid CAM may therefore represent the nonpivot information functionally as:

```text
address_cam_pointer = 0
address_descriptor  = ROW
row_or_column_addr  = 2
```

rather than storing the full `(2, 7)` pair independently.

---

## 9. CAM-Reuse Overflow Resource

A conventional CAM-based BISR may terminate when a new pivot fault cannot be stored because the Address CAM is full.

RECAM instead permits additional pivot faults to be retained for later online CAM reuse.

The paper's fault-collection example describes additional pivot-fault storage up to a capacity equal to:

```text
Rs + Cs
```

which matches the number of Address CAM entries.

The simulator should model this as a separate logical resource, for example:

```text
cam_reuse_buffer
```

or:

```text
additional_pivot_faults
```

The exact software container is implementation-dependent.

Required behavior:

```text
new pivot
    |
    +-- Address CAM has space
    |       -> store in Address CAM
    |
    +-- Address CAM full
            |
            +-- CAM-reuse buffer has space
            |       -> store as additional CAM-repair fault
            |
            +-- CAM-reuse buffer full
                    -> unrepairable / early termination
```

### 9.1 Buffer CAM is additional-pivot-only

Fig. 4 and the text accompanying Fig. 5 explicitly describe the temporary
CAM-reuse buffer for an **additional pivot fault after Address CAM is full**.
They do not route a Hybrid-CAM-overflow nonpivot into that buffer. This project
therefore applies the following invariant in every operating mode:

```text
Buffer CAM accepts additional pivot faults only.
Hybrid CAM overflow never falls back to Buffer CAM or BUFFMAP.
```

`SharedLine_SRAM` exposes two capacity configurations without changing that
pivot-only admission rule:

```text
--paper-cam-reuse
    capacity of each local PE configuration = local Rs + local Cs
    buffer accepts additional pivots only

--buffer N
    fixed N entries per PE configuration
    buffer accepts additional pivots only

--buffer 0
    explicit zero-capacity architecture (CAM reuse disabled)
    an additional pivot or required Hybrid-CAM nonpivot with no CAM entry
    makes only that PE configuration unrepairable

no option
    fixed buffer capacity 0 (CAM reuse disabled)
```

`--paper-cam-reuse` and `--buffer` are mutually exclusive. Table-II-style
paper baseline experiments that include CAM reuse shall use
`--paper-cam-reuse`; a fixed nonzero `--buffer N` result uses a different
buffer capacity and must be reported separately. Neither mode accepts
Hybrid-overflow nonpivots.

Zero-buffer operation is a valid repair-rate experiment, not a runtime error.
An `Unrepairable fault list in buffer-disabled mode` diagnostic means that the
current PE/spare configuration encountered either an additional pivot with no
Buffer entry or a required nonpivot with no Hybrid entry. The simulator must
reject that configuration, continue testing the remaining configurations and
patterns, and calculate the repair rate normally. Successful zero-buffer
remap options must contain no
`BUFFMAP` records and must still pass independent MAP coverage validation.

```bash
make sl_sram_b
make sl_sram_r s=5 paperCamReuse=1

# Explicit buffer-disabled architecture:
make sl_sram_r s=5 buffer=0
make validate_remap
```


## 9.2 Nonpivot Fault Capacity and Must-Repair Behavior

This subsection records the nonpivot-fault behavior used by this project when reproducing RECAM.

### A. Hybrid CAM full before must-repair

A nonpivot fault normally matches row or column information already represented in the Address CAM and is recorded through the Hybrid CAM.

If all conditions below hold:

```text
1. A newly detected fault is nonpivot.
2. It does not trigger MustRepairRow or MustRepairColumn.
3. Its required information is not already represented.
4. Hybrid CAM has no free entry.
```

this project classifies the repair attempt as `UNREPAIRABLE`.

```text
New nonpivot fault
        |
        v
Update fault count and check must-repair
        |
       NO
        |
        v
Hybrid CAM entry required
        |
        v
Free Hybrid CAM entry?
       / \
     YES  NO
      |    |
      v    v
   Store  UNREPAIRABLE
```

**Source boundary:** the RECAM paper explicitly states that matching/nonpivot faults are updated in Hybrid CAM, but it does not explicitly define a `Hybrid CAM full` branch. The temporary CAM-reuse buffer is demonstrated for additional **pivot faults** after Address CAM becomes full. Therefore, `Hybrid CAM full + required nonpivot + no must-repair => UNREPAIRABLE` is a **conservative project rule for an underspecified paper case**, not an explicitly stated transition in the paper.

### B. Nonpivot fault that triggers must-repair

Before allocating a new Hybrid CAM entry, update the relevant fault count and evaluate:

```text
row_fault_count > Cs    => MustRepairRow
column_fault_count > Rs => MustRepairColumn
```

If the newly detected nonpivot fault causes a must-repair condition, activate the corresponding must flag. The triggering fault does **not** require a new Hybrid CAM entry.

After a line becomes must-repair, subsequent faults on that line do not require additional Hybrid CAM entries for the covered dimension.

#### Example

```text
Rs = 2
Cs = 2

(3,3) -> pivot
         Address CAM stores (3,3)

(3,2) -> nonpivot
         same row as pivot (3,3)
         required information is stored in Hybrid CAM

(3,1) -> nonpivot
         row_fault_count[3] = 3
         3 > Cs (=2)

         therefore:
             Row 3 -> MUST-REPAIR
             RowMust = 1

         (3,1) does not consume a new Hybrid CAM entry.

(3,0) -> later fault on Row 3
         Row 3 is already must-repair.
         No new Hybrid CAM entry is required.
```

### C. Clear existing Hybrid CAM entries after must-repair

The paper states that once the must-repair condition is satisfied, there is no need to continue updating individual fault information on the corresponding line.

The paper does **not** explicitly state whether entries allocated before the
must-repair transition are reclaimed. This project defines reclamation as a
simulator rule and applies it in every buffer mode.

For the example:

```text
Before (3,1):
Address CAM: (3,3), RowMust = 0
Hybrid CAM:  information for (3,2)

After (3,1):
Address CAM: (3,3), RowMust = 1
Hybrid CAM:  information for Row 3, including (3,2), is cleared
```

The triggering fault must be evaluated for must-repair **before** testing
Hybrid CAM free capacity. If it triggers must-repair, it does not allocate a
new entry, and the controller removes existing entries for the same Address-CAM
pointer and covered direction:

```text
RowMust at pointer i:
    remove entries with pointer == i and descriptorRowIsDiff == false

ColMust at pointer i:
    remove entries with pointer == i and descriptorRowIsDiff == true
```

Entries for the other direction are not cleared. Consequently, a fault which
triggers must-repair succeeds even if Hybrid CAM was full immediately before
that fault arrived. Later faults on the must line require no Hybrid entry.

### Summary

```text
Nonpivot triggers must-repair:
    -> set must flag
    -> triggering fault consumes no new Hybrid CAM entry

Later faults on the must-repair line:
    -> consume no new Hybrid CAM entries

Previously allocated Hybrid CAM entries:
    -> clear entries for the newly mandatory line and direction
    -> apply this project rule in every buffer mode

Hybrid CAM full + new required nonpivot + no must-repair:
    -> UNREPAIRABLE in this project's conservative model
    -> project policy, not paper-explicit behavior
```

### D. BUFFER=0 and solution-matrix evaluation

With `BUFFER=0`, Address CAM and Hybrid CAM remain separate resources. When
there is no Address-CAM pivot overflow and no required Hybrid-CAM nonpivot
overflow, the simulator builds the matrix, checks all `C(Rs+Cs, Rs)` paper
candidates, and determines success from `validSolList`.

If either CAM overflows, the driver still invokes matrix generation and
candidate comparison for diagnostic consistency, but the final result is
forced to failure:

```text
RepairSuccess = isRepairable && !validSolList.empty()
```

Therefore, `total_faults <= Address_capacity + Hybrid_capacity` is not
sufficient. Pivots cannot consume unused Hybrid entries, and nonpivots cannot
consume unused Address or Buffer entries.


### E. Expanded nonpivot walkthrough (informative)

This section defines the handling of non-pivot faults in the RECAM simulator, especially the behavior when the Hybrid CAM has insufficient capacity and when a non-pivot fault triggers a must-repair condition.

#### E.1 Normal Non-Pivot Fault Handling

When a newly detected fault shares a row or column address with an existing pivot fault in the Address CAM, the fault is classified as a **non-pivot fault**.

If the fault does not trigger a must-repair condition, its required fault information is stored in the Hybrid CAM.

```text
New fault
    |
    v
Matches row/column in Address CAM?
    |
   YES
    |
    v
Non-pivot fault
    |
    v
Check must-repair condition
    |
   NO
    |
    v
Store required fault information
in Hybrid CAM
````

---

#### E.2 Hybrid CAM Full

If a newly detected non-pivot fault:

1. requires a new Hybrid CAM entry,
2. does not trigger `MustRepairRow` or `MustRepairColumn`, and
3. no free Hybrid CAM entry remains,

the simulator shall classify the current memory as:

```text
UNREPAIRABLE
```

The simulator behavior is therefore:

```text
New non-pivot fault
        |
        v
Update fault count
        |
        v
Triggers must-repair?
       / \
     YES  NO
      |    |
      |    v
      |   Hybrid CAM entry required
      |          |
      |          v
      |   Free Hybrid CAM entry?
      |        /     \
      |      YES      NO
      |       |        |
      |       v        v
      |     Store   UNREPAIRABLE
      |
      v
Set must-repair flag
```

**Paper interpretation note:**
The RECAM paper explicitly states that non-pivot faults are updated in the Hybrid CAM. However, it does not explicitly describe a `Hybrid CAM full` branch in the repair-analysis flowchart.

The temporary buffer introduced for CAM reuse is explicitly used for
additional **pivot faults** when the Address CAM has no remaining capacity.
Therefore, every simulator mode reserves Buffer CAM for additional pivots and
does not use it for Hybrid CAM overflow.

Accordingly:

```text
Hybrid CAM full
+
new non-pivot requires storage
+
no must-repair condition
=
UNREPAIRABLE
```

is used as the **conservative baseline behavior of this simulator** for a case that is not explicitly specified by the paper.

---

#### E.3 Non-Pivot Fault Triggering Must-Repair

Before allocating a new Hybrid CAM entry, the simulator shall update the corresponding row and column fault counts and evaluate the must-repair conditions.

For a row:

```text
row_fault_count > Cs
    => MustRepairRow
```

For a column:

```text
column_fault_count > Rs
    => MustRepairColumn
```

If the newly detected non-pivot fault causes one of these conditions to become true, the corresponding must-repair flag is activated.

The triggering fault does **not** need to be stored as a new Hybrid CAM entry because the entire corresponding row or column is now guaranteed to be repaired by a spare line.

Furthermore, once a row or column has been marked as must-repair, subsequent faults belonging to that already-covered line do not require additional Hybrid CAM entries.

---

#### E.4 Example

Assume:

```text
Rs = 2
Cs = 2
```

and the following fault sequence:

```text
(3,3)
(3,2)
(3,1)
(3,0)
```

#### Fault 1: `(3,3)`

There is no existing pivot with row 3 or column 3.

Therefore:

```text
(3,3) -> Pivot
```

and it is stored in the Address CAM:

```text
Address CAM:

Row   Column   RowMust
3       3         0
```

---

#### Fault 2: `(3,2)`

Row 3 already exists in the Address CAM.

Therefore:

```text
(3,2) -> Non-pivot
```

The row currently contains:

```text
(3,3)
(3,2)
```

so:

```text
row_fault_count[3] = 2
```

Since:

```text
2 <= Cs
```

Row 3 has not yet reached the must-repair condition.

The required information for `(3,2)` is therefore stored in the Hybrid CAM.

Conceptually:

```text
Address CAM:
    (3,3), RowMust = 0

Hybrid CAM:
    (3,2)
```

---

#### Fault 3: `(3,1)`

The new fault is also a non-pivot fault because Row 3 is already represented by pivot `(3,3)`.

After detecting the fault:

```text
row_fault_count[3] = 3
```

Since:

```text
3 > Cs
```

the row must be repaired using a spare row:

```text
MustRepairRow(3) = true
```

The corresponding Address CAM state becomes:

```text
Address CAM:
    (3,3), RowMust = 1
```

Because the entire Row 3 is now guaranteed to be repaired by a spare row, `(3,1)` does not require a new Hybrid CAM entry.

Therefore:

```text
(3,1)
    |
    v
Non-pivot
    |
    v
row_fault_count[3] = 3
    |
    v
3 > Cs
    |
    v
RowMust = 1
    |
    v
Do NOT allocate a new Hybrid CAM entry
```

---

#### Fault 4: `(3,0)`

Row 3 is already marked:

```text
RowMust = 1
```

Therefore, `(3,0)` is already covered by the spare row assigned to Row 3.

No additional Hybrid CAM entry is required.

```text
(3,0)
    |
    v
Row 3 already MustRepair
    |
    v
No Hybrid CAM allocation
```

Thus, once a line becomes must-repair, later faults on the same line do not continue consuming Hybrid CAM capacity.

---

#### E.5 Existing Hybrid CAM Entries After Must-Repair

In the previous example, `(3,2)` was already stored in the Hybrid CAM before `(3,1)` caused Row 3 to become must-repair.

The RECAM paper states that once the must-repair condition is satisfied, individual fault information on that line no longer needs to be updated.

The paper does not explicitly state whether previously allocated Hybrid CAM
entries are reclaimed. This simulator applies the Section 9.2-C project rule
in every mode:

```text
Before detecting (3,1):

Address CAM:
    (3,3), RowMust = 0

Hybrid CAM:
    (3,2)


After detecting (3,1):

Address CAM:
    (3,3), RowMust = 1

Hybrid CAM:
    entries for Row 3, including (3,2), are cleared

(3,1):
    no new Hybrid CAM entry

Later (3,x) faults:
    no new Hybrid CAM entries
```

The must-triggering fault is evaluated before capacity allocation, so this
transition succeeds even if Hybrid CAM was full before `(3,1)` arrived.

---

#### E.6 Summary of Simulator Rules

```text
Case 1:
Non-pivot
+ no must-repair
+ Hybrid CAM has free entry

    -> Store in Hybrid CAM


Case 2:
Non-pivot
+ no must-repair
+ Hybrid CAM full

    -> UNREPAIRABLE
       [Project conservative rule;
        paper does not explicitly define this overflow case]


Case 3:
Non-pivot
+ triggers must-repair

    -> Set RowMust / ColMust
    -> Do not allocate a new Hybrid CAM entry for the triggering fault


Case 4:
Fault occurs on an already must-repair line

    -> Do not allocate a new Hybrid CAM entry


Case 5:
Hybrid CAM entry was allocated before the line became must-repair

    -> Clear entries for that pointer and must direction
    -> Apply this rule in every buffer mode
```

Case 3 is supported by Fig. 5's statement that individual fault information on
a must-repair line no longer needs updating. Clearing already allocated entries
is this project's implementation rule because the paper does not explicitly
specify reclamation. Case 2 must likewise retain the `project conservative
rule` label because the paper does not explicitly define a Hybrid-CAM-overflow
branch.













---

## 10. Matrix Conversion

### 10.1 Matrix size

The redundancy-analysis matrix shall have dimensions:

```text
K = Rs + Cs

matrix[K][K]
```

Therefore:

```text
MatrixSize = (Rs + Cs)^2
```

Examples:

```text
Rs = 2, Cs = 2 -> 4 × 4 -> 16 signals
Rs = 3, Cs = 3 -> 6 × 6 -> 36 signals
Rs = 4, Cs = 4 -> 8 × 8 -> 64 signals
```

---

### 10.2 Matrix meaning

Rows and columns of this matrix correspond to row/column addresses represented by Address CAM entries.

The matrix is a compact representation of fault relationships, not the original physical `M × N` memory bitmap.

The simulator shall preserve enough information that all collected faults are represented by the matrix.

---

### 10.3 Pivot mapping

Enabled Address CAM pivot entries activate diagonal positions according to Address CAM entry order.

For enabled pivot entries:

```text
AddressCAM[0]
AddressCAM[1]
AddressCAM[2]
...
```

the corresponding matrix positions include:

```text
matrix[0][0] = 1
matrix[1][1] = 1
matrix[2][2] = 1
...
```

---

### 10.4 Must-repair mapping

If a pivot-associated row is marked must-repair, the corresponding matrix row is fully activated.

Conceptually:

```text
must row i
=> matrix[i][*] = 1
```

If a pivot-associated column is marked must-repair, the corresponding matrix column is fully activated.

Conceptually:

```text
must column j
=> matrix[*][j] = 1
```

The actual index mapping shall follow the Address CAM entry ordering used by the implementation.

---

### 10.5 Hybrid CAM contribution

Hybrid CAM entries contribute matrix activations according to:

```text
address_cam_pointer
address_descriptor
stored row/column address
```

The paper classifies Hybrid-CAM matrix conversion into multiple cases depending on whether:

1. free Address CAM positions remain;
2. both row and column correspond to Address CAM information;
3. only the row or only the column corresponds to Address CAM information.

The simulator must reproduce the final logical matrix implied by all collected faults.

A useful implementation invariant is:

```text
Every collected non-CAM-reuse fault must be represented by at least one active matrix relationship.
```

---

## 11. Repair Solution Space

The number of row/column repair-allocation patterns considered by RECAM is:

```text
NumSolutions =
    (Rs + Cs)! / (Rs! * Cs!)
```

Equivalent form:

```text
NumSolutions = C(Rs + Cs, Rs)
```

Examples:

```text
Rs = 2, Cs = 2 -> 6
Rs = 3, Cs = 3 -> 20
Rs = 4, Cs = 4 -> 70
```

For `Rs = Cs = 2`, the six abstract resource patterns are:

```text
RRCC
RCRC
RCCR
CRRC
CRCR
CCRR
```

where `R` denotes a row-repair choice and `C` denotes a column-repair choice.

### 11.1 Exact candidate semantics and numbering

For every logical index `i`, a candidate chooses exactly one of:

```text
Row_i
Column_i
```

It cannot choose both orientations of one index and leave another index
unused.  In the implementation, solution indices are zero-based while Fig. 8
is one-based:

```text
implementation index 0 == paper Fig. 8 solution #1
implementation index 5 == paper Fig. 8 solution #6   (Rs=Cs=2)
```

Changing candidate order cannot change repairability because every candidate
is checked, but it changes logged `solution_index` values and which valid
remap option is emitted first.  Logs generated with the previous reversed
numbering must not be interpreted using the new numbering.

### 11.2 Required derived false negative

Configuration and input order:

```text
Rs = Cs = 1
faults = (0,0), (0,1), (1,0)
```

Literal paper-derived collection gives one greedy pivot `(0,0)`, with both
row-must 0 and column-must 0.  Figs. 6–7 therefore imply:

```text
M = [1 1]
    [1 0]
```

Fig. 8 gives only `C(2,1)=2` candidates:

| Candidate | Uncovered matrix cell | Result |
|---|---|---|
| `Row_0 + Column_1` | `M[1][0] = 1` | invalid |
| `Column_0 + Row_1` | `M[0][1] = 1` | invalid |

The physically feasible repair `physical row 0 + physical column 0` is absent
because both physical lines belong to Address-CAM/matrix index 0.  This is a
**derived limitation** of the published fixed-index representation; the paper
does not state that this example is unrepairable and does not discuss this
`Rs=Cs=1` corner case.

The same fault set is order-sensitive.  Across its six input permutations,
the literal greedy implementation produces two failures when `(0,0)` is first
and four successes otherwise.  Consequently, paper-faithful repair-rate runs
must preserve the generated fault arrival order and must not sort a fault set
before classification.

Regression contract:

```text
paper RECAM result  = unrepairable
physical oracle     = repairable by row 0 + column 0
classification      = expected paper-derived false negative
```

---

## 12. Fault-Free-Area Analysis

### 12.1 Core principle

RECAM does not need to sequentially prove that each selected spare line covers every fault.

Instead, for each candidate repair solution, it checks the region that is **not covered** by the selected row and column repairs.

A candidate is valid if the uncovered region contains no represented fault.

---

### 12.2 Functional software equivalent

For each repair candidate:

```text
selected_rows
selected_columns
```

define:

```text
uncovered_positions
```

Then:

```text
candidate is valid
IFF
all matrix values in uncovered_positions are 0
```

Equivalent pseudocode:

```python
for solution in all_repair_solutions:
    uncovered = get_uncovered_matrix_positions(solution)

    if all(matrix[r][c] == 0 for r, c in uncovered):
        return REPAIRABLE, solution

return UNREPAIRABLE
```

The paper's hardware performs these checks using combinational logic / NOR logic in parallel.

Therefore, software loop iterations must **not** be interpreted as RECAM hardware analysis cycles.

---

## 13. Example: `Rs = 2`, `Cs = 2`

Suppose one candidate selects:

```text
repair Row0
repair Row1
repair Col2
repair Col3
```

Then the area that must remain fault-free is:

```text
(2,0)
(2,1)
(3,0)
(3,1)
```

If:

```text
matrix[2][0] == 0
matrix[2][1] == 0
matrix[3][0] == 0
matrix[3][1] == 0
```

then this candidate is a valid repair solution.

---

## 14. Offline RECAM Functional Flow

The implemented simulator should be behaviorally equivalent to the following flow.

```text
START
  |
  v
Select repair domain / channel
  |
  v
Receive next fault
  |
  v
Check Address CAM row/column match
  |
  +------------------------------+
  |                              |
 match                           no match
  |                              |
  v                              v
nonpivot                    new pivot candidate
  |                              |
  v                              v
update Hybrid CAM         Address CAM has space?
                                 |
                         +-------+-------+
                         |               |
                        yes              no
                         |               |
                         v               v
                 store Address CAM   CAM-reuse buffer
                                        available?
                                         |
                                   +-----+-----+
                                   |           |
                                  yes          no
                                   |           |
                                   v           v
                              store extra   early terminate /
                              pivot fault   unrepairable
                                   |
                                   v
                        matrix / analyzer update
                                   |
                                   v
                        fault-free-area analysis
                                   |
                                   v
                         legal RA solution?
                            |            |
                           yes           no
                            |            |
                            v            v
                       repairable    unrepairable
```

The implementation may organize the control flow differently, provided the observable behavior is equivalent.

---

## 15. Online CAM Reuse

This section is required only if the simulator models RECAM runtime behavior in addition to repair rate.

### 15.1 Power-up

Fault addresses assigned to CAM redundancy are loaded from eFuse information into Address CAM.

```text
eFuse
  ->
Address CAM
```

---

### 15.2 Write

For an incoming write:

```text
(address, data)
```

Address CAM is searched in parallel with normal DRAM operation.

If the address matches a CAM-repair entry:

```text
Hybrid CAM[pointer] = input_data
```

If no match occurs:

```text
normal DRAM write behavior
Hybrid CAM remains inactive
```

---

### 15.3 Read

For an incoming read:

```text
address
```

if Address CAM matches:

```text
Address CAM match
    ->
pointer
    ->
Hybrid CAM data
    ->
output MUX
    ->
host
```

The Hybrid CAM value replaces the data read from the faulty DRAM location.

If there is no Address CAM match:

```text
normal DRAM data
    ->
output MUX
    ->
host
```

### 15.4 Remap log contract

A successful `OPTION` in `reports/legacy/sharedline_sram/RemapTable.txt` must provide
an independently executable repair description for every fault in that
pattern.  Two record types have different coverage semantics.

Spare-line mapping:

```text
MAP <HBMID> <ChannelID> <BankID> <SubarrayGroupID> <SubarrayID>
    <r> <c> <R|C> <new_address> <latency>
```

`MAP ... R ...` covers every fault with the same first five domain fields and
the same source row.  `MAP ... C ...` analogously covers the complete source
column.  The other anchor coordinate does not restrict a line match.

Exact-address CAM-reuse mapping:

```text
BUFFMAP <HBMID> <ChannelID> <BankID> <SubarrayGroupID> <SubarrayID>
        <r> <c> <latency>
```

`BUFFMAP` covers exactly one address: all seven address fields must match.  It
does not consume a spare row/column and has no `new_address`, because online
CAM reuse serves data through Address CAM, Hybrid CAM, and the output MUX
(Figs. 11–13).  The implementation uses 3 cycles as the conservative endpoint
of the paper's reported 2–3-cycle online CAM path (Sec. IV-D).  This latency is
distinct from the simulator's project-specific spare-line latency.

Remap generation must preserve physical address provenance for every logical
matrix row and column:

```text
matrixRowAddresses[i]
matrixColumnAddresses[i]
```

This includes free matrix slots populated by Hybrid-CAM Case 1 in Fig. 7(c).
Walking only the Address CAM when emitting `MAP` is incorrect: a valid solution
may select a Hybrid-extended row/column which otherwise disappears from the
log.

Every generated remap log shall be checked by the independent coverage oracle:

```text
validateRemapCoverage(fault_file, remap_file, optional_pattern_id)
```

For SharedLine input, four consecutive FaultLists form one pattern:

```text
pattern p, PE i = faultLists[p * 4 + i]
```

The validator must not trust `RepairSuccess`.  For each logged option it checks
that every original fault is covered by a same-domain row/column `MAP` or an
exact-address `BUFFMAP`, and also checks PE resource counts and conflicting
destinations.

Reference invocation:

```bash
make validate_remap_b
./build/bin/validate_remap \
  fault_generator/faults.faults \
  reports/legacy/sharedline_sram/RemapTable.txt

# Validate only one pattern ID:
./build/bin/validate_remap \
  fault_generator/faults.faults \
  reports/legacy/sharedline_sram/RemapTable.txt 66
```

---

## 16. Recommended Simulator Data Model

The existing implementation does not need to use these exact class names. Codex should map existing structures to the following logical model before proposing changes.

```text
RECAM
|
+-- Configuration
|   +-- M
|   +-- N
|   +-- Rs
|   +-- Cs
|   +-- channels
|
+-- Fault state
|   +-- detected_faults
|   +-- row_fault_counts
|   +-- column_fault_counts
|
+-- AddressCAM
|   +-- entries[Rs + Cs]
|
+-- HybridCAM
|   +-- entries[Rs(Cs-1) + Cs(Rs-1)]
|
+-- CAMReuseBuffer
|   +-- additional_pivot_faults
|
+-- Matrix
|   +-- dimension = Rs + Cs
|
+-- RepairAnalyzer
|   +-- candidate_solutions
|   +-- valid_solution
|
+-- Result
    +-- repairable
    +-- spare_rows
    +-- spare_columns
    +-- cam_repair_faults
```

---

## 17. Required Debug / Inspection Output

Because the architecture is already implemented, validation requires visibility into intermediate states.

For deterministic tests, the simulator should be able to emit:

```text
fault sequence index
fault address
fault classification
matched Address CAM entry
Address CAM state
Hybrid CAM state
row fault count
column fault count
must-row flags
must-column flags
CAM-reuse buffer state
matrix state
candidate repair result
final spare-row allocation
final spare-column allocation
CAM-repair faults
repairable / unrepairable
```

Recommended machine-readable dump:

```json
{
  "fault_index": 0,
  "fault": {"row": 0, "column": 0},
  "classification": "pivot",
  "address_cam": [],
  "hybrid_cam": [],
  "must_rows": [],
  "must_columns": [],
  "cam_reuse_faults": [],
  "matrix": [],
  "repairable": null
}
```

The exact JSON schema may follow the existing project structure.

---

# 18. Verification Strategy

Verification shall be performed in four layers.

```text
Layer 1: Structural invariants
Layer 2: Deterministic RECAM examples
Layer 3: Exhaustive optimality oracle
Layer 4: Monte Carlo paper reproduction
```

The simulator should not be considered validated solely because a final repair-rate curve looks similar to the paper.

---

## 19. Layer 1 — Structural Unit Tests

### TEST-01: Address CAM capacity

Verify:

```text
AddressCAMEntries == Rs + Cs
```

Required cases:

```text
(2,2) -> 4
(2,3) -> 5
(3,3) -> 6
(4,4) -> 8
(5,5) -> 10
```

---

### TEST-02: Hybrid CAM capacity

Verify:

```text
HybridCAMEntries ==
    Rs*(Cs-1) + Cs*(Rs-1)
```

Required cases:

```text
(2,2) -> 4
(2,3) -> 7
(3,3) -> 12
(4,4) -> 24
(5,5) -> 40
```

---

### TEST-03: Matrix dimension

Verify:

```text
matrix dimension == Rs + Cs
matrix signal count == (Rs + Cs)^2
```

Required cases:

```text
(2,2) -> 4 × 4
(3,3) -> 6 × 6
(4,4) -> 8 × 8
```

---

### TEST-04: Repair-solution count

Verify:

```text
NumSolutions = C(Rs + Cs, Rs)
```

Required cases:

```text
(2,2) -> 6
(3,3) -> 20
(4,4) -> 70
```

---

## 20. Layer 2 — Fault Classification Tests

### TEST-05: First fault is pivot

Input:

```text
(4,7)
```

Expected:

```text
classification = pivot
Address CAM contains (4,7)
```

---

### TEST-06: Shared column becomes nonpivot

Initial Address CAM:

```text
(4,7)
```

Input:

```text
(2,7)
```

Expected:

```text
classification = nonpivot
matched column = 7
Hybrid CAM receives relationship information
```

---

### TEST-07: Shared row becomes nonpivot

Initial Address CAM:

```text
(3,3)
```

Input:

```text
(3,2)
```

Expected:

```text
classification = nonpivot
matched row = 3
```

---

### TEST-08: Unique row and column becomes pivot

Existing pivots:

```text
(4,7)
(5,4)
```

Input:

```text
(3,3)
```

Expected:

```text
classification = pivot
```

---

## 21. Layer 2 — Must-Repair Tests

### TEST-09: Must-repair row

Configuration:

```text
Rs = 2
Cs = 2
```

Faults:

```text
(3,3)
(3,2)
(3,1)
```

Expected:

```text
row_fault_count[3] = 3
3 > Cs
MustRepairRow(3) = true
```

---

### TEST-10: Must-repair column

Configuration:

```text
Rs = 2
Cs = 2
```

Faults:

```text
(1,5)
(2,5)
(3,5)
```

Expected:

```text
column_fault_count[5] = 3
3 > Rs
MustRepairColumn(5) = true
```

---

## 22. Layer 2 — Paper Fig. 5 Golden Sequence

This is the highest-priority deterministic regression test.

Configuration:

```text
Memory = 8 × 8
Rs = 2
Cs = 2
```

Fault sequence:

```text
#1  (4,7)
#2  (2,7)
#3  (5,4)
#4  (3,3)
#5  (3,2)
#6  (3,1)
#7  (1,0)
#8  (7,5)
#9  (6,6)
#10 (3,6)
```

Expected qualitative transitions include:

```text
(4,7) -> pivot
(2,7) -> nonpivot because column 7 is already represented

(5,4) -> pivot
(3,3) -> pivot

(3,2) -> nonpivot because row 3 is represented
(3,1) -> causes row 3 to satisfy must-repair condition

(1,0) -> pivot

Address CAM eventually becomes full

additional pivot faults:
(7,5)
(6,6)
are retained for CAM reuse rather than immediately causing
conventional Address-CAM-overflow failure
```

The final internal state should be compared against Fig. 5 of the paper, especially:

```text
Address CAM contents
Hybrid CAM contents
must-repair state
temporary CAM-reuse buffer
```

This test should run with deterministic insertion order.

---

## 23. Layer 2 — Matrix Conversion Tests

### TEST-11: Pivot diagonal activation

Given `k` enabled Address CAM pivot entries:

```text
matrix[i][i] == 1
for each enabled pivot index i
```

before additional must-repair or Hybrid CAM contributions are applied.

---

### TEST-12: Must-row activation

If the Address CAM entry associated with a row is marked must-row:

```text
corresponding logical matrix row is fully active
```

---

### TEST-13: Must-column activation

If the Address CAM entry associated with a column is marked must-column:

```text
corresponding logical matrix column is fully active
```

---

### TEST-14: Matrix reconstructability

For a deterministic fault set small enough to inspect manually:

```text
Every collected fault that is handled by spare-row/spare-column
analysis must influence the final matrix representation.
```

No collected non-CAM-reuse fault may disappear from the logical analysis state.

---

## 24. Layer 2 — Fault-Free-Area Tests

### TEST-15: Valid candidate

Configuration:

```text
Rs = 2
Cs = 2
```

Candidate:

```text
Rows = {0,1}
Cols = {2,3}
```

Uncovered matrix positions:

```text
(2,0)
(2,1)
(3,0)
(3,1)
```

Set all four to zero.

Expected:

```text
candidate = valid
```

---

### TEST-16: Invalid candidate

Use the same candidate, but set:

```text
matrix[3][1] = 1
```

Expected:

```text
candidate = invalid
```

---

## 25. Layer 3 — Brute-Force Optimal Repairability Oracle

A separate, intentionally simple brute-force solver should be used as a correctness oracle for **conventional spare-row/spare-column repairability**.

For each small fault map, enumerate all allowed combinations of up to:

```text
Rs rows
Cs columns
```

and determine whether every fault is covered.

Reference pseudocode:

```python
def brute_force_repairable(faults, Rs, Cs):
    candidate_rows = unique_fault_rows(faults)
    candidate_cols = unique_fault_cols(faults)

    for rows in all_subsets(candidate_rows, max_size=Rs):
        for cols in all_subsets(candidate_cols, max_size=Cs):
            if all(
                fault.row in rows or fault.column in cols
                for fault in faults
            ):
                return True, rows, cols

    return False, None, None
```

Important:

This oracle validates the line-repair portion.

It does **not** define the expected paper-faithful RECAM answer.  A
brute-force/RECAM disagreement must first be classified:

```text
RECAM=true,  oracle=false -> false positive / representation bug
RECAM=false, oracle=true  -> possible paper-algorithm false negative
RECAM=oracle              -> agreement
```

Expected derived false negatives, including Section 11.2, are regression
targets for the baseline rather than failures to be repaired.  New
disagreements must be dumped and reviewed to determine whether they arise from
the published fixed-list algorithm, an underspecified convention, or an
implementation defect.

CAM-reuse faults must be handled according to the RECAM resource model before comparing final RECAM repairability.

A recommended comparison is:

```text
RECAM conventional-line subproblem
vs.
brute-force optimum
```

and separately:

```text
final RECAM repairability
=
line-repair solution
+
valid CAM-reuse assignment
```

---

## 26. Exhaustive Small-Memory Validation

Recommended configurations:

```text
Memory = 4 × 4
Rs = 1
Cs = 1

Memory = 5 × 5
Rs = 1
Cs = 2

Memory = 5 × 5
Rs = 2
Cs = 1

Memory = 6 × 6
Rs = 2
Cs = 2
```

For each configuration:

1. enumerate or randomly generate many small fault sets;
2. compare RECAM's line-repair decision with the brute-force oracle;
3. separately verify CAM-reuse capacity and assignment;
4. classify every disagreement as false positive, expected/derived false
   negative, underspecified case, or implementation defect;
5. preserve paper-derived false negatives in baseline repair-rate results;
6. dump the complete internal RECAM state for every new disagreement.

Recommended failure report:

```text
seed
configuration
fault list
pivot sequence
Address CAM
Hybrid CAM
must rows
must columns
CAM reuse buffer
matrix
RECAM result
brute-force result
```

---

# 27. Layer 4 — Paper Monte Carlo Reproduction

The paper evaluates repair rate using:

```text
10,000 simulated memories
memory size = 1024 × 1024
```

and varies the number of faults with:

```text
Rs,Cs = [2,2]
Rs,Cs = [3,3]
Rs,Cs = [4,4]
```

The paper states that the fault distribution is based on the ITC'2020 RA Benchmarks and uses:

```text
20% probability: cluster fault after the previous fault
30% probability: line fault
remaining probability: random fault
```

Important limitation:

The RECAM paper does not, by itself, provide enough detail to reconstruct every implementation detail of the ITC'2020 benchmark generator from only these percentages.

Therefore:

```text
Do not treat a Table-II mismatch as proof that RECAM logic is wrong
until the benchmark fault generator is also verified.
```

If exact reproduction is required, the project should implement or import the fault-generation procedure from the cited ITC'2020 RA Benchmark source.

---

## 28. Paper Table-II Regression Targets

### 28.1 `Rs = Cs = 2`

| Fault count | RECAM repair rate |
|---:|---:|
| 4 | 100.0% |
| 5 | 100.0% |
| 6 | 99.9% |
| 7 | 99.7% |
| 8 | 99.5% |
| 9 | 96.7% |
| 10 | 86.3% |
| 11 | 64.9% |
| 12 | 41.1% |

### 28.2 `Rs = Cs = 3`

| Fault count | RECAM repair rate |
|---:|---:|
| 6 | 100.0% |
| 7 | 100.0% |
| 8 | 100.0% |
| 9 | 100.0% |
| 10 | 99.9% |
| 11 | 99.9% |
| 12 | 99.9% |
| 13 | 99.5% |
| 14 | 97.4% |

### 28.3 `Rs = Cs = 4`

| Fault count | RECAM repair rate |
|---:|---:|
| 8 | 100.0% |
| 9 | 100.0% |
| 10 | 100.0% |
| 11 | 100.0% |
| 12 | 100.0% |
| 13 | 100.0% |
| 14 | 100.0% |
| 15 | 100.0% |
| 16 | 100.0% |

These values are statistical regression targets, not deterministic unit-test expectations.

---

## 29. Repair Rate Definition

Use:

```text
repair_rate =
    successfully_repaired_memories
    / tested_memories
```

or, in percent:

```text
repair_rate_percent =
    100 * successfully_repaired_memories
        / tested_memories
```

For multi-channel experiments, follow the paper's criterion:

```text
a memory is counted as repaired only when all channels are repairable
```

---

## 30. Randomness and Reproducibility

Every Monte Carlo experiment shall record:

```text
random seed
trial count
memory dimensions
Rs
Cs
channel count
fault count
fault model
CAM-reuse capacity
```

Recommended CLI-style configuration:

```text
--seed
--trials
--rows
--cols
--spare-rows
--spare-cols
--channels
--fault-count
--fault-model
--enable-cam-reuse
```

A published simulation result should always be reproducible from the recorded seed and configuration.

---

## 31. Required Metrics

At minimum, report:

```text
total_trials
repairable_trials
unrepairable_trials
repair_rate
```

Recommended additional metrics:

```text
average_pivot_faults
average_nonpivot_faults
average_address_cam_usage
average_hybrid_cam_usage
average_cam_reuse_usage

address_cam_overflow_count
cam_reuse_overflow_count

must_row_count
must_column_count

repair_by_spares_only_count
repair_using_cam_reuse_count
```

For debugging:

```text
first_failure_seed
first_failure_fault_map
```

---

## 32. Optional Online-Mode Verification

If online CAM reuse is implemented, add the following tests.

### TEST-17: CAM-repair write hit

Given CAM-repair address:

```text
(7,5)
```

write:

```text
10101010
```

Expected:

```text
Address CAM match
Hybrid CAM data updated
```

---

### TEST-18: CAM-repair read hit

After TEST-17:

```text
read (7,5)
```

Expected:

```text
output == 10101010
source == Hybrid CAM
```

---

### TEST-19: Normal DRAM miss path

Use non-CAM-repair address:

```text
(4,3)
```

Expected:

```text
Address CAM mismatch
Hybrid CAM inactive
normal DRAM output selected
```

---

### TEST-20: Multiple CAM-repair entries

Use:

```text
(7,5) -> 10101010
(6,6) -> 00001111
```

Expected:

```text
read (7,5) -> 10101010
read (6,6) -> 00001111
```

with independent pointer resolution.

---

## 33. Timing Interpretation

The paper reports zero **additional repair-analysis clock cycles** for RECAM because repair analysis is performed concurrently with fault collection.

Therefore:

```text
software execution time != RECAM analysis cycles
```

and:

```text
Python/C++ loop count != RECAM hardware clock cycles
```

A software simulator may iterate through candidate repair solutions sequentially while still modeling the logical behavior of the hardware-parallel RECAM analyzer.

Do not claim cycle-accurate reproduction unless the simulator explicitly models the parallel hardware structure.

---

## 34. Hardware-Area Interpretation

The paper synthesizes the proposed hardware using a 32-nm Synopsys library and reports hardware area in NAND-gate-equivalent terms.

The paper reports approximately:

```text
3% gate-count increase
```

for enabling CAM reuse relative to RECAM without CAM reuse in the reported configuration.

This number should be treated as a paper result, not as an intrinsic constant of all RECAM implementations.

A software simulator should not derive physical area from data-structure sizes unless a separate hardware cost model is implemented.

---

## 35. Online Timing Result from the Paper

The paper reports that online address matching and data transfer can be completed in approximately:

```text
2–3 clock cycles
```

and reports an output-MUX delay of approximately:

```text
30 ps
```

for the paper's synthesized configuration using a 32-nm Synopsys library.

These values are reference results only.

Do not hardcode them as architecture-independent constants.

---

# 36. Acceptance Criteria

The RECAM implementation is considered functionally validated when all of the following hold.

## Mandatory

- [ ] Address CAM capacity matches `Rs + Cs`.
- [ ] Hybrid CAM capacity matches `Rs(Cs-1) + Cs(Rs-1)`.
- [ ] Pivot classification passes deterministic tests.
- [ ] Nonpivot classification passes deterministic tests.
- [ ] Must-row behavior passes.
- [ ] Must-column behavior passes.
- [ ] Matrix dimension is `(Rs + Cs) × (Rs + Cs)`.
- [ ] Pivot-to-matrix mapping passes.
- [ ] Must-repair matrix mapping passes.
- [ ] Hybrid CAM information is preserved in matrix conversion.
- [ ] Fault-free-area candidate validation passes.
- [ ] Fig. 5 deterministic sequence is reproduced.
- [ ] Small-array RECAM/oracle disagreements are classified; the Section 11.2
      paper-derived false negative is preserved rather than optimized away.
- [ ] Every successful RemapTable OPTION independently covers all source faults
      through MAP and/or exact-address BUFFMAP records.
- [ ] CAM-reuse capacity is enforced correctly.
- [ ] CAM-reuse overflow produces an unrepairable result.
- [ ] Monte Carlo runs are seed-reproducible.

## Strongly recommended

- [ ] Table-II trends are reproduced.
- [ ] Table-II numerical values are statistically close after the ITC'2020 fault generator is independently verified.
- [ ] Multi-channel repairability requires every channel to be repairable.
- [ ] Intermediate RECAM states can be exported for failed tests.
- [ ] Online CAM-reuse read/write tests pass if online mode is implemented.

---

# 37. Important Invariants for Codex Review

When reviewing the existing implementation, Codex should search for violations of these invariants before proposing architectural changes.

### INV-01

```text
number of Address CAM entries == Rs + Cs
```

### INV-02

```text
number of Hybrid CAM entries ==
Rs*(Cs-1) + Cs*(Rs-1)
```

### INV-03

No two currently represented pivot faults share a row or a column.

### INV-04

Every nonpivot fault shares a row or column with at least one pivot entry.

### INV-05

```text
row_fault_count > Cs
=> row must-repair
```

### INV-06

```text
column_fault_count > Rs
=> column must-repair
```

### INV-07

Every collected line-repair fault is represented in the redundancy-analysis state.

### INV-08

A valid repair solution leaves no represented fault in its fault-free area.

### INV-09

Address-CAM overflow alone does not immediately imply failure when CAM-reuse capacity remains.

### INV-10

Final repairability must account for both:

```text
spare-row/spare-column solution
AND
CAM-reuse capacity
```

### INV-11

Software search iterations must not be reported as RECAM hardware analysis cycles.

### INV-12

Monte Carlo validation must be reproducible from a fixed seed.

---

# 38. Codex Review Instructions

When asked to inspect the RECAM implementation, Codex should proceed in this order:

1. locate the current RECAM classes/functions;
2. map the implementation onto the logical components in this specification;
3. identify the fault insertion path;
4. identify pivot/nonpivot classification logic;
5. identify must-repair update logic;
6. identify Address CAM and Hybrid CAM capacity logic;
7. identify CAM-reuse overflow handling;
8. identify matrix-conversion logic;
9. identify fault-free-area analysis;
10. identify final repairability decision;
11. identify Monte Carlo fault generation;
12. identify repair-rate calculation;
13. run deterministic tests before modifying code;
14. compare small cases with a brute-force oracle;
15. only then run large Monte Carlo regression tests.

When a mismatch occurs, Codex should report:

```text
expected behavior
actual behavior
first diverging fault
relevant internal state
suspected function
minimal reproducing input
```

Do not begin by rewriting the algorithm.

---

# 39. Suggested Regression Test Order

```text
01_capacity_address_cam
02_capacity_hybrid_cam
03_matrix_dimension
04_solution_count

05_pivot_basic
06_nonpivot_shared_column
07_nonpivot_shared_row
08_pivot_unique

09_must_row
10_must_column

11_fig5_sequence

12_matrix_pivot
13_matrix_must_row
14_matrix_must_column
15_matrix_hybrid

16_fault_free_valid
17_fault_free_invalid

18_bruteforce_small_random
19_cam_reuse_capacity
20_cam_reuse_overflow

21_online_write_hit
22_online_read_hit
23_online_miss

24_monte_carlo_2_2
25_monte_carlo_3_3
26_monte_carlo_4_4
```

Online tests may be skipped when the simulator targets only offline repair rate.

---

# 40. Reference Experimental Configuration

Paper repair-rate experiment:

```text
trials       = 10,000
memory       = 1024 × 1024
spares       = [2,2], [3,3], [4,4]

fault source = ITC'2020 RA Benchmark based
cluster      = 20%
line         = 30%
random       = remaining probability
```

Paper hardware evaluation:

```text
synthesis library = SAED / Synopsys 32 nm
area metric       = NAND-gate count
```

These values should be stored in project configuration rather than embedded deeply inside the RECAM algorithm.

---

# 41. Source-to-Spec Mapping

The most important paper locations are:

```text
Fig. 3
    Address CAM and Hybrid CAM structures

Table I
    Address CAM / Hybrid CAM entry counts

Fig. 4
    RECAM fault-collection and repair-analysis flow

Fig. 5
    deterministic fault-collection example
    Rs = Cs = 2
    8 × 8 memory

Fig. 6–7
    matrix conversion

Fig. 8–10
    repair-solution enumeration and
    fault-free-area analysis

Fig. 11–13
    online CAM reuse

Table II
    repair-rate regression targets

Table III
    analysis-cycle comparison

Fig. 14–15
    multi-channel and CAM-size repair-rate behavior

Fig. 16
    area comparison

Fig. 17
    online reliability study

Fig. 18
    overall evaluation
```

---

# 42. Known Specification Boundary

The following items are **not fully specified by the RECAM paper alone** and must not be silently invented during validation:

1. the complete implementation details of the ITC'2020 RA benchmark fault generator;
2. transistor-level CAM implementation;
3. exact physical eFuse organization;
4. exact cycle-by-cycle BIST timing;
5. process-independent CAM/MUX delay;
6. process-independent area;
7. behavior when one Address-CAM entry becomes both row-must and column-must;
8. pointer/count priority when one nonpivot simultaneously matches a pivot row
   and a different pivot column;
9. any Address-CAM repacking or pivot reselection after a must condition;
10. special handling for `Rs=Cs=1`, which is absent from the paper's tables
    and experiments;
11. physical reclamation of existing Hybrid-CAM entries after a line becomes
    must-repair; this project clears them as specified in Section 9.2-C;
12. software-specific ordering when multiple mathematically equivalent repair
    solutions exist.

For these cases, the simulator may choose an implementation convention, but that convention must be documented separately from paper-derived RECAM behavior.

---

# 43. Final Verification Principle

Validation should distinguish three different questions:

```text
A. Did we implement RECAM's logical behavior correctly?

B. Does the implementation produce an optimal
   spare-row/spare-column repair decision?

C. Does the complete statistical experiment reproduce
   the paper's reported repair-rate distribution?
```

Use:

```text
deterministic tests
```

for A,

```text
brute-force oracle
```

for B,

and:

```text
verified benchmark fault generator
+ 10,000-trial Monte Carlo
```

for C.

A matching repair-rate graph alone is not sufficient evidence of correctness.
