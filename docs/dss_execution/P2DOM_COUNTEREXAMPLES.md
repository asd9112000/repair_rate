# P2-DOM counterexamples

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

## Effect-pair-only collapse is unsound

This minimal candidate map proves that collapsing solely by
`(actual_release, actual_borrow)` is unsafe.  Unlisted slots are invalid.

| Depth | Candidate | actual_release | actual_borrow |
|---|---|---:|---:|
| A | B/P1 | 0 | 1 |
| B | L/P1 | 1 | 0 |
| B | RB/P1 | 1 | 0 |
| C | R/P1 | 1 | 0 |
| D | R/P1 | 1 | 0 |

A's borrow creates an obligation on B_COL.  At B, L/P1 is visited before
RB/P1.  L/P1 cannot fulfill the obligation because L is not explicit release;
RB/P1 can fulfill it.  A naive effect-pair cache records L/P1's `(1,0)` class
before legality and incorrectly suppresses RB/P1, turning a repairable map into
a failure.  The audit test requires this baseline-pass/effect-only-fail result.

The counterexample does not refute the safe key
`(explicit_release, actual_release, actual_borrow)`.
