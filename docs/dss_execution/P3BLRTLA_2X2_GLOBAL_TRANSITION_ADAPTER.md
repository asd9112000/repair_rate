# P3-BL-RTL-A — transition adapter

The adapter is combinational and has no ledger state. It derives actual effects from the decoded candidate row/column demand, not action labels:

| SA | Release resource | Donor resource | Actual release | Actual borrow |
|---|---|---|---|---|
| A | A_ROW | B_COL | `rows < RS` | `cols > CS` |
| B | B_COL | D_ROW | `cols < CS` | `rows > RS` |
| C | C_COL | A_ROW | `cols < CS` | `rows > RS` |
| D | D_ROW | C_COL | `rows < RS` | `cols > CS` |

Nominal explicit-release identity remains in the unchanged search core: storage action R or RB implies `explicit_release=1`. This distinction preserves the frozen future-donor obligation semantics and OPT1 key `(explicit_release, actual_release, actual_borrow)`.

```text
TRANSITION_ADAPTER_IMPLEMENTED: YES
TRANSITION_ADAPTER_MISMATCHES: 0
```
