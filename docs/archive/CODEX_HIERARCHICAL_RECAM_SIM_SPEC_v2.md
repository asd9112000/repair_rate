# CODEX Hierarchical RECAM / WoW Repair Simulator Specification v2.0

> 文件狀態：Archive
> 適用範圍：hierarchical-device
> 建立時間：Unknown
> 最後修改時間：2026-09-02T00:00:00+08:00
> 本文件權威主題：歷史版 Hierarchical 設計規格；目前內容以主文件為準

## Purpose

The existing simulator was built under a mistaken assumption:

> each tile/subarray owns an independent RECAM CAM-reuse structure.

That assumption must be removed.

According to RECAM, the BISR resides on the **logic die**. Its BIRA contains the Address CAM, Hybrid CAM, matrix conversion, and redundancy analyzer. The repair-analysis flow selects channels, and the paper states that CAM redundancy resources are available to all channels. The online Address CAM explicitly includes channel information.

Therefore distinguish:

1. physical DRAM repair resources per subarray;
2. a four-subarray spare-sharing domain;
3. logic-die BISR scratch hardware reused across repair domains during offline BIRA;
4. the same physical CAM hardware reused in online mode as a shared data-repair resource.

Do not multiply RECAM CAM hardware by tile/subarray count.

Primary reference: J. Yoon et al., “A Built-In Self-Repair With Maximum Fault Collection and Fast Analysis Method for HBM,” IEEE TCAD 44(5), 2025, DOI 10.1109/TCAD.2024.3499903.

Use `WoW_DRAM_REFERENCE_ARCHITECTURE_v1.md` as the geometry source.

---

# 1. Required hierarchy

```text
Memory / WoW Stack
32 Gbit
│
├── Domain 0  (GBUS-based channel-like domain)
│   ├── Bank 0
│   │   ├── Group 0
│   │   │   +---+---+
│   │   │   | A | B |
│   │   │   +---+---+
│   │   │   | C | D |
│   │   │   +---+---+
│   │   ├── ... Group 63
│   ├── Bank 1
│   ├── Bank 2
│   └── Bank 3
├── ...
└── Domain 7

                Logic Die
                   │
         +---------+---------+
         | Shared BISR/BIRA  |
         | Address CAM       |
         | Hybrid CAM        |
         | Matrix Analyzer   |
         | Controller        |
         +---------+---------+
                   │
       time-multiplexed offline
                   │
                   ▼
          repair solution/eFuse

Normal operation:
         same CAM hardware
                   │
                   ▼
        Global CAM-Reuse Pool
```

---

# 2. Three repair tiers

## Tier 0 — Local spare-line repair

Domain: one subarray.

Resources:

```text
Rs local spare rows
Cs local spare columns
baseline Rs=Cs=2
```

Purpose:

```text
bad row / bad column -> local spare row / column
```

These are physical DRAM redundancy resources.

## Tier 1 — Constrained inter-subarray sharing

Domain: one 4-SA group.

```text
+---+---+
| A | B |
+---+---+
| C | D |
+---+---+
```

Purpose:

```text
local line repair fails
    -> borrow limited physical spare lines
       from another SA in the same group
```

This is still **line repair**, not CAM reuse.

Support initially:

```text
NO_SHARING
ROW_PAIRWISE
ROW_GROUP_LIMITED
IDEAL_ROW_COL_UPPER_BOUND
```

Recommended primary design:

```text
ROW_PAIRWISE or ROW_GROUP_LIMITED
m = 1
```

Keep `m=2` as sensitivity.

## Tier 2 — Global CAM-reuse repair

Purpose:

```text
residual faults after Tier 0 + Tier 1
    -> replacement-data repair using logic-die CAM reuse
```

Tier 2 is **not a spare row/column**.

A Tier-2 repair consumes a finite online repair-directory/replacement-data resource.

---

# 3. Critical correction: offline CAM is scratch hardware

The current simulator may effectively do:

```text
for each SA:
    allocate AddressCAM[Rs+Cs]
    allocate HybridCAM[...]
```

This may remain as a functional software representation temporarily, but hardware accounting must not interpret it as physical CAM-per-SA.

Correct physical interpretation:

```text
offline BISR engine
    -> select repair domain
    -> collect/analyze faults
    -> commit repair solution / CAM-reuse assignment
    -> clear/reuse BIRA scratch state
    -> select next repair domain
```

Default:

```yaml
bira_engines: 1
```

Optional sensitivity:

```yaml
bira_engines: [1, 2, 4, 8]
```

If multiple BIRA engines are modeled, area scales with actual engine count, not total SA count.

---

# 4. Three kinds of state

## A. Persistent physical local redundancy

```cpp
struct LocalRedundancy {
    int spare_rows_total;
    int spare_cols_total;
};
```

Exists per SA.

## B. Temporary offline BIRA scratch state

```cpp
struct BiraScratch {
    AddressCamState address_cam;
    HybridCamState hybrid_cam;
    MatrixState matrix;
    AnalyzerState analyzer;
};
```

Exists per **physical BIRA engine** and is reused.

Hardware cost:

```text
BIRA scratch hardware × number_of_bira_engines
```

Not:

```text
BIRA scratch hardware × total_subarrays
```

## C. Persistent final repair solution

Conceptually stored in eFuse/NVM:

```cpp
struct RepairSolution {
    DomainId domain;
    BankId bank;
    GroupId group;
    SubarrayId sa;

    vector<RowRemap> row_repairs;
    vector<ColRemap> col_repairs;
    vector<CamReuseAssignment> cam_repairs;
};
```

At power-up, CAM-repair assignments are loaded into the online CAM resource.

---

# 5. Offline repair scheduling

Default traversal:

```text
for domain in [0..7]:
    for bank in [0..3]:
        for group in [0..63]:
            run_group_repair(group)
```

Pseudo-flow:

```text
select group
    |
    v
Tier 0: local repair analysis
    |
    +-- success for all? -> commit local solution
    |
    no
    |
    v
Tier 1: constrained sharing
    |
    +-- repairable? -> commit local + borrowed-line solution
    |
    residual CAM-eligible faults
    |
    v
request Tier 2
    |
    +-- global CAM budget available?
            |
          yes -> reserve global CAM slot(s)
          no  -> group/device repair failure
```

A group may use Tier 0/Tier 1 and Tier 2 together.

---

# 6. Offline BIRA CAM vs online global reuse capacity

Do not conflate these concepts.

## Offline scratch capacity

For baseline `Rs=Cs=2`:

```text
Address CAM = 4 entries
Hybrid CAM  = 4 entries
```

Equations:

```text
AddressCAMEntries = Rs + Cs

HybridCAMEntries =
    Rs * (Cs - 1)
  + Cs * (Rs - 1)
```

## Online global reuse capacity

This is the finite number of data-repair addresses that can remain represented during normal operation.

Expose it as a physical design parameter:

```yaml
online_global_reuse_entries: [4, 8, 16, 32, 64, 128, 256]
```

**Hardware-accounting rule:** if the exact same physical CAM is reused offline and online, enlarging online capacity also enlarges physical CAM area offline. Extra entries are not free.

Do not double-count separate offline and online CAMs if one mode-reused structure is modeled.

---

# 7. Global CAM-reuse scope

Do NOT model a private online CAM pool for every SA.

Default:

```text
all repaired domains compete for one finite online pool
```

An online repair entry must identify its source.

For the 4-GiB reference architecture:

```cpp
struct GlobalRepairTag {
    uint8_t domain;       // 3 logical bits
    uint8_t bank;         // 2 bits
    uint8_t group;        // 6 bits
    uint8_t subarray;     // 2 bits
    uint16_t row;         // 9 bits
    uint8_t word_col;     // 5 bits for 256-bit words
};
```

Logical tag width:

```text
3 + 2 + 6 + 2 + 9 + 5 = 27 bits
```

plus valid/pointer/metadata.

This is a project-derived tag width.

---

# 8. Fault representation and CAM word granularity

Keep physical cell faults separate from runtime replacement words.

```cpp
struct Fault {
    uint8_t domain;
    uint8_t bank;
    uint8_t group;
    uint8_t subarray;

    uint16_t row;       // 0..511
    uint16_t cell_col;  // 0..8191

    FaultType type;
};
```

For 256-bit replacement:

```cpp
word_col = cell_col / 256;
```

Range:

```text
0..31
```

Add:

```cpp
RepairWordAddress canonicalizeForCamReuse(const Fault& f);
```

If multiple residual cells map to the same:

```text
(domain, bank, group, SA, row, word_col)
```

deduplicate them before consuming online CAM slots.

Make this configurable:

```yaml
cam_reuse_granularity: [CELL, DATA_WORD]
```

Default WoW experiment:

```yaml
cam_reuse_granularity: DATA_WORD
data_word_bits: 256
```

---

# 9. Sharing-policy interface

Do not hard-wire sharing into RECAM core.

Suggested interface:

```cpp
class SharingPolicy {
public:
    virtual GroupRepairBudget
    deriveBudget(const GroupState& group) = 0;

    virtual BorrowDecision
    allocate(const GroupState& group,
             const RepairDemand& demand) = 0;

    virtual ~SharingPolicy() = default;
};
```

Implement:

```text
NoSharingPolicy
RowPairwisePolicy
RowGroupLimitedPolicy
IdealRowColumnPolicy
```

RECAM analyzer receives an effective repair budget while physical ownership remains observable.

---

# 10. Recommended Tier-1 policies

## P0 — No Sharing

Each SA:

```text
2R + 2C
```

## P1 — Row Pairwise

```text
A <-> C
B <-> D
```

Maximum borrowed rows:

```text
m = 1 or 2
```

Columns local only.

## P2 — Row Group Limited

All A/B/C/D can access limited row-sharing capability inside the group.

Constraints:

```text
borrowed_rows_per_SA <= m
minimum_local_row_reserve >= r_min   # optional
```

Columns local only.

## P3 — Ideal Row+Column Upper Bound

Bounded row and column sharing for an architectural upper bound.

Do not present as the primary realizable architecture unless physical routing/remapping is modeled.

---

# 11. Tier-2 allocation

After Tier 0/Tier 1:

```text
residual_set = unique residual repair-word addresses
```

For each word:

```text
if global_pool.free_entries > 0:
    allocate entry
else:
    allocation failure
```

Use simple first-fit initially. Do not add complex global optimization yet.

---

# 12. Repair success hierarchy

```text
group_success  = all 4 SAs repaired
bank_success   = all groups repaired
domain_success = all banks repaired
device_success = all domains repaired
```

Also report softer metrics because full-device all-or-nothing probability can become tiny:

```text
fraction_groups_repairable
fraction_banks_repairable
fraction_domains_repairable
probability_device_repairable
```

---

# 13. Repair-source classification

Every successful group must be classified as exactly one:

```cpp
enum class RepairSource {
    LOCAL_ONLY,
    SHARING_REQUIRED,
    CAM_REUSE_REQUIRED,
    SHARING_AND_CAM_REUSE_REQUIRED
};
```

Count all four classes.

This is a key research metric: it reveals whether Tier 1 and Tier 2 solve complementary failures.

---

# 14. Global CAM metrics

Add:

```text
global_cam_capacity
global_cam_entries_reserved
global_cam_peak_occupancy
global_cam_utilization

cam_request_count
cam_allocation_success_count
cam_allocation_failure_count

cam_words_requested_before_dedup
cam_words_requested_after_dedup

groups_requesting_cam
banks_requesting_cam
domains_requesting_cam
```

Derived:

```text
global_cam_utilization = entries_reserved / capacity
cam_overflow_rate = allocation_failure_count / cam_request_count
```

---

# 15. Repair-stage metrics

Add:

```text
tier0_attempts
tier0_successes

tier1_attempts
tier1_successes
tier1_borrowed_rows
tier1_borrowed_cols

tier2_requests
tier2_successes
tier2_entries_consumed

fail_due_to_local_spare
fail_due_to_sharing_budget
fail_due_to_cam_capacity
```

Per group:

```text
fault_count
fault_count_A
fault_count_B
fault_count_C
fault_count_D

local_success_mask
sharing_used
borrowed_rows
borrowed_cols

residual_faults_after_tier0
residual_faults_after_tier1
cam_words_needed
cam_words_allocated

repair_source
final_success
```

---

# 16. Hardware-cost accounting correction

Do NOT calculate:

```text
CAM_area_per_SA × 8192
```

Use:

```text
TotalRepairHardware =
    local_spare_cost × 8192 SAs
  + sharing_control/routing_cost × 2048 groups
  + BIRA_engine_cost × num_bira_engines
  + persistent_eFuse/NVM_cost
  + shared online CAM cost
```

If offline and online CAM are the same physical CAM, do not double-count them.

Mode-reused structure rule:

```text
physical_CAM_area = area of the actually provisioned shared structure
```

not:

```text
offline_CAM_area + online_CAM_area
```

unless two separate structures are explicitly implemented.

---

# 17. Offline BIRA time model

Because BIRA engines are shared, manufacturing analysis time depends on number of repair domains processed.

Track:

```text
groups_processed_per_engine
cycles_per_group
total_bira_cycles
```

Scheduling baseline:

```text
1 engine: all groups serial
N engines: static round-robin groups across engines
```

Approximate total engine completion time:

```text
max(total_cycles_assigned_to_each_engine)
```

Do not equate C++ candidate loops directly with RECAM hardware cycles.

---

# 18. Offline vs online lifetime

```text
MANUFACTURING / OFFLINE

group 0 faults
    ↓
BIRA scratch CAM/matrix
    ↓
repair solution
    ↓
eFuse/NVM
    ↓
scratch reused for group 1

POWER-UP

eFuse/NVM
    ↓
load globally assigned CAM-repair tags
into Address CAM

RUNTIME / ONLINE

request address
    ↓
Address CAM match
    ↓
if hit: pointer -> Hybrid CAM replacement data
if miss: normal DRAM path
```

Offline fault information does not need to remain as a private CAM per SA after the solution is committed.

---

# 19. Preserve RECAM regression mode

## Original regression

```yaml
mode: recam_regression
multi_group: false
sharing: false
cam_scope: paper_style
```

Use existing RECAM/Table-II validation.

## WoW hierarchical mode

```yaml
mode: wow_hierarchical

domains: 8
banks_per_domain: 4
groups_per_bank: 64
subarrays_per_group: 4

Rs: 2
Cs: 2

sharing_policy: ROW_PAIRWISE
shared_rows_m: 1

bira_engines: 1

data_word_bits: 256
cam_reuse_scope: GLOBAL
```

Do not require the extended full-device model to reproduce a single-domain RECAM repair rate exactly.

---

# 20. Suggested config

```yaml
architecture:
  total_capacity_gbit: 32
  domains: 8
  banks_per_domain: 4
  groups_per_bank: 64
  subarrays_per_group: 4
  subarray_rows: 512
  subarray_cell_cols: 8192
  data_word_bits: 256

redundancy:
  Rs: 2
  Cs: 2

sharing:
  policy: ROW_PAIRWISE
  shared_rows_m: 1
  minimum_local_row_reserve: 1

bira:
  engines: 1
  offline_address_cam_entries: 4
  offline_hybrid_cam_entries: 4
  hardware_mode: RECAM_DYNAMIC

online_repair:
  technology: CAM
  scope: GLOBAL
  global_entries: 32
  repair_granularity: DATA_WORD

fault_model:
  type: CONFIGURABLE
  lambda_faults_per_sa: 1.0

simulation:
  trials: 10000
  seed: 12345
```

`global_entries=32` is an experiment parameter, not a literature value.

---

# 21. First experiment sequence

## Experiment 0 — Regression

Confirm original RECAM behavior still passes.

## Experiment 1 — Correct CAM ownership only

Keep no sharing. Change only hardware ownership:

```text
CAM_per_SA -> shared BISR scratch structure
```

Verify:
- equivalent local repair decisions remain unchanged;
- hardware CAM count changes;
- BIRA scheduler now reuses the engine across groups.

## Experiment 2 — Add Tier-1 row sharing

Compare:

```text
No Sharing
Row Pairwise m=1
Row Pairwise m=2
Row Group m=1
Ideal R+C upper bound
```

Metrics:

```text
group repair rate
borrowed rows
spare utilization
residual CAM demand
candidate/search complexity
```

**Key metric:** residual CAM demand after Tier 1.

## Experiment 3 — Global CAM-capacity sweep

Fix best constrained-sharing policy.

```text
global CAM entries = 4, 8, 16, 32, 64, 128, 256
```

Measure:

```text
group/device repairability
CAM occupancy
CAM overflow rate
fraction requiring Tier 2
```

## Experiment 4 — Scale number of repair domains

Keep SA size fixed. Vary number of groups/domains.

First keep global CAM capacity fixed.

Measure:

```text
CAM occupancy
CAM overflow probability
repairability
```

Then scale CAM capacity to find the capacity needed to maintain a target repairability.

---

# 22. Important plots after correction

```text
1. Repair-source composition:
   LOCAL / SHARING / CAM / SHARING+CAM

2. Residual CAM demand vs fault load

3. Global CAM occupancy vs number of groups

4. CAM overflow rate vs CAM capacity

5. Repair rate vs global CAM capacity

6. Repair rate vs number of groups/domains at fixed CAM capacity

7. Sharing policy vs reduction in Tier-2 CAM demand

8. BIRA engine count vs manufacturing BIRA time vs BIRA area
```

Plot 7 is especially important because constrained sharing may provide two benefits:

```text
repair-rate improvement
+
reduced demand on the finite global CAM-reuse pool
```

---

# 23. Required unit tests

## TEST-A — No physical CAM multiplication by SA count

Given:

```text
8192 SAs
1 BIRA engine
```

Expected physical BIRA CAM structures:

```text
1 Address CAM
1 Hybrid CAM
```

Functional per-SA software containers are allowed, but hardware accounting must still report one engine.

## TEST-B — Scratch reuse

Analyze group 0 -> commit solution -> clear BIRA scratch -> analyze group 1.

Expected:

```text
group 0 persistent solution survives
group 0 transient faults disappear from scratch
```

## TEST-C — Global CAM contention

Capacity 2. Three groups each need one distinct Tier-2 word.

Expected:

```text
2 succeed
1 fails
```

## TEST-D — Global tag disambiguation

Two groups have identical row/word_col.

Expected: different repair addresses because domain/bank/group/SA differ.

## TEST-E — Same-word dedup

At DATA_WORD granularity:

```text
same domain/bank/group/SA/row
cell_col=10
cell_col=200
```

Both belong to 256-bit word 0.

Expected: one Tier-2 request.

## TEST-F — Sharing can reduce CAM demand

Construct a case where no-sharing leaves one residual word for CAM, but one borrowed row repairs it.

Expected:

```text
Tier-2 demand: 1 -> 0
```

---

# 24. Implementation discipline

Do not rewrite the entire RECAM simulator.

Preferred migration:

```text
1. Preserve current RECAM core as RepairDomainAnalyzer.
2. Add Device / Domain / Bank / Group / Subarray hierarchy.
3. Add reusable BiraEngine.
4. Add SharingPolicy interface.
5. Add GlobalOnlineRepairPool.
6. Add device-level scheduler.
7. Add hardware-accounting layer.
8. Add regression tests before changing algorithms.
```

Suggested classes:

```cpp
class Device;
class ChannelLikeDomain;
class Bank;
class RepairGroup;
class Subarray;

class RecamDomainAnalyzer;
class BiraEngine;

class SharingPolicy;
class NoSharingPolicy;
class RowPairwisePolicy;
class RowGroupLimitedPolicy;

class PersistentRepairStore;
class GlobalOnlineRepairPool;

class DeviceRepairScheduler;
class HardwareCostModel;
class MetricsCollector;
```

---

# 25. Research hypothesis

Do not force this to be true; test it:

> Local spare sharing and global CAM reuse are hierarchical, potentially complementary resources. Constrained inter-subarray sharing may recover some local line-repair failures and reduce demand on a finite global CAM-reuse pool, while excessive sharing increases BIRA and routing complexity.

---

# 26. What NOT to implement yet

Do not add yet:

```text
SRAM replacement
cycle-accurate DRAM commands
NoC traffic
bank scheduling
complex CAM-allocation optimization
full RTL timing
column-sharing physical timing
```

First correct:

```text
resource ownership
hierarchy
repair flow
CAM occupancy
repair-rate accounting
```

Only after this baseline is validated should SRAM replace the global CAM or offline fault-storage component.

---

# 27. Acceptance criteria

```text
[ ] Original RECAM regression tests pass.
[ ] CAM hardware is not counted per tile/subarray.
[ ] One BIRA engine can analyze many groups sequentially.
[ ] Each SA has independent local physical spares.
[ ] Tier-1 sharing only uses resources of its 4-SA group.
[ ] Tier-2 uses one finite global CAM pool.
[ ] Persistent repair solutions survive BIRA scratch reuse.
[ ] Online CAM tags identify the complete source address.
[ ] Local / sharing / CAM-assisted repairs are separately reported.
[ ] Global CAM occupancy and overflow are observable.
[ ] Hardware accounting separates local spares, sharing logic,
    BIRA scratch hardware, persistent solution storage, and online CAM.
[ ] Legacy CAM-per-SA result files are not mixed with corrected results.
```

---

# 28. Migration warning

Existing results produced under:

```text
CAM_per_subarray = true
```

must be tagged:

```text
LEGACY_ARCHITECTURE
```

Repair-rate results may also require rerunning if CAM-reuse capacity was incorrectly reset independently for each SA/group.

Every new CSV should contain:

```text
architecture_version = WoW-v1.0
simulator_version = Hierarchical-RECAM-v2.0
cam_scope = GLOBAL_LOGIC_DIE
bira_engine_count = 1
```
