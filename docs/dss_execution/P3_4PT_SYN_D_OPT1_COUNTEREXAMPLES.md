# SYN-D OPT1 rejected keys and counterexamples

## The SYN-B key is not a SYN-D key

```text
SYN_D_CAN_USE_SYN_B_OPT1_KEY: NO
SYN_B_KEY: (explicit_release, actual_release, actual_borrow)
SYN_D_REASON: candidate inputs contain row/column demand; release, borrow,
              donor, and owner effects are derived from the prefix ledger.
```

There is no candidate-local `actual_borrow` with the SYN-B meaning in SYN-D.
The same SYN-D demand may borrow different physical owners in different valid
prefix states. For example, let B require three rows after A:

| Prefix before B | B demand | First available legal shared line | Derived B donor |
|---|---:|---|---|
| A used two local rows, including A's sharing row | `(3,0)` | C's sharing row | C |
| A used one local row; A's sharing row is still unassigned | `(3,0)` | A's sharing row | A |

Both rows are legal A→B→C prefix executions. A boolean “borrow” describes
neither donor identity nor the successor owner state. Treating it as a
candidate-local 2×2 effect would therefore erase information that later
search depths need. The same physical-line state also enforces no forwarding:
a row borrowed by B is assigned to B and cannot be lent again by B to C.

The 1×4 RTL explicitly permits only A-B, B-C, and C-D in
`is_adjacent_owner`; D-A is illegal. Any arithmetic or effect abstraction
that loses this boundary would reintroduce the previously fixed wraparound
class of error.

## Rejected or deliberately unclaimed relations

| Proposed relation | Result | Concrete reason / counterexample |
|---|---|---|
| SYN-B effect key | Not applicable | The preceding B example has the same demand and a borrow in both cases but different donors and successor physical owners. |
| same derived borrow count | Unsafe as a pre-DFS key | Borrow count is only known after applying the candidate to the prefix ledger; it does not name the donor or preserve future line availability. |
| same donor identity | Not a candidate-only relation | Donor follows the prefix allocation scan. Computing it requires the physical owner state the collapse is supposed to preserve. |
| same physical-owner transition | Not a cheaper pre-DFS key | It is the complete successor-ledger calculation, including no-forwarding; using it as a key would perform the transition before candidate selection. |
| PatternID omitted without representative rule | Unsafe | Equal-demand candidates with PatternID 1 and 2 have equal ledger effects, but the frozen objective must select PatternID 1. |
| attempt index omitted without representative rule | Unsafe | Equal-demand candidates with equal PatternID and attempts 0 and 1 must select attempt 0. |
| unequal `(usedRows, usedColumns)` demands | Not collapsed | They are distinct frozen C++ demands and distinct selected-tuple records. A changed row demand can change local allocation, borrow count, and future availability. |

The audit does not claim that every more aggressive relation is impossible in
every configuration; it records that none beyond exact equal demand has been
proved for the frozen RS2/CS2/m=1 contract. Such work is outside OPT1 and
must not be silently introduced as this phase's fairness optimization.

## Directed checks

The SYN-D core test includes these required discriminators:

```text
EARLY_GLOBAL_DISTINCTION_TEST: PASS
D_TO_A_BOUNDARY_REGRESSION: PASS
NO_FORWARDING_REGRESSION: PASS
PATTERNID_EQUAL_DEMAND_TIE: PASS
ATTEMPT_INDEX_EQUAL_DEMAND_TIE: PASS
OWNER_STATE_DONOR_REGRESSION: PASS
```

The final three checks show why PatternID and attempt index may be absent from
the demand key only when the canonical lower representative is retained, and
why donor/owner fields must remain transition state rather than collapsed
candidate metadata.
