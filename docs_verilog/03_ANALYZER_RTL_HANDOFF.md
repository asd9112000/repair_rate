# Codex Handoff — Verilog RTL for 2×2 Directional Multi-Config Analyzer

> **Scope:** 2×2 Directional Multi-Config Analyzer RTL.
>
> **Relationship:** Subordinate to
> [00_PROJECT_MASTER.md](00_PROJECT_MASTER.md) at project level, but
> authoritative for implementation details inside this RTL scope.

> **Phase 3 supersession notice:** This handoff records the completed Phase 2
> Directional Multi-Config implementation and is frozen as historical evidence.
> Do not delete, rewrite, refactor, or continue timing optimization of that
> implementation or its synthesis results.  It does not authorize Phase 3 RTL.
> For new work, [02_PHASE_CONTROL.md](02_PHASE_CONTROL.md) controls the staged
> analyzer-area plan: fully combinational 2R2C RECAM baseline, then one
> config-serial/pattern-parallel Shared Config Analyzer, then EARLY and GROUP.
> In particular, the Phase 2 40-cycle Pattern-serial AREA variant is historical
> only and must not be used as the new primary analyzer architecture.

## 1. Objective

Implement synthesizable SystemVerilog RTL for the finalized:

> **2×2 Directional Multi-Config RECAM Analyzer with shared fault-collection storage and EARLY/GROUP Solution Take Policies**

The RTL must use the corrected C++ simulator as the functional golden model.

Primary hardware objective:

> Support multiple repair-resource configurations without instantiating a complete Address CAM / Hybrid CAM pair for every configuration.

Area, critical path, register count, CAM bit count, comparator count, and policy overhead must remain separately measurable.

---

# 2. Architectural Top Level

Target architecture:

```text
                         BIST Fault Stream
                                |
                                v
                   +-------------------------+
                   | Shared Fault Collector  |
                   |                         |
                   | Pivot / Address CAM     |
                   | Shared Counters         |
                   | Tagged Hybrid Storage   |
                   | CAM-Reuse Temp Buffer   |
                   +-----------+-------------+
                               |
               +---------------+----------------+
               |               |                |
               v               v                v
             CAN0            CAN1             CAN2            CAN3
             2R2C            2R1C             3R2C            3R1C
                              ↕                 ↕                ↕
                           transpose         transpose        transpose
                              ↕                 ↕                ↕
                             1R2C              2R3C             1R3C
               |               |                |
               +---------------+----------------+
                               |
                               v
                     Config Pattern Results
                               |
                  +------------+------------+
                  |                         |
                  v                         v
               EARLY                     GROUP
              Selector                   Search
                  |                         |
                  +------------+------------+
                               |
                               v
                   Physical Resource Ledger
                               |
                               v
                    Pending Repair Buffer
                               |
                               v
                          eFuse Interface
```

---

# 3. Required RTL Hierarchy

Recommended module hierarchy:

```text
dss_analyzer_top.sv

shared_fault_collector.sv
shared_pivot_cam.sv
shared_fault_counter.sv
tagged_hybrid_store.sv
cam_reuse_temp_buffer.sv

must_threshold_decode.sv

config_analyzer.sv
recam_matrix_builder.sv
recam_pattern_analyzer.sv
lowest_pattern_encoder.sv
rc_transpose_wrapper.sv

config_result_bank.sv

early_selector.sv
group_selector.sv
resource_ledger.sv

pending_repair_buffer.sv
repair_decode.sv

dss_pkg.sv
```

Keep modules independent enough for individual synthesis.

---

# 4. Configuration Definitions

In `dss_pkg.sv` define seven physical configurations:

```systemverilog
CFG_2R2C
CFG_2R1C
CFG_3R2C
CFG_3R1C
CFG_1R2C
CFG_2R3C
CFG_1R3C
```

Use:

```text
ConfigID width = 3 bits
```

Semantics:

```text
ConfigID
=
available row/column spare-resource envelope
```

Do not use ConfigID to represent a RECAM pattern.

---

# 5. Pattern ID

Use a separate:

```text
PatternID
```

Current maximum number of candidates:

```text
3R2C:
C(5,3) = 10
```

Use:

```systemverilog
logic [3:0] pattern_id;
```

Encoding:

```text
0
=
invalid / no feasible candidate

1...10
=
valid RECAM candidate pattern
```

Pattern numbering must exactly match the simulator.

Do not independently invent RTL pattern ordering.

---

# 6. Canonical Analyzer IDs

Use four canonical structures:

```text
CAN0 = 2R2C
CAN1 = 2R1C
CAN2 = 3R2C
CAN3 = 3R1C
```

Physical mappings:

```text
2R2C -> CAN0 orientation=0

2R1C -> CAN1 orientation=0
1R2C -> CAN1 orientation=1

3R2C -> CAN2 orientation=0
2R3C -> CAN2 orientation=1

3R1C -> CAN3 orientation=0
1R3C -> CAN3 orientation=1
```

---

# 7. R/C Transpose Wrapper

`rc_transpose_wrapper.sv` must normalize all R/C-sensitive signals.

When:

```text
orientation = 0
```

use normal interpretation.

When:

```text
orientation = 1
```

exchange:

```text
row address / column address
row count / column count
Rs / Cs
RowMust / ColumnMust
matrix axes
repair-row / repair-column output
```

Normalize differing row-address and column-address widths internally.

Do not directly connect unequal-width buses through a raw swap.

Use explicit packing/unpacking.

---

# 8. Shared Pivot / Address CAM

Do not instantiate seven Address CAMs.

Do not instantiate four Address CAMs.

Instantiate one shared pivot-address CAM/payload structure.

For current configs:

```text
MAX_K = 5
```

Conceptual entry:

```systemverilog
typedef struct packed {
    logic valid;
    logic [ROW_W-1:0] row_addr;
    logic [COL_W-1:0] col_addr;
} pivot_entry_t;
```

Array:

```systemverilog
pivot_entry_t pivot_cam [MAX_K];
```

Configuration-dependent activity is supplied separately.

Initial safe RTL implementation:

```systemverilog
logic [NUM_CFG-1:0] cfg_entry_valid [MAX_K];
```

After golden validation, synthesize an optional optimized version deriving membership from:

```text
entry_index < K(config)
```

Compare both versions if practical.

---

# 9. Fault Broadcast

Every incoming BIST fault must be visible to:

```text
shared pivot logic
shared counters
Hybrid classification logic
CAM-reuse classification logic
```

Interface example:

```systemverilog
fault_valid
fault_row
fault_col
fault_last
```

The collector must process one fault according to the timing contract defined by the existing RECAM RTL model.

If no cycle-accurate RECAM fault collector exists yet, isolate the input handshake so timing assumptions can later be changed without redesigning analyzer logic.

---

# 10. Shared Fault Counters

Do not instantiate configuration-specific counters.

Use shared physical fault counts.

Expose threshold signals:

```text
row_gt1
row_gt2

col_gt2
col_gt3
```

The exact storage organization may be:

```text
register array
SRAM-like array
counter table
```

depending on the current synthesis experiment.

Keep the counter implementation replaceable.

---

# 11. Must Threshold Decoder

Baseline (Pivot-only) implementation:

```text
2R1C:
RowMust = row_gt1
ColMust = col_gt2

2R2C:
RowMust = row_gt2
ColMust = col_gt2

3R1C:
RowMust = row_gt1
ColMust = col_gt3

3R2C:
RowMust = row_gt2
ColMust = col_gt3
```

For transposed configurations use the normalized R/C signals from `rc_transpose_wrapper`.

Prefer combinational decode.

Do not store four duplicated permanent Must tags unless timing synthesis demonstrates a concrete need.

---

# 12. Shared / Tagged Hybrid Store

Do not instantiate one Hybrid CAM per configuration.

Use one physical tagged Hybrid structure.

Logical record should support at least:

```systemverilog
typedef struct packed {
    logic valid;

    logic [FAULT_REF_W-1:0] fault_ref;

    logic descriptor;

    logic [PIVOT_PTR_W-1:0] pivot_ptr;

    logic [NUM_CFG-1:0] cfg_valid;
} hybrid_entry_t;
```

The exact payload fields may be adapted to the existing RECAM representation.

Important:

A physical fault may have different logical Hybrid representations under different configurations.

Therefore support:

```text
multiple metadata records
referencing the same physical fault
```

when required.

Do not force all configuration views into one metadata tuple.

---

# 13. Hybrid Capacity Parameter

Define:

```systemverilog
parameter int HYBRID_SHARED_ENTRIES = ...;
```

Do not permanently hardcode:

```text
7
```

until simulator measurement proves that the maximum union occupancy fits seven entries.

Support at least two synthesis settings:

```text
A. measured shared-union capacity

B. conservative capacity
```

A conservative initial upper bound may use the sum of canonical logical capacities:

```text
1 + 4 + 7 + 2 = 14
```

This is a safe implementation parameter, not a claim that 14 entries are always necessary.

The corrected simulator must provide the experimental occupancy used for the final configuration.

---

# 14. CAM-Reuse Temporary Buffer

Create:

```text
cam_reuse_temp_buffer.sv
```

This buffer holds:

```text
additional pivot faults occurring after
the Address view of a configuration is full
```

It is outside the conventional Config/Pattern analyzer path.

It must not participate in:

```text
matrix candidate generation
PatternID encoding
GROUP path search
```

unless explicitly requested by a future experiment.

Conceptually:

```text
new pivot
   |
Address capacity available for cfg?
   |
  NO
   |
   v
CAMReuseTempBuffer
```

Allow configuration-valid metadata if the same additional pivot has a different role across configurations.

---

# 15. Config Analyzer Interface

Recommended normalized interface:

```systemverilog
module config_analyzer #(
    parameter int RS = ...,
    parameter int CS = ...
) (
    input  logic ...,

    input  normalized pivot state,
    input  normalized Hybrid state,
    input  normalized Must state,

    output logic       config_valid,
    output logic [3:0] pattern_id
);
```

Internally:

```text
Matrix Builder
    ↓
Parallel Candidate Analyzer
    ↓
Lowest Valid Pattern Encoder
```

---

# 16. Matrix Sizes

Support canonical dimensions:

```text
2R1C:
3 × 3

2R2C:
4 × 4

3R1C:
4 × 4

3R2C:
5 × 5
```

Keep matrix generation combinational where consistent with RECAM architecture.

Generate reusable parameterized RTL rather than four unrelated handwritten modules wherever synthesis tools handle parameterization cleanly.

---

# 17. Candidate Pattern Counts

Provide:

```text
2R1C:
3 candidates

2R2C:
6 candidates

3R1C:
4 candidates

3R2C:
10 candidates
```

Each candidate produces:

```text
candidate_valid
```

The pattern analyzer may evaluate candidates in parallel.

Do not implement group policy inside `recam_pattern_analyzer`.

---

# 18. Lowest Valid Pattern Encoder

Input:

```text
candidate_valid[N]
```

Output:

```text
pattern_id[3:0]
```

Behavior:

```text
no candidate valid:
pattern_id = 0

multiple valid:
pattern_id = lowest valid index + encoding offset
```

Example:

```text
valid:
P2 P4 P7

output:
P2
```

This deterministic result must bit-match the simulator.

---

# 19. Seven Physical Configuration Results

Even though only four canonical analyzer structures exist, the top-level architecture must expose:

```text
result[7]
```

for the seven physical ConfigIDs.

There are two acceptable RTL modes.

## AREA mode

Use four canonical analyzer structures and reuse the asymmetric analyzers across orientations.

```text
CAN1:
2R1C then 1R2C

CAN2:
3R2C then 2R3C

CAN3:
3R1C then 1R3C
```

This reduces duplicated combinational analyzer logic at the cost of analysis cycles.

## PARALLEL mode

Duplicate only the required transpose/analyzer combinational path so all seven physical configurations can be evaluated concurrently.

Shared CAM/counter storage must remain single-instance.

Make the mode parameterizable if implementation effort is reasonable:

```systemverilog
parameter bit PARALLEL_ORIENTATION = 0;
```

At minimum, implement AREA mode first.

Record analyzer-cycle difference between modes.

---

# 20. Config Result Bank

For one subarray store:

```text
7 × 4-bit PatternID
```

Conceptually:

```systemverilog
logic [3:0] config_pattern [NUM_CFG];
```

For GROUP:

```text
4 subarrays × 7 configs × 4 bits
=
112 bits
```

ConfigID is implicit in the array index.

Do not store ConfigID in each cell.

---

# 21. EARLY Selector

Input:

```text
PatternID for each allowed ConfigID
current PhysicalResourceLedger
SubarrayID
```

A config is feasible when:

```text
pattern_id != 0
AND
directional resources are available
```

Select according to the agreed priority.

For current directional case:

```text
2R1C
>
2R2C
>
3R1C
>
3R2C
```

Prefer implementation from decoded resource attributes:

```text
borrow_count
released_local_resources
deterministic ConfigID
```

rather than relying solely on arbitrary enum values.

Output:

```text
selected_config_id
selected_pattern_id
early_success
```

---

# 22. GROUP Selector

Input:

```text
ConfigPatternMap[4][7]
```

Search one valid ConfigID per subarray.

Use a small FSM / nested counter search rather than a giant combinational Cartesian-product block.

Conceptual sequence:

```text
select cfg_A
    ↓
update temporary ledger

select cfg_B
    ↓
resource conflict?
YES -> next candidate
NO  -> continue

select cfg_C

select cfg_D

all valid?
    ↓
group success
```

Support early pruning on directional-resource violations.

---

# 23. Search State

Recommended counters:

```systemverilog
cfg_a_idx
cfg_b_idx
cfg_c_idx
cfg_d_idx
```

For seven physical configs:

```text
raw upper bound:
7^4 = 2401
```

The actual number checked should be lower due to:

```text
invalid PatternID pruning
directional resource pruning
```

Expose:

```text
combinations_checked
```

in simulation/debug builds.

---

# 24. Physical Resource Ledger

Create a dedicated module:

```text
resource_ledger.sv
```

It must track physical ownership.

Do not use only aggregate row/column counts.

Required information includes:

```text
available local rows
available local columns

borrowed row ownership
borrowed column ownership

lender identity
borrower identity

resource already committed
```

A valid path must satisfy the 2×2 Directional borrowing topology.

---

# 25. Resource Descriptor LUT

ConfigID should decode into group-level resource behavior.

Example conceptual output:

```systemverilog
typedef struct packed {
    logic [1:0] local_rows;
    logic [1:0] local_cols;

    logic       borrow_row;
    logic       borrow_col;

    logic [...] borrow_direction;
} resource_desc_t;
```

Exact directional semantics must follow the current simulator policy.

The Verilog LUT and simulator must use identical ConfigID meaning.

---

# 26. Pending Repair Buffer

Do not directly expose irreversible eFuse programming at the moment EARLY selects a local result.

Store:

```text
SubarrayID
ConfigID
PatternID
CAMReusePending reference/information
```

in:

```text
pending_repair_buffer.sv
```

Once all four subarrays succeed:

```text
decode final conventional repair lines
+
append CAM-reuse repair information
+
assert final_program_valid
```

If group repair fails:

```text
discard pending group state
```

---

# 27. Final Repair Decode

Implement:

```text
PivotPayload
+
ConfigID
+
PatternID
```

to generate:

```text
source faulty row addresses
source faulty column addresses
```

Pattern bits specify whether each ordered pivot contributes:

```text
Row_i
or
Column_i
```

Example:

```text
Pattern:
R C R C

Pivot0 -> row0
Pivot1 -> col1
Pivot2 -> row2
Pivot3 -> col3
```

This decoder must match simulator output exactly.

### D-004 reconciliation — Hybrid dictionary extension

The Pivot-only form is sufficient only when the selected RECAM dictionary has
not been extended by a Hybrid record.  Under accepted decision D-004, retain a
single tagged Hybrid/dictionary delta snapshot per subarray until EARLY/GROUP
selection completes.  The final decoder consumes that snapshot in addition to
`PivotPayload + ConfigID + PatternID` and replays the selected ConfigID view.
The snapshot is temporary group state; it is neither a per-ConfigID CAM copy
nor a permanent eFuse field.  See `DECISION_LOG.md` D-004.

---

# 28. CAM-Reuse Final Output

Final repair information has two logically separate parts.

```text
A. Conventional spare-line repair

PivotPayload
+
ConfigID
+
PatternID
```

and:

```text
B. CAM-reuse repair

additional pivot fault information
from CAMReuseTemporaryBuffer
```

Keep separate RTL fields/interfaces.

Do not encode CAM-reuse information inside PatternID.

---

# 29. Subarray Processing

Support one shared physical analyzer engine processing subarrays sequentially:

```text
A
→ B
→ C
→ D
```

This matches the desired storage-reuse architecture.

After one SA completes:

```text
retain only ConfigPattern results
+
pivot information required for final decode
+
CAMReuse pending information
```

Release/reinitialize temporary analysis state before the next SA where allowed.

---

# 30. Important Storage Optimization

Do not replicate pivot addresses per config.

Store pivot provenance once per subarray.

Target conceptual retained state:

```text
Subarray A
    PivotPayload
    7 PatternIDs
    CAMReusePending

Subarray B
    PivotPayload
    7 PatternIDs
    CAMReusePending

...
```

GROUP must refer to these shared pivot arrays after it chooses the final ConfigID.

---

# 31. Required Parameters

Expose at least:

```systemverilog
ROW_W
COL_W

NUM_SA = 4
NUM_CFG = 7
NUM_CANONICAL = 4

MAX_K = 5

PATTERN_ID_W = 4

HYBRID_SHARED_ENTRIES

CAM_REUSE_ENTRIES

PARALLEL_ORIENTATION
```

Do not bury these values throughout RTL.

---

# 32. Required Hardware Counters / Reports

Synthesis/report scripts should separately report:

```text
Shared Pivot CAM area

Tagged Hybrid Store area

Shared counter area

Matrix Builder area

Pattern Analyzer area

Lowest Pattern Encoder area

EARLY Selector area

GROUP Selector area

Resource Ledger area

Pending Repair Buffer area

Total area
```

Also report:

```text
critical path
max clock frequency
register count
combinational cell count
CAM-equivalent bits
```

---

# 33. Mandatory RTL Testbench Cases

## TB01 — 2R2C baseline

Bit-match the existing RECAM simulator.

---

## TB02 — five independent pivots

Verify shared pivot storage reaches five entries.

Verify per-config visibility:

```text
3 / 4 / 5 / 4
```

for:

```text
2R1C / 2R2C / 3R2C / 3R1C
```

---

## TB03 — config-dependent overflow

Fourth pivot:

```text
2R1C:
CAMReuseTemporaryBuffer

2R2C:
Address pivot
```

---

## TB04 — Must threshold

Two faults on same row:

```text
2R1C -> RowMust
2R2C -> not RowMust
```

---

## TB05 — transpose

For a deterministic fault map:

```text
result(2R1C, fault_map)
```

must match the transposed equivalent:

```text
result(1R2C, transpose(fault_map))
```

after output coordinates are transposed back.

Repeat for:

```text
3R2C ↔ 2R3C
3R1C ↔ 1R3C
```

---

## TB06 — multiple valid patterns

Verify lowest valid PatternID.

---

## TB07 — final decode

Verify:

```text
PivotPayload
+
ConfigID
+
PatternID
```

matches simulator conventional repair addresses.

---

## TB08 — EARLY directional resource conflict

Verify unavailable borrowed resource masks the corresponding config.

---

## TB09 — GROUP recovery

Construct:

```text
EARLY fails
GROUP succeeds
```

and match simulator.

---

## TB10 — impossible GROUP

All paths violate resource constraints.

Verify:

```text
group_success = 0
```

---

## TB11 — pending buffer

Verify no final eFuse-valid signal is generated until the full group succeeds.

---

## TB12 — CAM reuse isolation

Additional pivot information must appear in CAMReuse output but must not alter conventional PatternID.

---

# 34. Golden Comparison

Create machine-readable traces from both simulator and RTL.

At minimum compare:

```text
PivotPayload

per-config Must result

per-config PatternID

CAMReuseTemporaryBuffer

EARLY selected ConfigID
EARLY selected PatternID

GROUP selected ConfigIDs
GROUP selected PatternIDs

final decoded row addresses
final decoded column addresses
```

The test should fail automatically on mismatch.

---

# 35. Synthesis Variants

Prepare at least these designs.

## V0 — Baseline RECAM

```text
2R2C
no sharing
```

---

## V1 — Multi-Config Full Replication Reference

Optional reference if practical:

```text
independent config state
```

This is not the preferred architecture.

It exists only as a hardware-cost comparison.

---

## V2 — Shared CAM Multi-Config

Main proposal:

```text
single Pivot CAM
single tagged Hybrid store
shared counters
config-specific analyzer views
```

---

## V3 — Shared CAM + AREA Orientation

```text
four canonical analyzers
time-multiplex transpose pairs
```

---

## V4 — Shared CAM + PARALLEL Orientation

If time permits:

```text
seven physical config results concurrently
without duplicating CAM storage
```

---

# 36. Primary Comparisons

Generate:

```text
Area:
V0 vs V2 vs V3/V4

Analyzer latency:
V0 vs V3 vs V4

Storage:
duplicated logical state
vs
shared/tagged state

GROUP overhead:
EARLY vs GROUP
```

The most important question is:

> How much extra analyzer/control hardware is required to exploit Dynamic Spare Sharing while avoiding per-configuration CAM replication?

---

# 37. Do Not Optimize Prematurely

Implementation order:

```text
Phase H1
package + ConfigID/PatternID definitions

Phase H2
shared Pivot CAM

Phase H3
shared counters + Must decode

Phase H4
tagged Hybrid storage

Phase H5
CAMReuseTemporaryBuffer

Phase H6
canonical analyzer modules

Phase H7
lowest Pattern encoder

Phase H8
seven Config result generation

Phase H9
EARLY

Phase H10
GROUP + Resource Ledger

Phase H11
Pending Repair Buffer + final decoder

Phase H12
simulator-vs-RTL regression

Phase H13
synthesis

Phase H14
area optimization
```

Correctness must precede storage compression.

---

# 38. Source Boundary

Treat the following as inherited RECAM concepts:

```text
Address/Hybrid CAM
pivot/nonpivot
Must repair
matrix conversion
fault-free-area candidate analysis
CAM reuse
temporary additional-pivot storage
```

Treat the following as this project's proposed architecture:

```text
shared CAM across configurations
config tags/logical views
shared counters
four canonical analyzers
R/C transpose reuse
seven ConfigIDs
smallest valid Pattern representative
EARLY
GROUP
Directional Resource Ledger
ConfigPatternMap
Pending Repair Buffer reuse
```

Keep this distinction in comments, README, and experiment reports.

---

# 39. RTL Acceptance Criteria

The RTL is complete when:

```text
[ ] only one shared Pivot/Address CAM exists
[ ] only one physical tagged Hybrid storage structure exists
[ ] counters are shared
[ ] Must thresholds are config dependent
[ ] all seven ConfigIDs are supported
[ ] four canonical analyzer definitions exist
[ ] transpose pairs pass equivalence tests
[ ] PatternID is separate from ConfigID
[ ] PatternID width supports 10 candidates
[ ] lowest valid Pattern is deterministic
[ ] additional pivots go to CAMReuseTemporaryBuffer
[ ] additional pivots do not modify conventional PatternID
[ ] ConfigPatternMap does not duplicate ConfigID
[ ] EARLY matches simulator
[ ] GROUP matches simulator
[ ] directional borrowing conflicts are enforced
[ ] final repair decode matches simulator
[ ] final eFuse output occurs only after group success
[ ] synthesis reports are generated per major block
```

---

# 40. Final Architectural Principle

The RTL should embody the following separation:

```text
Fault information
        ↓
Shared fault collection hardware
        ↓
Configuration-specific interpretation
        ↓
Local RECAM PatternID
        ↓
Solution Take Policy
        ↓
Physical spare-resource allocation
        ↓
Pending final repair information
        ↓
eFuse
```

The central optimization is:

> **Share expensive fault-information storage across repair configurations, while retaining only the minimum configuration-dependent metadata and analysis logic required to evaluate each resource envelope.**
