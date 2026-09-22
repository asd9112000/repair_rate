# FINAL-EARLY-C1R2 — RECAM Hybrid Entry and Config-Analyzer Boundary Audit

```text
FINAL_EARLY_C1R2_STATUS: COMPLETE
```

This is a read-only audit on
`integration/date2026-directional-canonical-v1`.  It corrects the scope of
the prior C1R conclusion without changing production RTL, simulator behavior,
priority, synthesis collateral, or `dss_final/`.

The central result is narrow but important: Vector 216's C:R rejection is
valid for the **current standalone-per-configuration C++ model**, but it is
not the result of the intended shared-collector/config-serial analyzer
boundary.  With the complete shared logical state presented to Config 1
(2R1C), the canonical analyzer returns valid Pattern 2.

## 1. Correct terminology

C1R used “descriptor” where it meant a whole Hybrid CAM record.  That was
incorrect terminology.

One logical Hybrid CAM **entry** contains the following stored fields:

| Field | C++ simulator | Frozen logical RTL form | Function |
| --- | --- | --- | --- |
| valid | `HybridCAMEntry::enable` | `hybrid_valid_i[h]` | entry allocation/membership |
| pivot pointer | `pointer` | `hybrid_pointer_flat_i[h*3 +: 3]` | selects the related Address-CAM pivot |
| descriptor | `descriptorRowIsDiff` | `hybrid_descriptor_i[h]` | one-bit differing-dimension metadata |
| differing address | `faultPtr` coordinate relative to pivot | `hybrid_differing_flat_i[h]` | supplies the non-common coordinate |

The RECAM formula counts **entries**, not descriptor bits:

```text
HybridCAMEntries = Rs*(Cs-1) + Cs*(Rs-1)
2R1C = 2*(1-1) + 1*(2-1) = 1 Hybrid CAM entry
2R2C = 2*(2-1) + 2*(2-1) = 4 Hybrid CAM entries
```

The current `RECAM_SPEC.md` §8 and the paper's Table I establish the formula.
The frozen Phase-3B interface explicitly defines the packed logical entry as
`valid + pointer[2:0] + descriptor + differing_address[8:0]`.

```text
C1R_USED_DESCRIPTOR_AS_ENTRY_SYNONYM: YES
CORRECT_CAPACITY_UNIT: HYBRID_ENTRY
DESCRIPTOR_IS_ONE_BIT_FIELD_PER_HYBRID_ENTRY: YES
```

## 2. Descriptor data path and current executable convention

The paper's Fig. 3 calls the field an “Address CAM address descriptor.”  Its
Fig. 5 describes the stored-row case as descriptor zero.  The current project
uses a polarity named for the *relation*, not the paper's stored-address
label.  The project conflict register already records this terminology/polarity
boundary.  All results below use the executable convention:

```text
descriptor = 0: same pivot row; differing address is a column
descriptor = 1: same pivot column; differing address is a row
transpose:       invert the differing-dimension interpretation
```

| Layer | File / field | Logical width | Stored or transient | Consumer |
| --- | --- | ---: | --- | --- |
| Simulator | `inc/RECAM_CAM.hpp`, `HybridCAMEntry::descriptorRowIsDiff` | 1 Boolean | stored in each vector entry | `RECAM_addressCAM::updateRowColMust`, `RECAM_PE::genFaultAnalyzeMatrix` |
| Historical shared collector | `rtl/dss_2x2/analyzer/shared_fault_collector.sv`, `descriptor`; `tagged_hybrid_store.sv`, `descriptor_mem` | 1 | stored | per-Config membership/Must projection and `config_analyzer` |
| Retained shared collector | `rtl/dss_v2/top/recam_dss_v2_retained_collector_bank.sv`, `hybrid_descriptor[0:10]` | 1 | retained logical state | 232-bit selected-Config projection |
| Canonical shared analyzer | `rtl/recam/recam_shared_config_analyzer.sv`, `hybrid_descriptor_i[H]` | 1 per input entry | input state; transiently decoded in matrix build | `differing_is_row = descriptor ^ transpose` |
| Matrix conversion | `RECAM_PE.cpp` / shared analyzer matrix loop | 1 read per Hybrid entry | transient interpretation | matrix bit, dictionary extension, line constraint |

Thus the descriptor is present in both models, but it is neither a capacity
unit nor a standalone record.  The matrix builder does not recreate
classification: it consumes the entry's pointer, descriptor, and differing
address to add the corresponding relationship.

```text
DESCRIPTOR_SIMULATOR_ONLY: NO
DESCRIPTOR_PRESENT_IN_SIMULATOR: YES
DESCRIPTOR_PRESENT_IN_RTL: YES
DESCRIPTOR_IS_CAPACITY_UNIT: NO
```

## 3. RECAM algorithm authority and boundary

The checked behavior is classified by authority rather than treating every
implementation choice as paper-mandated.

| Step | Status | Evidence |
| --- | --- | --- |
| Consume faults in supplied order; greedy first pivot match | PROJECT_DERIVED | `FaultList::classifyFaults`, `shared_fault_collector` scan order |
| Independent fault -> Address-CAM pivot | PAPER_EXPLICIT | paper §III-B/Fig. 4; `RECAM_SPEC.md` §§5,7 |
| Related nonpivot -> Hybrid logical entry | PAPER_EXPLICIT | paper Fig. 3/Fig. 5; `RECAM_SPEC.md` §8 |
| `row_count > Cs` / `column_count > Rs` Must rules | PAPER_EXPLICIT | paper §III-B/III-D; `RECAM_SPEC.md` §6 |
| Must-triggering fault does not allocate a new Hybrid entry | PROJECT_DERIVED | stated project rule in `RECAM_SPEC.md` §9.2 |
| Reclaim prior entries after Must | PROJECT_POLICY | explicitly identified as project policy in `RECAM_SPEC.md` §9.2-C |
| Build `K=(Rs+Cs)` matrix from pivots/Must/Hybrid state | PAPER_EXPLICIT / PROJECT_DERIVED | paper Fig. 6/§III-D; current exact dictionary cases in source |
| ordered dictionary extension/full-line handling | PROJECT_DERIVED | `RECAM_PE.cpp`, `recam_shared_config_analyzer.sv` |
| enumerate exactly `C(Rs+Cs,Rs)` row/column patterns | PAPER_EXPLICIT | paper Fig. 8 solution space; `SolGenerator`, shared analyzer pattern table |
| candidate valid iff every asserted matrix cell is covered | PROJECT_DERIVED from paper matrix semantics | same predicate in C++ and RTL |
| retain lowest valid canonical PatternID | PROJECT_POLICY | project ordering contract |

The frozen Phase-3B boundary is unambiguous:

```text
Address CAM logical state + Hybrid CAM logical state + Must state
    -> Matrix Builder -> candidate evaluation -> PatternID
```

It is explicitly not a raw-fault interface and does not reclassify faults.
`recam_shared_config_analyzer` has no Hybrid allocation state; it only walks
the supplied valid entries and separately gates candidates with the supplied
`conventional_overflow_i`.  Therefore it cannot itself determine that a
hypothetical 2R1C collector would have overflowed.

```text
HYBRID_CAPACITY_CHECK_OCCURS_IN: MULTIPLE — C++ per-Config collector; RTL shared collector/driver; analyzer only gates supplied overflow
CONFIG_ANALYZER_BOUNDARY_MATCHES_FROZEN_INTERFACE: YES
```

## 4. Paper versus project overflow policy

The paper specifies Hybrid CAM collection for nonpivots and explicitly shows
the Address-CAM-full -> temporary CAM-reuse path for additional pivots.  It
does not specify a Hybrid-CAM-full branch for a required nonpivot.  Current
`RECAM_SPEC.md` §9.2-A says this explicitly: required nonpivot + full Hybrid
CAM + no Must transition -> unrepairable is a conservative **project rule**
for an underspecified paper case.  The C++ implementation applies it when
`RECAM_hybridCAM::addHybridCAMEntry` cannot allocate a record;
`RECAM_PE::loadFaultsToCAMs` then makes the attempt unrepairable.

```text
HYBRID_OVERFLOW_FAILURE_PAPER_EXPLICIT: NO
HYBRID_OVERFLOW_FAILURE_PROJECT_POLICY: YES
```

This does not invalidate a standalone model; it prevents describing its
overflow decision as unquestionably required by the paper.

## 5. Standalone and shared-DSS models are different

| Model | Collection semantics | Current implementation evidence |
| --- | --- | --- |
| A — standalone RECAM per Config | Each `(Rs,Cs)` attempt creates its own `FaultList`, `RECAM_PE`, Address CAM, Hybrid CAM, and matrix. Hybrid capacity is that Config's formula. | `DynamicRepairSimulator::runAttempt` calls `RECAMSolverAdapter::solve` once for every capacity option; the adapter constructs `FaultList` and `RECAM_PE` using `request.availableRows/availableColumns`. |
| B1 — shared storage but emulated independent collection limit | One collector would reject an entry merely because a selected Config's hypothetical standalone store is full. | Not the intended Phase-3B boundary. |
| B2 — shared maximum logical collector, then Config-specific analysis | Store the fault relation once; ConfigID selects pivot prefix, membership, Must thresholds, matrix size, and candidate set. It does not allocate a separate Hybrid store. | `shared_fault_collector` has one 14-entry tagged store with Config masks; retained collector stores one H0..H10 sequence and projects the selected Config's ordered post-Must view. |

`DirectionalMultiConfigAnalyzer` is a **mixed helper**, not a completed B2
candidate oracle: it collects shared pivot/count metadata, but then invokes
the standalone `RECAMSolverAdapter::solve` independently for each Config.
The production `DirectionalV2Early` path likewise runs each capacity option
through the standalone adapter.  That is the exact coupling responsible for
Vector 216's C:R rejection.

The intended final-DSS RTL is B2.  The historical shared collector has 14
physical Hybrid slots (at most 11 reachable with `MAX_FAULTS=12`); the retained
canonical collector stores the 11 reachable entries and provides a 9-entry
post-Must selected-Config view.  The old seven-entry Phase-3B analyzer port is
an analyzer-input contract, not a physical collector store.  For Vector 216,
all three capacities (14, 11, and the relevant 9-entry view) retain both
required records.

```text
CURRENT_SIMULATOR_MODEL: A
CURRENT_RTL_MODEL: B2
INTENDED_FINAL_DSS_MODEL: B2
RTL_PHYSICALLY_HAS_ONE_HYBRID_STORE_PER_CONFIG: NO
PHYSICAL_SHARED_HYBRID_ENTRY_COUNT: 14 historical allocated; 11 retained/reachable canonical state
MAX_HYBRID_ENTRIES: 14 historical physical; 11 retained/reachable; 9 selected-Config post-Must view; 7 legacy analyzer-port assumption
RTL_SHARED_HYBRID_STORE_CAN_HOLD_VECTOR216_TWO_RECORDS: YES
```

Config serialization reads the same retained state.  It does not recollect
faults for each cycle.  Config-specific membership and Must filtering are
interpretation/projection rules; they are not per-Config physical Hybrid
allocation.  The retained nine-slot path does not truncate Vector 216.

```text
CONFIG_SERIAL_ANALYZER_USES_SAME_SHARED_FAULT_STATE: YES
CONFIG_CYCLE_RECOLLECTS_FAULTS: NO
CONFIG_CYCLE_ARTIFICIALLY_TRUNCATES_HYBRID_STATE: NO (for Vector 216; broader 7-slot historical boundary limitations remain separately documented)
```

## 6. Vector 216: exact two-entry state and four paths

The C fault arrival order is:

```text
P0 = (493,355)                  pivot
H0 = (860,355), pointer=0, descriptor=1, differing_row=860
P1 = (491,353)                  pivot
H1 = (677,353), pointer=1, descriptor=1, differing_row=677
```

Both Hybrid records are same-pivot-column/different-row relations.  Their
column counts become two; for `Rs=2`, neither has `column_count > Rs`, so
there is no ColMust transition.  No RowMust transition occurs.  Therefore the
complete logical fault state genuinely requires two Hybrid **entries**.

| Path | Fault collection success | Address entries / available | Hybrid entries required / available | Must transitions | Complete logical state | Matrix analysis | Pattern | Final Config valid |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1. Standalone project RECAM, 2R1C | No | 2 / 3 | 2 / 1 | none | No; H1 cannot allocate | partial 3x3 runs | raw partial P2/P3 exist, but none admitted | No |
| 2. Shared maximum collector -> 2R1C analyzer | Yes | 2 / 5 shared maximum; 2 selected prefix | 2 / 14 historical or 11 retained | none | Yes | 3x3 Config-1 analysis | P2 valid | Yes |
| 3. Current production DirectionalV2Early C++ | No | 2 / 3 | 2 / 1 | none | No | partial standalone analysis | no admitted PatternID | No |
| 4. Canonical shared analyzer with complete shared state | Yes at its input boundary | 2 active pivots | 2 supplied / 9 retained-view capacity | none | Yes | runs | P2 valid | Yes |

For path 4, a temporary parameter-preserving Verilator check drove both H0 and
H1 and no overflow into `recam_shared_config_analyzer`:

```text
C_R_2R1C_COMPLETE_SHARED_STATE bitmap=0x2 pattern=2 solution_valid=1 repairable=1
```

For contrast, applying the standalone collector-overflow signal to the same
analyzer yields zero candidates.  That was the prior C1R RTL cross-check.  It
validates only “the analyzer obeys the asserted overflow input,” not that the
shared-collector architecture should assert it for Config 1.

```text
VECTOR216_ACTUALLY_REQUIRES_TWO_HYBRID_ENTRIES: YES
```

## 7. Consequences

The statement “C:R is invalid because 2R1C Hybrid capacity is one while the
solution requires two descriptors” is:

- **correct as standalone RECAM semantics**, after replacing “descriptors”
  with “Hybrid entries”;
- a **project-policy-dependent** failure branch, because the paper does not
  explicitly prescribe Hybrid-full handling; and
- an **architecture mismatch** if used as final shared-DSS semantics, because
  B2 collects a complete shared fault state before serial Config analysis.

It is not evidence of a candidate-enumeration defect in
`recam_shared_config_analyzer`, nor is it a defect in the standalone C++
model relative to its own chosen Model-A contract.  The mismatch is the use of
Model A to judge a B2 final-DSS analyzer interface.

Under the correct B2 architecture, Vector 216 C:R is valid and selected before
C:L under the proposed R-first exploration.  Thus this vector's specific
`C:L:R=I` claim-mask witness disappears.  The broader C1 ambiguity proof was
derived from the Model-A corpus and must be regenerated under a faithful B2
simulator before deciding whether any actual-demand/claim-mask interface is
needed.

```text
C1R_ROOT_CAUSE_CLASSIFICATION: CORRECT_AS_STANDALONE_RECAM_SEMANTICS; CORRECT_BUT_TERMINOLOGY_WRONG; PROJECT_POLICY_ONLY; ARCHITECTURE_MISMATCH_FOR_FINAL_SHARED_DSS
COMPLETE_SHARED_STATE_SHOULD_BE_ANALYZED_INDEPENDENT_OF_PER_CONFIG_STORAGE: YES
VECTOR216_AMBIGUITY_SURVIVES_CORRECT_FINAL_ARCHITECTURE: NO
CLAIM_MASK_WORK_SHOULD_PROCEED: WAIT
```

## Reproducibility and audit limits

The following focused checks were run without modifying functional source:

```text
make test_directional_multi_config_analyzer
temporary Verilator shared-state replay: PASS
```

The temporary Vector-216 RTL output is confined to:

```text
tmp/date2026/final_early_c1r2/vector216_shared_state_rtl_crosscheck.txt
```

The shared-state result proves the Vector 216 boundary conclusion.  It does
not claim that every historical shared-collector state fits the legacy
seven-entry analyzer input; the documented 9-entry retained projection exists
because broader historical states can exceed seven.

## Final status

```text
FINAL_EARLY_C1R2_STATUS: COMPLETE
CORRECT_HYBRID_CAPACITY_TERM: HYBRID CAM ENTRY
DESCRIPTOR_IS_ONE_BIT_FIELD_PER_HYBRID_ENTRY: YES
DESCRIPTOR_PRESENT_IN_SIMULATOR: YES
DESCRIPTOR_PRESENT_IN_RTL: YES
2R1C_HYBRID_ENTRY_CAPACITY: 1
2R2C_HYBRID_ENTRY_CAPACITY: 4
VECTOR216_REQUIRED_HYBRID_ENTRIES: 2
HYBRID_CAPACITY_CHECK_OCCURS_IN: MULTIPLE — simulator per-Config collector and RTL shared collector/driver; analyzer gates only supplied overflow
HYBRID_OVERFLOW_FAILURE_PAPER_EXPLICIT: NO
HYBRID_OVERFLOW_FAILURE_PROJECT_POLICY: YES
CURRENT_SIMULATOR_MODEL: A
CURRENT_RTL_MODEL: B2
INTENDED_FINAL_DSS_MODEL: B2
CONFIG_ANALYZER_BOUNDARY_MATCHES_FROZEN_INTERFACE: YES
CONFIG_SERIAL_ANALYZER_USES_SAME_SHARED_FAULT_STATE: YES
CONFIG_CYCLE_ARTIFICIALLY_TRUNCATES_HYBRID_STATE: NO (for Vector 216)
VECTOR216_ACTUALLY_REQUIRES_TWO_HYBRID_ENTRIES: YES
RTL_SHARED_HYBRID_STORE_CAN_HOLD_VECTOR216_TWO_RECORDS: YES
C1R_ROOT_CAUSE_CLASSIFICATION: ARCHITECTURE_MISMATCH_FOR_FINAL_SHARED_DSS; standalone result remains project-policy-dependent
COMPLETE_SHARED_STATE_SHOULD_BE_ANALYZED_INDEPENDENT_OF_PER_CONFIG_STORAGE: YES
VECTOR216_AMBIGUITY_SURVIVES_CORRECT_FINAL_ARCHITECTURE: NO
CLAIM_MASK_WORK_SHOULD_PROCEED: WAIT
FUNCTIONAL_SOURCE_MODIFIED: NO
RTL_MODIFIED: NO
SIMULATOR_MODIFIED: NO
SYNTHESIS_RUN: NO
WORKTREE_CLEAN_AFTER_COMMIT: YES
NEXT_ACTION: STOP FOR HUMAN RECAM-SEMANTIC REVIEW
```

```text
VERILOG_SKILL_RULES_USED:
- Read the readable-verilog-generator skill, dispatcher, and ASIC review
  reference before the read-only collector/analyzer audit.
- Reviewed the analyzer task class with no RTL mutation. The generated
  temporary harness supplied only external logical input to the existing DUT.
```
