# S1G-A2C — Historical Collector ↔ V2 Analyzer Capacity/Contract Reconciliation

## 1. Executive conclusion

**BLOCKED — Branch C.**  The historical `14` and frozen-V2 `7` identify
different storage/view layers, but the source does not support the required
claim that every historical Config-local analyzer view fits in seven entries.

The default historical collector allocates a single, append-only, physical
`tagged_hybrid_store` with 14 slots.  Its per-entry `cfg_valid[6:0]` mask is
derived from the matched pivot pointer and filters the shared physical array
for each Config.  The frozen V2 analyzer instead accepts one compact,
membership-free list of exactly seven logical Hybrid records and processes
every valid record as applicable to the currently selected Config.

A reachable default-parameter trace has three pivots followed by nine related
faults.  For Config 2 (3R2C), all nine records survive the historical Must
filter and are consumed in stored order.  Seven input slots cannot contain that
nine-record logical view without dropping or otherwise changing records.
Consequently, no universal lossless 14-to-7 projection exists, and neither the
current 204-bit analyzer boundary nor the unchanged V2 analyzer can be used as
the complete historical collector contract.

This is an audit result only.  No RTL, analyzer, baseline EARLY, GROUP, or
S1G-B work was changed or resumed.

## 2. Starting blocker

S1G-A2R established that the historical collector retains a per-Hybrid
ConfigID mask whereas the frozen 204-bit V2 input carries only seven untagged
logical records.  This audit resolves the remaining capacity question rather
than deriving a retained-state width.  Evidence priority is: implementation
first, then prior executable/reference evidence, then phase documents.

## 3. Historical 14-entry origin

| Item | Evidence | Finding |
| --- | --- | --- |
| Declaration | `rtl/dss_2x2/analyzer/shared_fault_collector.sv:6-10` | `HYBRID_SHARED_ENTRIES=14`, alongside `MAX_FAULTS=12`, `MAX_K=5`, and `NUM_CFG=7`. |
| Storage instantiation | `shared_fault_collector.sv:86-93` | The parameter is passed as `ENTRY_NUM` to one `tagged_hybrid_store`. |
| Storage implementation | `rtl/dss_2x2/analyzer/tagged_hybrid_store.sv:5-13,36-42` | A single physical array has 14 field sets; it is not two banks or row/column halves. |
| Consumer | `rtl/dss_2x2/analyzer/dss_analyzer_top.sv:91-99` | The historical multi-Config analyzer receives all 14 physical slots plus the Config masks. |
| Design-note derivation | `docs_verilog/03_ANALYZER_RTL_HANDOFF.md:435-467` | The stated conservative shared-union upper bound is `1 + 4 + 7 + 2 = 14`; it explicitly says this is not proof that 14 are always necessary. |

Thus `14` is **physical, global, shared Hybrid storage capacity** (and a
conservative implementation parameter), not per-Config candidate storage,
not a pair of seven-entry banks, and not a row/column partition.

There is an important default-parameter reachability qualification: each
accepted fault causes at most one `new_hybrid` write
(`shared_fault_collector.sv:66-70`), and the first accepted fault cannot be a
Hybrid because no pivot exists.  With `MAX_FAULTS=12`, at most **11** of H0
through H13 can be valid in one default transaction.  H12 and H13 are
allocated physical locations but unreachable under this exact default
acceptance bound.  Therefore a full 14-entry overflow is not reachable in the
frozen default instance, even though the physical capacity is 14.

## 4. V2 7-entry origin

| Item | Evidence | Finding |
| --- | --- | --- |
| Declaration | `rtl/recam/recam_shared_config_analyzer.sv:5-10` | `HYBRID_ENTRIES=7` is the frozen analyzer input-boundary parameter. |
| Consumer ports | `recam_shared_config_analyzer.sv:21-25` | Seven valid bits, seven 3-bit pointers, seven descriptors, and seven differing addresses are supplied; no per-entry Config membership port exists. |
| Consumer loop | `recam_shared_config_analyzer.sv:185-245` | The analyzer traverses every valid input record in slot order; it does not first apply historical membership filtering. |
| 204-bit packing | `docs/dss_execution/S1GA_EARLY_OVERLAP_RETAINED_STATE_CONTROL_IMPLEMENTATION.md:34-46` | The seven logical records occupy valid [111:105], pointer [132:112], descriptor [139:133], and differing address [202:140]. |
| Frozen rationale | `docs_verilog/PHASE3B_ANALYZER_INTERFACE.md:118-127` | The document calls seven the largest 3R2C/2R3C logical Hybrid capacity using `Rs(Cs-1)+Cs(Rs-1)`. |

Accordingly, `7` is a **per-selected-Config analyzer-input capacity** and
storage-accounting assumption.  It is not a compressed encoding of the
historical 14-slot physical store: the V2 boundary has neither 14 payload
slots nor membership bits, and the live V2 analyzer has no membership-filter
operation.

## 5. Capacity terminology

| Term | Value | Scope | Storage/view/limit | Source |
| --- | ---: | --- | --- | --- |
| Historical physical Hybrid capacity | 14 | global | allocated append-only store slots | `shared_fault_collector.sv:6-26`; `tagged_hybrid_store.sv:5-56` |
| Historical reachable physical occupancy | 11 | global, default parameters | accepted-transition bound, not allocated capacity | `shared_fault_collector.sv:66-73` |
| Historical membership-visible Hybrid maximum | 11 | per Config | `valid && cfg_valid`; before Must retirement | `shared_fault_collector.sv:59-63` |
| Historical analyzer-consumed maximum | 8/3/9/8/3/9/8 | per Config 0..6 | after membership and Must filtering | `multi_config_analyzer_bank.sv:35-56`; proof in section 7 |
| Historical per-Config dictionary capacity | `Rs+Cs` | per Config | transient `config_analyzer` row/column dictionaries | `config_analyzer.sv:16-43` |
| V2 Hybrid input slots | 7 | one selected Config | compact analyzer input | `recam_shared_config_analyzer.sv:9,21-24` |
| V2 analyzer-consumed count | 0..7 | one selected Config | all valid V2 slots are traversed | `recam_shared_config_analyzer.sv:185-245` |
| CAM-reuse capacity | 12 | global | separate temporary reuse buffer | `shared_fault_collector.sv:7,94-98` |
| Historical Hybrid full threshold | 14 | global | `occupancy == ENTRY_NUM` | `tagged_hybrid_store.sv:44-46` |
| Default-reachable Hybrid overflow threshold | unreachable | global | needs a 15th Hybrid write; only 11 can occur | transition bound above |

The terminology is therefore reconciled: 14 and 7 name different layers.
They are nevertheless semantically incompatible as a universal historical to
V2 projection because the historical Config-local consumed view can exceed 7.

## 6. Historical Hybrid membership semantics

Each physical entry H0 through H13 contains the same field schema:
`valid`, fault reference, row, column, descriptor, pivot pointer, and
`cfg_valid[6:0]` (`tagged_hybrid_store.sv:36-56`).  On each write the store
uses the current occupancy as its index, copies all fields, and increments
occupancy; there is no update, deletion, or compaction until reset/clear
(`tagged_hybrid_store.sv:60-79`).  Thus H0, H1, ... are strict insertion order.

`shared_fault_collector` finds the first matching pivot in ascending index
order, preferring a row match to a column match for the same pivot
(`shared_fault_collector.sv:49-58`).  It then sets membership for Config `c`
exactly when `relation_ptr < k(c)` (`:59-64`):

| ConfigID | Geometry | k | A record for pointer 0..2 | Pointer 3 | Pointer 4 |
| ---: | --- | ---: | --- | --- | --- |
| 0 | 2R2C | 4 | visible | visible | hidden |
| 1 | 2R1C | 3 | visible | hidden | hidden |
| 2 | 3R2C | 5 | visible | visible | visible |
| 3 | 3R1C | 4 | visible | visible | hidden |
| 4 | 1R2C | 3 | visible | hidden | hidden |
| 5 | 2R3C | 5 | visible | visible | visible |
| 6 | 1R3C | 4 | visible | visible | hidden |

This makes two facts precise:

1. Physical entries are not always visible to every Config.  The supplied
   fourth-pivot counterexample is reachable: a related record at pointer 3 is
   visible to `{0,2,3,5,6}` and hidden from `{1,4}`.
2. In this frozen collector, the mask is a deterministic function of the
   stored pointer and the fixed Config table.  It is still semantically absent
   from the V2 boundary because the existing V2 analyzer does not perform that
   Config-specific filter before traversing its input.

After membership, the historical bank retires a row-descriptor entry when the
corresponding RowMust bit is true, and a column-descriptor entry when
ColumnMust is true (`multi_config_analyzer_bank.sv:35-46`).  Retirement changes
the Config-local view but never clears the stored `cfg_valid` bit.

## 7. Per-Config maximum visible Hybrid entries

This section uses **visible** in its analyzer-relevant sense: a record that
passes valid, membership, pointer, and Must filtering at
`multi_config_analyzer_bank.sv:41-46`.  The separate pre-Must membership-only
maximum is 11 for every Config: one pivot at index 0 followed by 11 related
faults gives all records a pointer smaller than every Config's `k`.

For `p` relevant pivots, there are at most `12-p` Hybrid writes.  To avoid
Must retirement, each pivot can retain at most `(Rs-1)` row-descriptor records
and `(Cs-1)` column-descriptor records.  Therefore the exact upper bound is
the maximum of:

```text
min(12-p, p * ((Rs-1) + (Cs-1))), for 1 <= p <= k.
```

Each listed maximum is reachable by assigning unique nonpivot row/column
addresses so that every relevant row and column count remains at, not above,
its Must threshold.

| ConfigID | Geometry | Maximum consumed Hybrid records | Achieving pivot count | Reason |
| ---: | --- | ---: | ---: | --- |
| 0 | 2R2C | **8** | 4 | one row- and one column-descriptor record per pivot |
| 1 | 2R1C | **3** | 3 | only one column-descriptor record per pivot can avoid Must |
| 2 | 3R2C | **9** | 3 | one row- plus two column-descriptor records per pivot |
| 3 | 3R1C | **8** | 4 | two column-descriptor records per pivot |
| 4 | 1R2C | **3** | 3 | only one row-descriptor record per pivot can avoid Must |
| 5 | 2R3C | **9** | 3 | two row- plus one column-descriptor record per pivot |
| 6 | 1R3C | **8** | 4 | two row-descriptor records per pivot |

All entries in the 8/9-entry constructions have pointers below the respective
`k`; all required `cfg_valid` bits are consequently set.  Configs 0, 2, 3, 5,
and 6 thus each have a reachable analyzer-visible sequence longer than seven.

### Reachable CE-C2 (Config 2 has nine visible records)

Accept, in this order, three mutually distinct pivots:

```text
P0=(r0,c0), P1=(r1,c1), P2=(r2,c2)
```

Then for each pivot accept one same-row fault using a fresh column and two
same-column faults using fresh rows.  Example for P0:

```text
(r0,c10), (r10,c0), (r11,c0)
```

and use disjoint fresh coordinates for P1 and P2.  There are exactly twelve
accepted faults, nine Hybrid writes H0..H8, and no counter overflow.  For
Config 2 (3R2C), each pivot row has count 2 (not `>2`) and each pivot column
has count 3 (not `>3`), so no record is Must-retired.  The source therefore
presents all H0..H8 to `config_analyzer` in ascending stored order.  This is a
**REACHABLE** counterexample to a seven-entry view.

## 8. Dictionary/full/overflow comparison

| Property | Historical collector/analyzer | Frozen V2 analyzer | Equivalent? |
| --- | --- | --- | --- |
| Pivot dictionary | Up to `Rs+Cs`, derived per Config from shared pivot prefix | Up to `canonical_rows+canonical_cols` | only for an already equivalent Config-local input |
| Hybrid traversal | 14 physical slots filtered by membership and Must | exactly seven supplied slots, no membership filter | no |
| Hybrid dictionary extension | all surviving historical entries in H-index order | all valid input slots in 0..6 order | no for 8/9-entry states |
| Dictionary-full behavior | history-dependent ordered extension; line constraints handled in `config_analyzer.sv:28-43` | history-dependent ordered extension, but only seven records; invalid pointer asserts `dictionary_overflow_o` | no universal equivalence |
| Global collector overflow | separate counter, Hybrid, and reuse sticky flags | one `conventional_overflow_i` input; `dictionary_overflow_o` is local | no |
| Hybrid storage full | threshold 14, unreachable at default `MAX_FAULTS=12` | no physical-store state | no |

The historical top passes `counter_overflow || hybrid_overflow` to candidate
analysis (`dss_analyzer_top.sv:96-99`); CAM-reuse overflow is a separate
collector fact.  The current 204-bit value contains only one overflow bit.
Thus overflow distinctions also cannot be assumed preserved by the present
boundary.

## 9. Ordering and compaction semantics

Historical storage order is the append order H0, H1, ... and
`config_analyzer` explicitly scans increasing Hybrid index
(`config_analyzer.sv:28-43`).  Dictionary extension can allocate the first
free row/column position at the first record that needs it, so order is
architectural state.

Membership filtering can create holes.  A reachable example is four pivots
followed by a relation to pivot 3:

```text
H0(pointer 0, visible to all),
H1(pointer 3, visible to {0,2,3,5,6}),
H2(pointer 0, visible to all).
```

For Config 1 the view is H0,H2; for Config 2 it is H0,H1,H2.  A compactor that
keeps increasing H index preserves the historical order **when the selected
view has at most seven records**.  It cannot preserve the complete order of a
nine-record view in seven slots.

## 10. 14-to-7 projection analysis

The proposed hypothesis,

```text
global physical entries -> Config membership/Must filter -> <= 7 compact records
```

is false.  Filtering is deterministic from complete collector state and a
selected Config, but the resulting list is not bounded by seven.  The
reachable maxima in section 7 are 8 or 9 for five ConfigIDs.

For a state whose resulting list is at most seven, a selected-Config adapter
could encode that list in the V2 field layout without reordering it.  That
conditional observation is not a universal projection proof and does not make
the frozen 204-bit per-SA snapshot complete.

## 11. Analyzer-input aliasing analysis

The existing 204-bit input has seven Hybrid payload positions and no source
occupancy or membership-filter stage.  Any mapping that retains only seven of
the reachable nine records maps multiple distinct nine-record histories to the
same seven-slot input, regardless of whether it keeps a prefix, suffix, or
another deterministic subset.  The omitted records remain part of the
historical ordered dictionary traversal, so that mapping is not lossless.

The previously established pointer-3 membership case remains independently
relevant: V2 treats a supplied valid record as applicable, whereas history
would hide it from Configs 1 and 4.  Although the frozen collector's mask can
be recomputed from pointer and fixed `k`, the existing V2 datapath does not do
so; a complete collector-to-V2 boundary must supply or perform that selection.

An arbitrary pair with identical stored pointer/payload and different
`cfg_valid` masks is **UNREACHABLE** in this collector, because the mask is
deterministically assigned by `relation_ptr < k`.  It is not used as evidence.

## 12. Counterexamples

| ID | Class | Result | Reachability |
| --- | --- | --- | --- |
| CE-C1 | Physical occupancy >7 while one Config view is <=7 | The nine-entry Config-2 construction has physical occupancy 9; Config 1 retires all records under its stricter Must limits, so its effective view is 0.  A selected Config-1 view fits seven, but this does not make the global state representable by one seven-slot per-SA snapshot. | REACHABLE |
| CE-C2 | One Config-visible view >7 | Config 2 construction has H0..H8 all consumed. | REACHABLE |
| CE-C3 | Same seven selected records, different hidden records | A seven-slot truncation necessarily aliases distinct 8/9-entry historical sequences.  A bit-exact pair under an unspecified future truncation policy is not defined, so no stronger bit-level claim is made. | REACHABLE class; exact 204 mapping NOT_PROVEN |
| CE-C4 | Same payload, different membership mask | Not a legal independent state variable in this collector; mask is derived from pointer and fixed `k`. | UNREACHABLE as stated |
| CE-C5 | Holey membership | H0(pointer 0), H1(pointer 3), H2(pointer 0): Config 1 sees H0,H2; Config 2 sees H0,H1,H2.  Stable compaction retains the shown order only when capacity permits. | REACHABLE |
| CE-C6 | Physical storage full, Config not full | The allocated 14-slot state exists, but full occupancy is impossible with default `MAX_FAULTS=12`; no reachable default demonstration exists. | UNREACHABLE at default parameters |
| CE-C7 | Config dictionary constrained while physical slots remain | Must and ordered dictionary constraints can arise before 14 physical slots; the Config-2 nine-entry trace has only nine occupied physical slots. | REACHABLE |

## 13. Reachability analysis

The counterexamples use only the implemented acceptance and relation rules:
one accepted pivot, then one Hybrid write for each accepted related fault.
They respect the twelve-fault `fault_ready_o` limit, first-match pivot search,
append-only Hybrid allocation, and the actual Must predicates.  No arbitrary
mask, physical H12/H13 occupancy, or unimplemented multi-write behavior was
used.

The Phase-3B seven-entry formula is a documentation-level logical-capacity
assumption.  Its asserted maximum conflicts with the source-observable
9-record Config-local view.  By the defined evidence priority, the frozen
implementation controls this audit result; the disagreement is recorded rather
than silently resolved in favor of the formula.

## 14. 204-bit interface compatibility

| Question | Answer | Reason |
| --- | --- | --- |
| Q1: Can every Config-specific 7-entry view be generated losslessly from complete historical state? | **NO** | Some valid historical Config views contain 8 or 9 records, not 7. |
| Q2: Can a view that actually has <=7 records be encoded in the current 204-bit field layout? | **YES, conditionally** | Its seven logical slots and Must predicates can encode a selected ordered subset; the layout itself is adequate for that narrowed case. |
| Q3: Can the unchanged V2 analyzer reproduce every historical candidate result? | **NO** | It cannot consume a 8/9-record view and has no historical membership-filter stage. |

The exact missing boundary semantics are: (1) an ordered, complete
Config-local Hybrid sequence with capacity sufficient for every reachable
historical view; (2) the selected-Config membership/Must projection from the
full historical store; (3) associated complete collector occupancy/overflow
facts.  Per-entry `cfg_valid` must remain retained or be proven re-derivable
for any future collector variant; for the currently inspected collector it is
derivable from pointer and the fixed Config table, but it is not consumed by
the existing V2 boundary.

No target width is derived in this phase.

## 15. Contract comparison table

| Property | Historical Collector | Frozen V2 Analyzer | Equivalent? | Evidence |
| --- | --- | --- | --- | --- |
| Hybrid physical capacity | 14 allocated, 11 reachable at default | seven input slots, no collector store | no | sections 3-5 |
| Hybrid logical capacity | up to 9 post-Must for C2/C5 | at most 7 | no | section 7 |
| Per-Config visible capacity | membership 11; effective 8/3/9/8/3/9/8 | at most 7 valid slots | no | sections 6-7 |
| Entry membership | `cfg_valid`, then Must retirement | every input-valid record applies | no | `multi_config_analyzer_bank.sv:35-46`; `recam_shared_config_analyzer.sv:185-245` |
| Entry ordering | physical append order filtered stably | input slot order | conditional only | section 9 |
| Dictionary occupancy | all surviving ordered records | only seven supplied records | no | `config_analyzer.sv:28-43`; V2 `:185-245` |
| Dictionary full | Config-local ordered behavior | V2-local, with invalid-pointer overflow | no universal proof | section 8 |
| Overflow | counter/Hybrid/reuse distinctions | one input plus local flag | no | section 8 |
| Config invalidation | derived by view/candidate outcome; old mask bits are retained | no historical view state | no | sections 6, 8 |
| Config feasibility | all historical records participate | at most seven records participate | no | CE-C2 |
| Pattern feasibility / PatternID | ordered historical matrix result | ordered V2 result over truncated input | no universal equivalence | sections 9-10 |

## 16. Decision branch

**Selected model: C — historical and V2 capacities are fundamentally
inconsistent for complete historical collector semantics.**  The trigger is a
reachable per-Config analyzer-consumed count greater than seven.  This is not
Model A because the required filtered view is not universally <=7; it is not
Model B because merely adding membership awareness to a seven-slot analyzer
would not add the missing eighth/ninth record.

## 17. Remaining blockers

1. The frozen V2 seven-entry boundary cannot carry every reachable historical
   Config-local logical Hybrid sequence.
2. The existing V2 analyzer neither receives nor performs the required
   historical Config-specific membership/Must projection.
3. Existing overflow information is collapsed relative to historical collector
   state.
4. The Phase-3B seven-entry capacity rationale disagrees with the source
   behavior exposed by the default historical collector.  It must be resolved
   architecturally before retained-state sizing or overlap integration.

## 18. Recommended next phase

Human/architecture review is required to reconcile the historical source
contract with the frozen V2 analyzer contract.  A future authorization may
choose a revised source contract or a revised analyzer boundary, but this audit
does not select, size, or implement either.  S1G-B remains stopped.

```text
S1GA2C_STATUS:
BLOCKED

HISTORICAL_HYBRID_CAPACITY:
14

V2_HYBRID_BOUNDARY:
7

HISTORICAL_14_ENTRY_ORIGIN_PROVEN:
YES

V2_7_ENTRY_ORIGIN_PROVEN:
YES

CAPACITY_TERMINOLOGY_RECONCILED:
YES

HYBRID_CFG_VALID_SEMANTICS:
PROVEN

PER_CONFIG_VISIBLE_MAX_PROVEN:
YES

CONFIG0_MAX_VISIBLE_HYBRID:
8

CONFIG1_MAX_VISIBLE_HYBRID:
3

CONFIG2_MAX_VISIBLE_HYBRID:
9

CONFIG3_MAX_VISIBLE_HYBRID:
8

CONFIG4_MAX_VISIBLE_HYBRID:
3

CONFIG5_MAX_VISIBLE_HYBRID:
9

CONFIG6_MAX_VISIBLE_HYBRID:
8

ANY_REACHABLE_CONFIG_VISIBLE_GT7:
YES

14_TO_7_DETERMINISTIC_PROJECTION_EXISTS:
NO

PROJECTION_PRESERVES_ORDER:
NO

PROJECTION_PRESERVES_DICTIONARY_FULL:
NO

PROJECTION_PRESERVES_OVERFLOW:
NO

PROJECTION_PRESERVES_CONFIG_FEASIBILITY:
NO

PROJECTION_PRESERVES_PATTERN_FEASIBILITY:
NO

PROJECTION_PRESERVES_PATTERN_ID:
NO

204BIT_INTERFACE_CAN_REMAIN:
NO

EXISTING_V2_ANALYZER_CAN_REMAIN:
NO

SELECTED_ARCHITECTURE_MODEL:
C

MINIMUM_PROVEN_RETAINED_STATE_BITS_PER_SA:
NOT_PROVEN

V2_ANALYZER_CHANGED:
NO

BASELINE_EARLY_CHANGED:
NO

GROUP_CHANGED:
NO

S1GB_RESUMED:
NO

PRODUCTION_RTL_MODIFIED:
NO

GIT_DIFF_CHECK:
PASS

AUDIT_DOCUMENT:
docs/dss_execution/S1GA2C_HISTORICAL_V2_CAPACITY_CONTRACT_RECONCILIATION.md
```
