# Phase 3B — RECAM / DSS Analyzer Interface Freeze

> Status: COMPLETE — frozen 27-bit address geometry and shared-analyzer
> interface.  This document does not authorize Phase 3C or synthesis.

## 0. Frozen system address and CAM geometry

The 27-bit system word address is frozen as follows:

| Field | Bits |
|---|---:|
| Domain | 3 |
| Bank | 2 |
| Group | 6 |
| SA | 2 |
| Row | 9 |
| ColumnWord | 5 |
| **Total** | **27** |

RECAM's historical Channel field is the 13-bit concatenation
`Domain + Bank + Group + SA`; `SA` identifies the active subarray within the
group. The analyzer interface therefore uses `ROW_ADDR_W=9`,
`COL_ADDR_W=5`, and `DIFF_ADDR_W=9`. `WORD_W=16` is the online replacement
word width. The supported configurations have `MAX_K=5`, `PIVOT_PTR_W=3`,
`MAX_ADDRESS_ENTRIES=5`, and `MAX_HYBRID_ENTRIES=7`.

## 1. Analyzer boundary definition

The canonical per-subarray analyzer boundary is:

```text
Address CAM logical state + Hybrid CAM logical state + Must state
→ Matrix Builder → candidate evaluation → PatternID
```

It is explicitly not a raw fault-stream interface and must not reclassify
faults as pivot/nonpivot during matrix analysis.  The collector generates and
stores the logical state.  Matrix conversion remains responsible for applying
the documented Hybrid-to-matrix cases, including any ordered dictionary
extension required by those cases.

The immediate analyzer-area measurement begins at these input ports.  It
therefore excludes collector logic and CAM storage.  That exclusion does not
remove those costs from a later complete RECAM comparison.

This is a per-subarray analyzer boundary, so the 13-bit RECAM Channel and
16-bit online replacement word are not analyzer ports. They remain part of the
frozen system-address and reusable-CAM accounting above, and must be included
when collector/storage or online-repair hardware is later measured.

## 2. Pivot logical-state format

For one selected configuration, each Address/Pivot CAM logical entry is:

```systemverilog
typedef struct packed {
    logic                    valid;
    logic [ROW_ADDR_W-1:0]   row_address;
    logic [COL_ADDR_W-1:0]   column_address;
    logic                    row_must;
    logic                    column_must;
} pivot_logical_entry_t;
```

This represents stored state, not a request for the Matrix Builder to recreate
fault counters or Must thresholds.

## 3. Hybrid logical-state format

The canonical candidate representation is the RECAM logical Hybrid entry:

```systemverilog
typedef struct packed {
    logic                         valid;
    logic [PIVOT_PTR_W-1:0]       pivot_pointer;
    logic                         descriptor;
    logic [DIFF_ADDR_W-1:0]       differing_address;
} hybrid_logical_entry_t;
```

`PIVOT_PTR_W = 3` supports all entries through `MAX_K=5`. The executable collector convention is
`descriptor=0` for a fault on the pivot row (the stored differing address is a
column) and `descriptor=1` for a fault on the pivot column (the stored
differing address is a row).  Transpose normalization inverts the descriptor.
This corrects the earlier prose, which had reversed the stored differing
dimension.  Evidence is `shared_fault_collector.sv`, `RECAM_CAM.cpp`, and the
Hybrid-aware decoder regression; Phase 3A itself has full row/column Hybrid
tuples and therefore defines no descriptor bit. `DIFF_ADDR_W =
max(ROW_ADDR_W, COL_ADDR_W)`;
with the frozen widths, an offline Hybrid entry occupies `1 + 3 + 1 + 9 = 14`
bits. For `descriptor=0`, the 5-bit ColumnWord is stored as
`{4'b0000, column_word[4:0]}`; for `descriptor=1`, the 9-bit row is stored
directly. This representation remains deterministic after transpose
normalization.

## 4. Ownership table

`A→B` means fault collection generates the value and CAM state retains it for
the analyzer.  It is not a request to duplicate the logic in Matrix Builder.

| Item | Ownership classification | Rationale |
|---|---|---|
| Pivot row address | A→B | Collected pivot payload retained by Address CAM. |
| Pivot column address | A→B | Collected pivot payload retained by Address CAM. |
| RowMust | A→B | Derived from collection counters, then retained against its pivot. |
| ColumnMust | A→B | Derived from collection counters, then retained against its pivot. |
| Hybrid valid | A→B | Allocation/clear state of a Hybrid CAM entry. |
| Hybrid `pivot_pointer` | A→B | Hybrid is defined relative to an Address-CAM entry. |
| Hybrid `descriptor` | A→B | Hybrid stored-state metadata defining the differing dimension. |
| Hybrid `differing_address` | A→B | Hybrid stored payload for the nonpivot dimension. |
| Hybrid differing-address dictionary match | C | Matrix conversion determines the documented existing/free-dictionary matrix case. |
| Free logical dictionary index | C | A transient matrix-conversion index; it is not a free Address-CAM entry. |

No raw-fault reclassification belongs in Matrix Builder.  The exact matrix
case semantics for a Hybrid value that matches an already extended dictionary
remain subject to the reference questions below.

## 5. Frozen raw reusable CAM storage accounting

The largest supported configuration is 3R2C/2R3C. Address CAM capacity is
`Rs+Cs=5`; Hybrid capacity is `Rs(Cs-1)+Cs(Rs-1)`, whose maximum is seven.

| Structure | Offline entry | Online entry | Physical entry | Entries | Raw storage |
|---|---:|---:|---:|---:|---:|
| Address CAM | `1+9+5+1+1+2+2 = 21` | `1+9+5+13 = 28` | 28 | 5 | 140 bits |
| Hybrid CAM | `1+3+1+9 = 14` | `1+3+16 = 20` | 20 | 7 | 140 bits |
| **Total** |  |  |  |  | **280 bits** |

The 280 bits are raw logical storage accounting only. They exclude CAM
comparators, write/update and collector logic, counters, decoder, temporary
reuse buffer, analyzer logic, eFuse/NVM, and macro packing or silicon area.

## 6. Information before and inside analysis

Before analysis: pivot selection, Address-CAM allocation, nonpivot
classification, Hybrid pointer/descriptor/differing-address generation,
Hybrid valid/clear, and Must threshold classification.

Inside Matrix Builder: diagonal/Must matrix initialization; conversion of each
stored Hybrid record through its pointer and descriptor; the documented
existing-versus-free logical dictionary case; transient dictionary index
selection; and final matrix-bit activation.

## 7. Remaining interface assumptions

1. Hybrid records are processed in stored order; the golden model fixes that
   order for dictionary extension.
2. The asymmetric-width packing is frozen: ColumnWord is zero-extended to
   `DIFF_ADDR_W=9`; no sign extension or unspecified upper bits are allowed.
3. Collector generation/storage and physical CAM implementation remain outside
   this analyzer-only boundary.

## 8. Expected future Phase 3B top-level ports

This interface is frozen for a future shared analyzer; it does not authorize
its module creation.

```systemverilog
input  logic [K-1:0]                    pivot_valid_i;
input  logic [K*ROW_ADDR_W-1:0]         pivot_rows_flat_i;
input  logic [K*COL_ADDR_W-1:0]         pivot_cols_flat_i;
input  logic [K-1:0]                    row_must_i;
input  logic [K-1:0]                    col_must_i;
input  logic [HYBRID_ENTRIES-1:0]       hybrid_valid_i;
input  logic [HYBRID_ENTRIES*PIVOT_PTR_W-1:0] hybrid_pointers_flat_i;
input  logic [HYBRID_ENTRIES-1:0]       hybrid_descriptor_i;
input  logic [HYBRID_ENTRIES*DIFF_ADDR_W-1:0] hybrid_differing_addrs_flat_i;
input  logic                             conventional_overflow_i;
output logic [K*K-1:0]                  matrix_flat_o;
output logic [PATTERN_ID_W-1:0]         pattern_id_o;
output logic                             repairable_o;
```

`ConfigID`, scheduler state, ConfigPatternMap, EARLY, GROUP, and resource
ledger ports are deliberately absent.  They are outside this interface-freeze
task.

## 9. Fairness for later full-area comparison

Moving pointer/descriptor generation out of Matrix Builder reduces the
reported analyzer-only area, but it does not eliminate the hardware.  A later
complete RECAM/DSS comparison must separately include:

- collector comparators and pivot/nonpivot classification;
- pointer/descriptor/differing-address generation logic;
- added Pivot/Hybrid CAM storage bits and write/clear logic; and
- matrix/analyzer logic from this frozen boundary onward.

The future report must show both analyzer-only and complete-hardware totals,
each with area, mapped leaf-cell count, and NAND2X1-equivalent gate count.
