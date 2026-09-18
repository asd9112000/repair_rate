# T0B — V2 ColumnWord / BIST Word / Replacement-Payload Semantic Reconciliation

> Status: COMPLETE — read-only evidence audit. No RTL, C++, geometry,
> BIST-schedule, synthesis, or experiment change is authorized.

## 1. Executive conclusion

The two five-bit values are not proven to be the same address.

- Frozen V2 consumes ColumnWord[4:0] as a retained repair-analysis coordinate.
  It is zero-extended and equality-compared while constructing the candidate
  matrix.
- The BIST schedule independently uses bistWordColumn = floor(Fault::c / 256).
- Phase-3B WORD_W=16 is online replacement-payload width, not a demonstrated
  column-address stride.

The S1G-A2G retained collector makes one local conversion visible:
analyzer_view_o[50 + 5*i +: 5] = pivot_col[i][4:0]. It is a low-five-bit
projection of a collector-local 10-bit column value, not a divide-by-256
conversion. No active bridge maps C++ Fault::c into that collector input.

T0B selects Branch C. The BIST timing domain is independently valid, while
the V2 repair-column mapping from physical C++ fault coordinates is unresolved.

## 2. Starting mismatch

| Domain | Row | Column | Meaning |
| --- | --- | --- | --- |
| C++ physical fault | Fault::r, 9 bits | Fault::c, 13 bits, 0..8191 | physical cell coordinate |
| S1D/E0-L BIST | row, 9 bits | wordColumn, 5 effective bits, 0..31 | 256-cell BIST transaction |
| frozen V2 analyzer | ROW_ADDR_W=9 | COL_ADDR_W=5 | retained pivot/Hybrid ColumnWord |
| Phase-3B payload | — | WORD_W=16 | online replacement payload |

An equal five-bit range does not establish equal units or a conversion
contract.

## 3. Physical fault domain

Fault::r and Fault::c are the C++ fault coordinates. FaultAddressGeometry
defines rows=512, cellColumns=8192, and wordBits=256 in
inc/FaultAddress.hpp; src/FaultAddress.cpp validates Fault::c against that
physical range. The frozen E0-L corpus sets the same 512-row, 8192-column,
256-bit configuration.

    FAULT_C_PHYSICAL_RANGE: 0..8191
    FAULT_C_UNIT: physical cell / physical bit-column

The Hierarchical-RECAM simulator separately canonicalizes an online
DataWord tag as sourceColumn / dataWordBits. That C++ simulator behavior does
not feed the V2 analyzer port.

## 4. BIST timing domain

BistWordAddress7 deliberately distinguishes wordColumn from legacy Fault::c.
src/FaultAddress.cpp implements:

    bistWordColumn = floor(Fault::c / geometry.wordBits)
    bitOffsetWithinWord = Fault::c % geometry.wordBits

SerialBistSchedule uses the same geometry. At 8192/256, there are 32 BIST
word columns per row and an effective five-bit word column. The frozen E0-L
test asserts the recipe and a four-SA completion of 65,536 cycles.

## 5. V2 ColumnWord trace

| Source / module | Signal and width | Semantic use | Operation |
| --- | --- | --- | --- |
| rtl/dss_v2/common/dss_v2_params_pkg.sv | COL_ADDR_W=5 | frozen DATE parameter | no conversion |
| recam_dss_v2_{early,group}_top | pivot_cols_flat_i, 5 per pivot | pre-collected analyzer input | no raw-fault or BIST port |
| recam_shared_config_analyzer | a 5-bit pivot slice | row/column dictionary item; can transpose | zero-extend 5→9 and equality-compare |
| same analyzer | candidate_valid_o, pattern_id_o, repairable_o | candidate decision | no ColumnWord arithmetic or ColumnWord-value storage index |

In rtl/recam/recam_shared_config_analyzer.sv, the ColumnWord is inserted as
{4'b0000, pivot_cols_flat_i[... +: 5]}. Hybrid column differing addresses are
treated identically before dictionary matching. The pivot slot indexes packed
state; the ColumnWord value does not index storage. The analyzer emits
candidate validity, PatternID, solution-valid, repairable, and overflow, not a
repair address.

    V2_COLUMNWORD_SEMANTICS:
    5-bit retained repair-analysis coordinate used as an opaque equality/
    dictionary tag; physical and BIST unit NOT_PROVEN.

## 6. pivot_cols_flat_i source trace

The legacy production EARLY/GROUP tops receive an already packed five-bit
bundle. Their testbenches drive that bundle directly; no source derives it
from C++ Fault::c or a BIST packet.

The newer retained path is the in-tree collector-to-analyzer producer:

    fault_col_i[9:0] -> pivot_col[i][9:0] (retained collector state)
    analyzer_view_o[50 + 5*i +: 5] = pivot_col[i][4:0]

The retained collector bank and its historical lockstep wrapper implement the
same low-bit projection. This establishes an internal V2 view conversion, but
not a relationship between that 10-bit input and the C++ 13-bit physical
column.

    PIVOT_COL_SOURCE_DOMAIN:
    legacy top: pre-collected 5-bit analyzer bundle; retained path: 10-bit
    collector-local fault-column state; C++ physical binding NOT_PROVEN.

    PIVOT_COL_CONVERSION:
    retained path: v2RepairColumnWord = retained_fault_col[4:0];
    legacy top: direct packed analyzer input; C++-to-V2 conversion NOT_PROVEN.

## 7. RECAM_WORD_BITS=16 trace

The authoritative Phase-3B spelling is WORD_W=16; there is no active V2 RTL
parameter named RECAM_WORD_BITS. The interface freeze calls WORD_W the online
replacement word width. Its raw-accounting table uses it in the online Hybrid
entry, 1+3+16=20 bits. The document also explicitly excludes the 16-bit
replacement word from analyzer ports.

| Use | Classification | Column-address effect |
| --- | --- | --- |
| Phase-3B WORD_W=16 | REPLACEMENT_PAYLOAD_WIDTH | none evidenced |
| online Hybrid CAM 1+3+16 accounting | REPLACEMENT_PAYLOAD_WIDTH | none evidenced |
| frozen analyzer pivot_cols_flat_i | not a 16-bit signal | no dependency |

No active V2 consumer derives, compares, or decodes ColumnWord from the
16-bit payload. The 16-bit width must not be used to derive the BIST word
count.

## 8. Physical page 8192 trace

FaultAddressGeometry::cellColumns=8192 is physical cell columns per modeled
row, not a V2 analyzer bundle width. The E0-L frozen configuration
independently fixes memoryColumns=8192 and columnAddressWidthBits=13. No
evidence identifies 8192 as a Phase-3B logical page or a 16-bit payload.

## 9. 256-bit BIST recipe trace

FaultAddressGeometry::wordBits and the E0-L dataWidthBits supply 256 to
encodeBistWordAddress(), bitOffsetWithinWord(), and SerialBistSchedule. A BIST
packet has a fail mask of exactly wordBits bits, so its role is:

    BIST_WORD_BITS: 256
    BIST_WORD_BITS_ROLE: C++ BIST transaction and serial-timing abstraction

Repository-wide inspection found no active V2/RECAM RTL interface carrying a
256-bit fault, BIST, or test word. V2 uses individual fault coordinates or
analyzer-state summaries. For this V2 audit:

    BIST_256BIT_WIDTH_ARCHITECTURAL: NO

The Hierarchical-RECAM simulator having a configurable 256-bit data word does
not create a V2 RTL BIST bus or a V2 repair-address definition.

## 10. CHANNEL_W / DIFF_ADDR_W semantics

The Phase-3B 27-bit system address is:

    Domain[2:0] + Bank[1:0] + Group[5:0] + SA[1:0] + Row[8:0] + ColumnWord[4:0]

Its 13-bit historical Channel is Domain+Bank+Group+SA: hierarchy ownership,
not high physical-column bits. DIFF_ADDR_W=9 is max(ROW_ADDR_W, COL_ADDR_W).
It stores a row directly or a ColumnWord as {4'b0000, ColumnWord[4:0]}; it is
an asymmetric dictionary representation, not a split-column extension.

    COLUMN_ADDRESS_IS_HIERARCHICALLY_SPLIT: NO

No frozen active-interface field carries the missing bits required to view a
five-bit ColumnWord as every 16-bit word in an 8192-cell row.

## 11. Online repair/remap semantics

The Phase-3B freeze supplies storage accounting, not an active V2 runtime
remap lookup. It accounts for an online Address CAM entry of 1+9+5+13 bits
and an online Hybrid CAM entry of 1+3+16 bits, while explicitly excluding
both Channel and the 16-bit replacement word from the analyzer boundary.

The executable Hierarchical-RECAM simulator separately compares a global tag
containing hierarchy, row, and sourceColumn / dataWordBits for DataWord
granularity. It does not drive pivot_cols_flat_i. Active V2 RTL consequently
does not prove whether replacement happens at a 16-bit word, 256-bit segment,
or another physical unit.

## 12. Four-domain mapping table

| Domain | Field | Width / range | Unit | Source | Consumer |
| --- | --- | --- | --- | --- | --- |
| Physical fault | Fault::c | 13 bits, 0..8191 | physical cell column | FaultAddressGeometry | C++ fault/BIST paths |
| BIST timing | BistWordAddress7::wordColumn | 5 effective bits, 0..31 | 256-cell timing word | Fault::c / 256 | SerialBistSchedule |
| V2 analyzer/repair | ColumnWord / pivot_cols_flat_i | 5 bits, 0..31 | retained repair-analysis coordinate; physical unit unresolved | analyzer boundary / retained [4:0] view | dictionary, matrix, PatternID |
| RECAM payload/data | WORD_W | 16 bits | online replacement payload | Phase-3B accounting | future collector/online-repair accounting |

## 13. H1 direct-equivalence test

    H1: V2 ColumnWord[4:0] == floor(Fault::c / 256)
    RESULT: NOT_PROVEN

No active conversion or frozen V2 specification makes that assignment.
Matching widths are insufficient. The visible retained-path conversion is
[4:0], not /256, but its input is not bound to C++ Fault::c; it therefore
neither proves nor globally disproves H1.

## 14. H2 16-bit word-index test

    H2: V2 ColumnWord indexes all 16-bit logical words in an 8192-cell row
    RESULT: DISPROVEN for the frozen active V2 interface

That interpretation needs 512 values, hence nine bits, or a proven field
carrying the missing four bits. Channel is hierarchy identity and DIFF_ADDR_W
is a row-or-zero-extended-column dictionary field. The freeze instead defines
the 16-bit quantity as replacement payload width.

## 15. Safe timing-domain decision

LAST_FAULT_ACCEPT timing needs only the physical coordinate, BIST geometry,
and serial schedule:

    physicalCellColumn = Fault::c
    bistWordColumn     = floor(physicalCellColumn / 256)
    arrivalCycle       = SerialBistSchedule::arrivalCycle(Fault)

It does not consume pivot_cols_flat_i or V2 analyzer semantics.

    BIST_TIMING_CAN_BE_MODELED_IN_SEPARATE_DOMAIN: YES

## 16. Selected decision branch

    SELECTED_BRANCH: C

The BIST timing mapping is implemented and independently validated. V2 mapping
relative to C++ physical faults remains unresolved.

## 17. T1 implications

T1 is not authorized by T0B. If later explicitly authorized, it must keep
names and mappings separate:

    physicalCellColumn = fault.c;
    bistWordColumn = physicalCellColumn / bistWordBits;
    // v2RepairColumnWord requires a separately reviewed mapper.

The BIST mapper must not be reused as a V2 repair mapper, and T1 must not
claim bistWordColumn equals v2RepairColumnWord.

## 18. Remaining unknowns

1. The physical source and conversion contract for the legacy
   pivot_cols_flat_i input are absent from the active V2 production tops.
2. The retained collector's 10-bit fault-column input is not bound to the
   C++ 13-bit physical fault domain.
3. Active V2 RTL has no online remap datapath proving a replacement
   granularity or its relationship to 16-bit payloads and 256-bit BIST words.
4. A repair/timing integration needs a new, reviewed Fault::c to
   v2RepairColumnWord contract; T0B does not invent one.

## Evidence and review matrix

Primary evidence: rtl/dss_v2/common/dss_v2_params_pkg.sv,
rtl/dss_v2/top/recam_dss_v2_{early,group}_top.sv,
rtl/dss_v2/top/recam_dss_v2_retained_collector_bank.sv,
rtl/dss_v2/top/recam_dss_v2_retained_analyzer_path.sv,
rtl/recam/recam_shared_config_analyzer.sv,
docs_verilog/PHASE3B_ANALYZER_INTERFACE.md, inc/FaultAddress.hpp,
src/FaultAddress.cpp, src/HierarchicalRecamSimulator.cpp, and
tests/e0l_generate_candidate_corpus.cpp.

No RTL was generated or modified. The readable-RTL public review matrix is:

    compile: NOT_RUN
    ast: NOT_RUN
    readability: NOT_RUN
    comment: NOT_APPLICABLE
    naming: NOT_RUN
    profile: NOT_RUN
    testbench: NOT_RUN
    toolchain: NOT_RUN

## T0B final status

    T0B_STATUS:
    COMPLETE

    RTL_ROW_ADDR_W:
    9

    RTL_COL_ADDR_W:
    5

    RTL_LOGICAL_WORD_BITS:
    16

    RTL_PHYSICAL_PAGE_BITS:
    8192

    CPP_FAULT_ROW_DOMAIN:
    9-bit row

    CPP_FAULT_COL_DOMAIN:
    13-bit physical-cell column

    BIST_WORD_BITS:
    256

    BIST_WORD_COLUMNS:
    32

    BIST_WORD_COLUMN_BITS:
    5

    V2_COLUMNWORD_SEMANTICS:
    5-bit retained repair-analysis coordinate used as an opaque equality/dictionary tag;
    physical and BIST unit NOT_PROVEN

    PIVOT_COL_SOURCE_DOMAIN:
    pre-collected 5-bit analyzer bundle; S1G-A2G path projects a 10-bit collector-local
    fault column; C++ physical binding NOT_PROVEN

    PIVOT_COL_CONVERSION:
    S1G-A2G v2RepairColumnWord = retained_fault_col[4:0]; legacy top input is DIRECT;
    C++-to-V2 conversion NOT_PROVEN

    RECAM_WORD_BITS_ROLE:
    Phase-3B WORD_W=16 online replacement payload width / online Hybrid-CAM accounting

    RECAM_WORD_BITS_AFFECTS_COLUMN_ADDRESS:
    NO

    BIST_256BIT_WIDTH_ARCHITECTURAL:
    NO

    COLUMN_ADDRESS_IS_HIERARCHICALLY_SPLIT:
    NO

    H1_V2_COL_EQUALS_BIST256_WORD:
    NOT_PROVEN

    H2_V2_COL_IS_FULL_16BIT_WORD_INDEX:
    DISPROVEN

    BIST_TIMING_CAN_BE_MODELED_IN_SEPARATE_DOMAIN:
    YES

    SELECTED_BRANCH:
    C

    T1_TIMING_MODEL_MAY_PROCEED:
    YES

    RTL_MODIFIED:
    NO

    CPP_TIMING_MODEL_MODIFIED:
    NO

    GEOMETRY_CONSTANTS_MODIFIED:
    NO

    EARLY_MODIFIED:
    NO

    GROUP_MODIFIED:
    NO

    S1GB_RESUMED:
    NO

    SYNTHESIS_RUN:
    NO

    GIT_DIFF_CHECK:
    PASS

    AUDIT_DOCUMENT:
    docs/dss_execution/T0B_V2_COLUMNWORD_BIST_WORD_SEMANTIC_RECONCILIATION.md

    NEXT_RECOMMENDED_PHASE:
    Explicit review/authorization of T1 separate-domain BIST timing model; do not map
    physicalCellColumn to v2RepairColumnWord without a new conversion contract
