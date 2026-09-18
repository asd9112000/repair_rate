# Device shared-CAM GO / NO-GO audit

## Status

```text
STATUS: AUDIT_COMPLETE
CAM_SHARING_SCOPE: PER_DEVICE (GLOBAL_LOGIC_DIE)
ONE_CAM_SERVES_GROUPS: 2048 in the frozen WoW-v1.0 reference geometry
ONE_CAM_SERVES_REPAIR_DOMAINS: 8192 in the frozen WoW-v1.0 reference geometry
CAM_ENTRY_COUNT: 4 at default Rs=Cs=2 (otherwise Rs+Cs, unless explicitly overridden)
CAM_ENTRY_WIDTH: 28-bit online address tag + 259-bit replacement entry; 287 online logical bits per capacity slot
CAM_ENTRY_LIFETIME: committed after successful Tier-2 allocation; retained for modeled-device runtime; never released during scheduler run
CAM_FULL_BEHAVIOR: transactional Tier-2 group allocation failure; no partial new assignments are committed; the group fails
GROUP_LEVEL_q_AVAILABLE: NO
EXPECTED_CAM_DEMAND: NOT_NUMERICALLY_DETERMINABLE; frozen-geometry expression = 2048*q
CAM_PRESSURE: NOT_NUMERICALLY_DETERMINABLE; frozen-geometry expression = 512*q
DEVICE_REPAIR_RATE_DEFINITION: device_success=1 only when every modeled group finally repairs; group/bank/domain fractions are separate outputs
DEVICE_SIMULATION_RECOMMENDATION: NO_GO_FULL_DEVICE
RECOMMENDED_NEXT_EXPERIMENT: CAM_SHARING_SCOPE_SENSITIVITY
ADDITIONAL_DEVICE_SIMULATION_REQUIRED: YES, but only after an explicit groups/domains-per-CAM sensitivity contract
SIMULATION_CODE_CHANGED: NO
FORMAL_DATA_TOUCHED: NO
```

No simulator was modified, no device simulation was started, and no formal
group-level artifact was read or written by this audit.

## Implemented hierarchy

```text
one DEVICE (one DeviceRepairScheduler / one GlobalOnlineRepairPool)
  -> 8 channel-like GBUS domains
       -> 4 banks per domain
            -> 64 repair groups per bank
                 -> 4 subarrays / repair domains (A, B, C, D) per group
```

| Required quantity | Implemented value | Qualification |
|---|---:|---|
| `CHANNELS_PER_DEVICE` | 8 | Called `channel-like domains` / `domain`; not an asserted commercial HBM channel count. |
| `BANKS_PER_CHANNEL` | 4 | Frozen WoW-v1.0 reference geometry. |
| `GROUPS_PER_BANK` | 64 | Frozen WoW-v1.0 reference geometry. |
| `SUBARRAYS_PER_GROUP` | 4 | Research modeling assumption. |
| `TOTAL_GROUPS_PER_DEVICE` | 2048 | `8 × 4 × 64`. |
| `TOTAL_REPAIR_DOMAINS_PER_DEVICE` | 8192 | `2048 × 4`. |

`--groups N` limits the number of generated input groups evaluated by one run
(`1..2048`); it does not alter the hardware geometry or create a smaller
physical device. File input supplies explicit hierarchy addresses, but the
scheduler accepts only one device ID per global pool.

## CAM ownership and capacity

`DeviceRepairScheduler::run()` constructs one `GlobalOnlineRepairPool`.
`DeviceRepairResult.camScope` is `GLOBAL_LOGIC_DIE`; the pool rejects input
spanning multiple device IDs. It is one finite Tier-2 directory per modeled
memory device, shared by groups, banks, domains, and BIRA engines.

The default online capacity is `Rs + Cs`. For `Rs=Cs=2`:

```text
ONLINE_CAM_ENTRY_COUNT: 4 distinct GlobalRepairTag values
ONLINE_ADDRESS_TAG:     valid(1) + domain(3) + bank(2) + group(6)
                        + subarray(2) + row(9) + word-column(5) = 28 bits
ONLINE_HYBRID_ENTRY:    valid(1) + pointer(2) + replacement data word(256)
                        = 259 bits
ONLINE_LOGICAL_TOTAL:   4 × (28 + 259) = 1148 bits
PHYSICAL_MODE_REUSED:   4 × (max(offline Address=29, online Address=28) + 259)
                        = 1152 bits
```

There is not one useful scalar entry width: a capacity slot is represented by
the global address-tag directory and its replacement-word state. The 1152-bit
value is implemented physical storage accounting; 1148 bits is the online-only
logical representation. `--online-global-reuse-entries N` overrides capacity
and is marked `EXPLICIT_OVERRIDE`.

The stored `GlobalRepairTag` is
`(domain, bank, group, subarray, row, repair_column)`. At default `DATA_WORD`
granularity, `repair_column = cell_column / 256`. An entry is a persistent
data-word fallback mapping, not a spare line or per-group scratch record.

## Entry lifetime and CAM-full behavior

The implemented transition is:

```text
Tier-0 local line repair fails
  -> optional Tier-1 in-group sharing fails
       -> Tier-2 analysis with a large functional per-group CAM container
            -> deduplicate BufferRepairMapping values into GlobalRepairTag values
                 -> reserve the complete request in the one global pool
```

Allocation happens only after Tier-2 analysis succeeds. Existing tags are
successful deduplicated hits. For novel tags:

```text
all novel tags fit:       commit all and increase persistent occupancy
any novel tags do not fit: commit none of that group's novel tags; allocation
                            failures equal the novel-tag count
```

This is neither a stall nor capacity degradation. It makes the Tier-2-dependent
group `finalSuccess=0`; `device_success=1` only when every modeled group
succeeds. A failed group consumes none of the unavailable tags.

Committed assignments are never released by the scheduler. Offline BIRA scratch
is cleared after every group, but that clear does not release the global
assignment. Entries leave state only when a separate simulated device's
in-memory scheduler/pool is destroyed, not through a modeled runtime release.

```text
WHEN_CAM_ENTRY_ALLOCATED: after successful Tier-2 group analysis, at global-pool reservation
WHEN_CAM_ENTRY_RELEASED: never during the modeled device runtime
ENTRY_RETAINED_DURING_RUNTIME: YES
MULTIPLE_GROUPS_CAN_OCCUPY_CAM_SIMULTANEOUSLY: YES
```

## Existing normalized 1k data and CAM dilution

The frozen normalized 1k root contains independent 4-SA group outcomes. Its
sidecars provide `repair_success`, selected candidate/configuration, borrowing,
local spare accounting, and group-local generic CAM accounting. They do not
contain the device-scheduler observables needed to identify a Tier-2 fallback:

```text
tier0_success
tier1_success
tier2_success with unrestricted functional CAM
repair_source = CAM_REUSE_REQUIRED or SHARING_AND_CAM_REUSE_REQUIRED
cam_words_needed after GlobalRepairTag word deduplication
```

Therefore group-level `repair_success` cannot be relabeled as "requires the
device-global CAM" without changing resource-ownership semantics:

```text
q = P(Tier-0/Tier-1 line repair fails AND Tier-2 functional analysis succeeds)
  = NOT AVAILABLE from completed normalized 1k data.
```

For the frozen full-device geometry, a future compatible corpus would use:

```text
G = 2048 repair groups per one CAM
E = 4 default online entries
EXPECTED_CAM_DEMAND = 2048*q
CAM_PRESSURE = (2048*q)/4 = 512*q
```

The nominal contention point is `q = 1/512 = 0.001953125` (about 0.1953%).
This is only an activation approximation: a group may need multiple unique
data-word tags. A faithful capacity model additionally requires
`W = cam_words_needed` after deduplication, using expected slot demand
`2048 × E[W]` and pressure `(2048 × E[W])/4`. The missing statistic is the
paired Tier-0/Tier-1/Tier-2 outcome and deduplicated `W` distribution by fault
density and topology, produced on an explicitly scoped device-compatible
corpus. It cannot be inferred from the normalized 1k aggregate.

## RECAM interpretation boundary

For thesis discussion, retain this boundary: RECAM's online CAM is modeled here
as a shared redundancy pool across evaluated channels, and RECAM experiments
report reduced repair benefit when more channels share a fixed pool. However,
RECAM does not fully specify the physical bank/subarray/MAT hierarchy that fixes
how many physical local repair domains share that pool.

This code supplies a WoW-v1.0 research hierarchy including a four-subarray
group assumption. It must not silently claim that this is paper-specified, nor
equate one RECAM channel with one physical subarray.

## Decision

`NO_GO_FULL_DEVICE` applies to a one-point full-device repair-rate sweep using
the present default pool. The scheduler's persistent-pool behavior is coherent;
the gate is model validity, not an implementation defect:

1. Four persistent fallback words are shared by 2048 groups / 8192 domains.
2. The fallback probability and word-demand distribution are absent from the
   completed group dataset.
3. Groups/domains per shared CAM is a research-architecture assumption rather
   than a RECAM-paper hierarchy fact.

The next design should be `CAM_SHARING_SCOPE_SENSITIVITY`: make groups-per-CAM
and domains-per-CAM explicit axes, then record `q`, `W`, occupancy, overflow,
and device success separately for every scope. Only after selecting and
documenting that scope should device repair rate be interpreted architecturally.

## Audit evidence

- `inc/HierarchicalRecamSimulator.hpp`: frozen geometry, global pool interface,
  global-tag definition, result metrics.
- `src/HierarchicalRecamSimulator.cpp`: one-pool construction, persistent
  reservation, transactional overflow, group/device failure propagation.
- `HierarchicalRECAM.cpp`: generated address mapping and `--groups <= 2048`.
- `docs/HIERARCHICAL_RECAM.md`: geometry and default 4-entry / 1152-bit
  Rs=Cs=2 accounting.
- `tmp/repair_rate_matrix_v2_normalized_1k`: read only; raw and aggregate
  group-level schemas lack Tier-2 fallback and deduplicated word-demand fields.
