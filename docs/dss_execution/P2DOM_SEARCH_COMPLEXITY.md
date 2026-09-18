# P2-DOM search complexity

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

The exhaustive upper bound assumes every one of the 40 slots per SA is valid
and all but the last completion path require backtracking:

```text
40 + 40^2 + 40^3 + 40^4 = 2,625,640 candidate visits
```

There are at most eight safe transition classes at one depth:
`explicit_release x actual_release x actual_borrow`.  If candidate-map intake
provides the first canonical representative of each present class, the
equivalent-effect upper bound is:

```text
8 + 8^2 + 8^3 + 8^4 = 4,680 visits
```

Dominance pruning alone has no tighter architecture-independent bound proven
than exhaustive DFS; it is data-dependent.  Combining it with the eight-class
frontier cannot exceed 4,680 visits and can lower typical work further.

The non-production fixed-seed (20260918), 1,000-map audit counts valid
candidate evaluations, not RTL cycles or invalid-slot cursor advances:

| Search | Worst proven bound | Mean | Median | P95 | P99 |
|---|---:|---:|---:|---:|---:|
| current exhaustive | 2,625,640 | 19 | 7 | 72 | 184 |
| safe class collapse | 4,680 | 12 | 6 | 42 | 90 |
| failed-state dominance | <= 2,625,640 | 10 | 7 | 31 | 46 |
| combined | <= 4,680 | 9 | 6 | 25 | 37 |

The dominance-only model executed 1,154 failed-subtree prunes across these
vectors and preserved all compared results.

## History compression boundary

```text
480_BITS_INFORMATION_THEORETICALLY_REQUIRED: NO
MINIMUM_SEARCH_RELEVANT_SUMMARY:
  per SA, up to 8 classes each with presence plus the first canonical 6-bit
  action/PatternID identity; 4 * 8 * (1 + 6) = 224 bits maximum.
```

This is a search/reconstruction summary, not a claim that the present
collector may emit it.  It preserves only the semantic information consumed
after candidate-map capture; it does not preserve every arbitrary raw map bit.
No compressed storage or RTL was implemented.
