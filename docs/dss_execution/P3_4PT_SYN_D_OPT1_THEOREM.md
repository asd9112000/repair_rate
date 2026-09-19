# SYN-D OPT1 candidate-equivalence theorem

## Frozen theorem

For one 1×4 SA depth, let a valid candidate be

```text
c = (PatternID, attemptIndex, usedRows, usedColumns, mappingReference)
```

and define its demand as `d(c) = (usedRows, usedColumns)`. Let `r(d)` be the
valid candidate with demand `d` minimizing `(PatternID, attemptIndex)` in the
frozen C++ and RTL tuple order. Replacing the candidate set at every depth by
`{ r(d) | d occurs at that depth }` preserves:

```text
repairability
borrowed-row objective
used-row objective
lexicographically selected PatternID tuple
lexicographically selected attempt tuple
selected donor identities
final physical row assignments
atomic commit result
```

This is COMMON_OPT1: a pre-DFS candidate-equivalence collapse. It is not
OPT2 dominance pruning and does not cache or prune DFS states.

## Source anchors

- `src/DynamicRepairSimulator.cpp`, `globalPlansForSubarray`, stores the
  lowest `(solutionId, attemptVectorIndex)` plan in a map keyed by
  `(usedRows, usedColumns)`.

- `src/DynamicRepairSimulator.cpp`, `compressedChoiceIsBetter`, applies the
  frozen GLOBAL objective: borrow count, used rows, PatternID tuple, then
  attempt tuple.

- `src/PhysicalResourceLedger.cpp`, `allocateSequential`, derives owners and
  transfers from the prefix ledger and the exact demand.

- `rtl/dss_1x4/policy/recam_dss_line1x4_rs2_cs2_m1_normalized_global_core.v`
  applies the private ledger transition and `pattern_tuple_less` /
  `attempt_tuple_less` objective.

- `rtl/dss_1x4/policy/recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_core.v`
  is the separate static OPT1 elaboration.

## Proof

Take two candidates `x` and `y` with `d(x) = d(y)` and a common incoming
private ledger state `S` at their depth.

1. The RTL initializes its transition from `S`, consumes the same local rows,
   and scans the same explicitly legal sharing rows for equal demand. It
   therefore has the same legality result, borrowed-row delta, used-row delta,
   donor list, and successor ledger `T(S, d)` for `x` and `y`.

2. The ledger records physical owner and current borrower on each allocated
   line. Because `T` is identical, every later depth observes exactly the same
   owner, availability, single-hop adjacency, and no-forwarding state.

3. The only residual difference is reconstruction metadata. By definition
   `r(d)` has lower PatternID, or equal PatternID and lower attempt index. At
   the first depth where `x` or `y` differs from `r(d)`, the frozen tuple
   comparator selects `r(d)` before the discarded representative; no later
   depth can reverse that lexicographic decision.

4. The C++ source performs the same replacement before searching. Thus the
   retained metadata, including the C++ mapping reference associated with the
   selected plan, is the source-defined representative rather than a new
   RTL-only choice.

Applying this replacement independently at each depth is valid by induction
over A→B→C→D: the base ledger is the same, and each retained equal-demand
transition produces the same next ledger. At D, the terminal objective and the
atomic commit inputs are therefore unchanged.

## Candidate-field audit

| Field | Transition / future legality | Objective / reconstruction | OPT1 treatment |
|---|---|---|---|
| `usedRows` | Determines local and borrowed physical-row allocation, owner state, and future availability | Contributes to used-row objective | Required in key |
| `usedColumns` | Part of the frozen C++ demand and selected candidate record | Reconstructed selected tuple | Required in key |
| PatternID | No ledger transition effect | Lexicographic tuple tie-break and selected tuple | Not a key bit; lowest retained representative is mandatory |
| attempt index | No ledger transition effect | Final tie-break and selected tuple | Not a key bit; lowest retained representative is mandatory |
| borrow count | Derived from prefix ledger plus allocation | Primary objective | Not candidate input; must remain in DFS state |
| donor identity | Derived by physical-line scan from prefix ledger | Selected donor tuple | Not candidate input or collapse key; must remain exact transition output |
| physical row assignment | Successor ledger and future no-forwarding legality | Atomic commit input | Must remain exact state |
| owner/origin and lendability | Determines legal adjacent, unassigned shared lines | Final-owner result | Must remain exact state |
| mapping reference | Does not drive RTL physical transition | C++ selected-plan reconstruction/reporting | Preserved by retaining C++'s canonical representative |

`usedRows` and `usedColumns` are deliberately not replaced with a derived
borrow or donor field. Those fields cannot be known from a candidate alone:
they depend on the prefix physical ledger.

## Implementation boundary

The base `..._global_core` has a compile-time
`ENABLE_OPT1_CLASS_COLLAPSE` parameter. The OPT0 top fixes it to zero; the
separate OPT1 core and top wrappers fix it to one. No RTL signal chooses
between them at runtime. The producer, private ledger, atomic commit, and
external interface are identical.

The proof corpus is not the sole basis of the theorem; it validates the
source-and-transition proof with 1,000 shared C++ oracle cases (seed
`20260921`) and directed PatternID, attempt, D→A boundary, no-forwarding,
owner-state, and EARLY/GLOBAL distinction cases.
