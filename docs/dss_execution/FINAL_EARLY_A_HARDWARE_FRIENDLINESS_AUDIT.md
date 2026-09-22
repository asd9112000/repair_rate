# FINAL-EARLY-A — Historical 2×2 Directional EARLY Hardware-Friendliness Audit

## Scope, boundary, and evidence

```text
FINAL_EARLY_A_STATUS: COMPLETE
MODE: READ ONLY
ARCHITECTURE: GRID2X2_DIRECTIONAL / RS=2 / CS=2 / m=1 /
              NORMALIZED_STREAMING_EARLY / NoScratch
RTL_MODIFIED: NO
MANIFEST_MODIFIED: NO
SYNTHESIS_RERUN: NO
```

This audit concerns only historical SYN-A, not the 2×2 GROUP_GLOBAL path and
not either 1×4 path.  Those paths have different candidate lifetime and
resource-ownership semantics, so their state or area must not be used as a
substitute for this finding.

Evidence order is: frozen SYN-A contract and hierarchy, frozen DC reports,
actual elaborated-source closure, then source-level inference.  A generic
source construct is not called area overhead unless it survives mapping or
the mapping report cannot disprove the source-grounded risk.

The `readable-verilog-generator` skill was used in its `analyze` route.
`SKILL.md`, `references/workflows/verilog_dispatcher.md`, and
`references/rules/asic-verilog-quality.md` were read and applied.  Its public
read-only CLI was also attempted.  The installed Python 3.8.10 stopped while
loading `dict[str, list[SignalDecl]]` in the skill's formatter backend.  No
skill-package file was changed.  Therefore the public evidence matrix is:

| Gate | Status | Evidence |
| --- | --- | --- |
| compile | PASS | frozen exact-top Verilator lint recorded by P3 four-point closure |
| ast/readability/comment/naming/profile/testbench | BLOCKED | skill Python-3.8 incompatibility before AST analysis |
| toolchain | PASS | frozen DC W-2024.09-SP2, TSMC018 slow, 20 ns |

Applied ASIC review rules: preserve the frozen clock/reset/interface and
ordering contract; distinguish fixed elaboration loops from runtime datapath;
inspect dynamic packed selection as mux/decoder logic; keep widths explicit;
and review high-fanout control and long priority/decode cones.  No FPGA-only
guidance was used as ASIC evidence.

## Actual SYN-A hierarchy

The accepted top is
`recam_dss_canonical_rs2_streaming_early_top`.  It fixes
`RESOURCE_POINT=2`, `PATTERN_ID_W=6`, `ROW_ADDR_W=9`, `COL_ADDR_W=5`,
`DIFF_ADDR_W=9`, and `HYBRID_ENTRIES=7`.

```text
top: recam_dss_canonical_rs2_streaming_early_top
├── analyzer: recam_shared_config_analyzer
└── policy_core: recam_dss_canonical_streaming_early_core
    ├── generate_rs2_decode/config_decode: dss_v2_group_slot_decode
    ├── generate_rs2_decode/feasibility: dss_v2_resource_feasibility
    │   └── topology: dss_topology_2x2_directional
    ├── resource_ledger: dss_v2_resource_ledger
    └── ledger_diagnostic: dss_v2_legacy_ledger_diagnostic_adapter
```

| Module | Source present | Referenced | Elaborated | Mapped | Evidence |
| --- | --- | --- | --- | --- | --- |
| top, analyzer, policy core, slot decode, feasibility, topology, ledger, diagnostic adapter | YES | YES | YES | YES | SYN-A source manifest and `hierarchy_area.rpt` |
| RS3 config table and RS3 topology | YES | only false `RESOURCE_POINT != 2` generate branch | NO | NO | fixed top parameter and absent mapped hierarchy |

Thus the manifest's RS3 definitions do not create RS3 mapped hardware in
SYN-A.  The active topology already fixes four physical resources:
`A_ROW`, `D_ROW`, `B_COL`, and `C_COL`.

## Hardware walkthrough and retained state

The controller has two states: `IDLE` and `SEARCH`.  On `start`, it resets
the attempt to A/rank 0.  It processes A → B → C → D.  For each SA it tries
the fixed semantic rank `RELEASE_ONLY`, `LOCAL`, `RELEASE_AND_BORROW`, then
`BORROW_ONLY`; `dss_v2_group_slot_decode` maps that rank to role-specific
ConfigID.  The shared analyzer returns the lowest valid PatternID for the
currently requested ConfigID.  The first analyzer-valid and ledger-legal
candidate commits; no earlier SA is revisited.  An exhausted rank fails at the
current SA, and D's accepted commit completes the group.

There is no candidate bank, candidate bitmap register, snapshot bank,
rollback state, multi-config snapshot, or group-wide candidate map in SYN-A.
`candidate_solution_valid_i`, `candidate_repairable_i`, and PatternID are live
analyzer responses.  `candidate_valid_bitmap` at the top is a 10-bit unused
debug wire, not retained candidate state.

| Register / state | Width × depth | Bits | Why it exists | Required for frozen semantics? | Derivable? |
| --- | ---: | ---: | --- | --- | --- |
| `state_q`, `sa_q`, `rank_q` | 1 + 2 + 2 | 5 | IDLE/SEARCH and A→D/rank progress | YES | NO |
| `done_q`, `group_repairable_q`, `failure_position_q` | 1 + 1 + 2 | 4 | terminal result | YES at existing interface | PARTIAL: result encoding can change only with interface approval |
| `sa_commit_valid_q` | 4 | 4 | records committed A–D results | YES | NO |
| selected action/config/pattern/donor | 8 + 12 + 24 + 8 | 52 | publishes each accepted result | YES at existing interface | NO while outputs remain registered |
| `borrow_flat_q`, `release_flat_q` | 4 + 4 | 8 | per-SA selected effect trace | PARTIAL | YES from accepted action/donor/resource relation if interface timing permits |
| ledger `released_q` | 4 | 4 | records which physical resources have been released | YES for generic ledger representation | YES in a final fixed design from committed per-SA release effects plus availability state |
| ledger `borrower_valid_q` | 4 | 4 | resource has been consumed by one borrower | YES for generic ledger representation | YES from a four-token availability state plus accepted selections |
| ledger `borrower_id_q` | 4 × 2 | 8 | identifies borrower for each resource | PARTIAL: required by current diagnostic/output contract | YES from retained selected donor/action/SA records in a fixed no-rollback implementation |
| ledger commit status | 1 + 1 | 2 | generic transaction diagnostics | NO for the current policy core; outputs unused | YES/REMOVABLE only after preserving or retiring that debug contract |

The listed policy state totals 73 bits at `PATTERN_ID_W=6`; the ledger owns
18 bits.  These are source-level logical bits, not a gate-equivalent claim.

## Specific RTL-pattern audit

| Structure | Source evidence | Inferred hardware | Classification | Area risk | Timing risk | Mapping evidence |
| --- | --- | --- | --- | --- | --- | --- |
| Dynamic array/packed indexing | ledger `released_q[release_resource_id_i]`, `borrower_valid_q[selected_donor_resource_id_i]`; variable borrower-ID write | 4-way read muxes plus 1-of-4 write enables/hold muxes; the ID write has a runtime base part-select | `REPRESENTATION_DRIVEN` | LOW–MEDIUM | HIGH | mapped ledger is 2,215.38 area; endpoint of worst path |
| Variable part-select | core lines 251–258 and ledger line 62 | four-way write-enable/hold muxes for 2-, 3-, 6-, and 2-bit per-SA fields; ledger 2-bit borrower-ID write | `REPRESENTATION_DRIVEN` for fixed four SA/resource mapping | LOW | MEDIUM | survives source closure; exact subcone area not reported |
| Runtime `integer` | feasibility `resource_index` and diagnostic adapter `r` | both are fixed `0..3` generate-equivalent combinational scans after synthesis, not a runtime stored 32-bit datapath or divider | `SOURCE_GENERICITY_ONLY` | LOW | LOW | no integer-width cell evidence |
| Arithmetic | `sa_q*{2,3,PATTERN_ID_W}` and resource-ID × `SA_ID_W` bases | small constant-scaled select/address expressions, not multiply/divide units proven in mapped netlist | `REPRESENTATION_DRIVEN` | LOW | MEDIUM | not decomposed below hierarchy |
| Resource availability vector and four-item scan | feasibility lines 41–63 | four parallel released-and-not-borrowed terms, invariant check, then a two-level primary/secondary donor priority | `CONTROL_MUX_OVERHEAD` | LOW | MEDIUM | feasibility maps to 595.43 area; topology child 242.83 |
| Runtime donor selection | fixed topology maps each SA to two candidate donors and feasibility selects primary then secondary | 2:1 priority mux/control | `ARCHITECTURE_INTRINSIC` for the frozen fallback donor order; generic signal encoding is representation-driven | LOW | MEDIUM | mapped; exact cells not separated |
| Generic resource ledger | transaction well-formed validation, released/borrower state, borrower ID, commit status | dynamic resource selects, transaction checks, state and diagnostic support | `MIXED`: policy legality is intrinsic; generic transaction/metadata representation is not proven minimal | MEDIUM | HIGH | ledger is 2.6% total area and is timing endpoint |
| Duplicated resource metadata | `resource_borrowed_o = borrower_valid_q`; both exported as 4 bits | no independent state for `resource_borrowed`; it is a wire alias | `POTENTIAL_SEMANTIC_REDUNDANCY` | LOW | LOW | source-proven alias; output-port removal needs interface authority |
| Candidate retention | no candidate bank in actual SYN-A path | none; live analyzer response only | `ARCHITECTURE_INTRINSIC` streaming choice | NONE | NONE | source and mapped hierarchy agree |
| Config priority | rank case `1,0,3,2`, four levels | fixed four-way priority sequence across cycles, not one long combinational priority chain | `ARCHITECTURE_INTRINSIC` | LOW | LOW–MEDIUM | `rank_q` controls longest path, but reports do not isolate priority delay |
| Pattern priority | inside shared analyzer | required lowest-valid PatternID selection | `ARCHITECTURE_INTRINSIC` | HIGH | HIGH | analyzer is 72,718.43 area / 86.7% of top and occupies most of critical path |
| Over-generic RS3 support | false generate branch only | none in RS2 elaboration | `SOURCE_GENERICITY_ONLY` | NONE | NONE | absent from hierarchy report |

No variable part-select or runtime resource index is a correctness failure.
They are the concrete hardware-representation review targets because this
topology has only four fixed resources and four fixed SAs.

## Source complexity versus mapped cost

Frozen DC reports 83,831.93 total cell area: analyzer 72,718.43 (86.7%),
policy core 11,113.50 (13.3%), and ledger 2,215.38 (2.6%).  The ledger is not
the principal area block.  It is, however, on the worst timing path.  This
separates the concerns: a compact block can still dominate setup timing.

The authoritative path is `sa_q_reg[1]` → slot decode → shared analyzer
(`repairable_o`) → policy acceptance gating →
`resource_ledger/transaction_valid_i` → OAI/inverter/mux logic →
`borrower_valid_q_reg[0]/D`, with 19.79 ns arrival against 19.80 ns required.
The report supports these stages, but does not support attributing an exact
delay or cell count to every RTL conditional.  The ledger tail is visible
from 18.78 to 19.79 ns; the analyzer dominates the preceding cone through
18.16 ns.  Therefore `HISTORICAL_SYN_A_CRITICAL_PATH_RESOURCE_LEDGER_DOMINATED`
is `PARTIAL`, not `YES`.

## Minimum plausible final EARLY reconstruction

Subject to an equivalence proof of the frozen A→D order, four rank order,
lowest PatternID rule, no rollback, first-failure termination, and current
cycle-visible outputs, the smallest plausible control structure is:

```text
canonical shared analyzer
  -> current Config valid + lowest PatternID
  -> constant per-SA/per-slot release/borrow consume mask
  -> 4-bit availability token legality
  -> first-valid rank selection
  -> commit selected Config/Pattern/donor
  -> update tokens and advance SA
```

`shared_line_free_q[3:0]` can represent the resource legality state if a
release sets its token and a borrow consumes it.  It is sufficient for future
legality only.  It is not, alone, bit-for-bit equivalent to SYN-A's public
`resource_released`, `resource_borrowed`, `borrower_id_flat`, and diagnostic
encoding.  Those outputs can plausibly be derived from committed per-SA
selected action/donor records, but that must be proven at the exact output
cycle before the ledger's state bits are removed.

| Function | Historical SYN-A | Minimum plausible final EARLY | Difference | Semantic boundary |
| --- | --- | --- | --- | --- |
| resource legality | 4 released + 4 borrower-valid plus dynamic ID indexing | four fixed availability tokens | removes duplicated state representation | safe only with token transition proof |
| borrower identity | 8-bit resource-indexed ledger state | derive from committed per-SA selections | removes indexed borrower-ID write | preserve all current outputs/timing |
| donor lookup | topology output plus primary/secondary availability priority | fixed per-SA/slot consume alternatives | may replace generic IDs with constants | preserve fallback donor priority |
| candidate handling | live analyzer output | live analyzer output | none | already streaming; no 80-bit store to remove |
| Config/Pattern order | `rank_q` and analyzer | same | none | architecture-intrinsic |
| failure/commit | first failure and one-edge ledger commit | same | none | architecture-intrinsic |

## Safe rewrite boundary

Potentially safe representation changes, contingent on exact sequential and
interface equivalence, are: generic ledger state to four tokens; dynamic
resource-ID selection to constant role/slot masks; resource-indexed borrower
metadata to derivation from retained selected results; and removal of the
unused internal ledger commit-status consumption.  No candidate-store removal
is applicable because SYN-A has none.

The following would be architectural changes and are out of scope: SA order,
rank/Config priority, PatternID priority, donor priority, analyzer contract,
rollback, parallel commit, topology, Config universe, or output-cycle
interface changes.

## Final verdict

```text
EARLY_HW_FRIENDLY_AUDIT: COMPLETE
VERILOG_SKILL_USED: YES
ASIC_RULEBOOK_USED: YES
ACTUAL_EARLY_TOP: recam_dss_canonical_rs2_streaming_early_top
ACTUAL_EARLY_CORE: recam_dss_canonical_streaming_early_core
GENERIC_RESOURCE_LEDGER_PRESENT: YES
GENERIC_RESOURCE_LEDGER_REQUIRED: NO for minimum fixed-resource representation;
                                  YES for the frozen current diagnostic/output contract
DYNAMIC_RESOURCE_INDEXING_PRESENT: YES
VARIABLE_PART_SELECT_PRESENT: YES
RUNTIME_INTEGER_DATAPATH_PRESENT: NO
WIDE_RUNTIME_MUX_PRESENT: NO (only bounded 4-way resource/SA selection)
DUPLICATED_RESOURCE_METADATA_PRESENT: YES (`resource_borrowed` aliases `borrower_valid`)
UNNECESSARY_CANDIDATE_RETENTION_PRESENT: NO
LEGACY_GENERICITY_SURVIVES_MAPPING: YES for RS2 ledger/feasibility;
                                     NO for RS3 false-generate branch
HISTORICAL_SYN_A_CRITICAL_PATH_RESOURCE_LEDGER_DOMINATED: PARTIAL
MOST_IMPORTANT_REPRESENTATION_OVERHEAD: dynamic resource-indexed ledger update/metadata
MOST_IMPORTANT_TIMING_OVERHEAD: analyzer-to-ledger acceptance cone, including ledger tail
MOST_IMPORTANT_AREA_OVERHEAD: shared analyzer (architecture-intrinsic)
MINIMUM_PLAUSIBLE_RESOURCE_STATE_BITS: 4 legality tokens, plus existing result
                                        registers or equivalent output derivation
MINIMUM_PLAUSIBLE_CANDIDATE_STATE_BITS: 0 retained bits; live valid/repairable/
                                        PatternID response only
FOUR_TOKEN_MODEL_HARDWARE_ADVANTAGE: removes generic indexed ledger-state and
                                     borrower-metadata update structures if interface-equivalent
STREAMING_EARLY_HARDWARE_ADVANTAGE: no 80-bit, 480-bit, or group-wide candidate store
SAFE_FINAL_REWRITE_READY: NO — exact token/output-cycle equivalence is not yet proven
```

No gate-equivalent or GE saving is claimed.  A future final-EARLY rewrite may
be authorized only after a separate equivalence contract fixes all observable
result/ledger outputs and their cycle timing.
