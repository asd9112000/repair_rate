# Phase 3H/3I Functional Verification

Reproduction command:

```bash
bash scripts/simulation/run_recam_phase3fg_functional.sh
bash scripts/simulation/run_recam_phase3hi_functional.sh
```

## Phase 3G correction revalidation

`PHASE3G_RESOURCE_FALLBACK_REGRESSION PASS`,
`PHASE3HI_GROUP_REGRESSION PASS`, and the downstream allocator RTL-vs-golden
regression all passed after correcting B/C GROUP ranking to frozen
`2 -> 0 -> 3 -> 1`. This corrects an implementation defect only; no new policy
was introduced.

## Independent golden model

`tb/recam/recam_dss_group_allocator_random_test.cpp` independently implements
map ranking, role legality, physical feasibility, frozen donor priority,
A->B->C->D traversal, release/borrow commit, first-failure termination, and no
rollback. It does not call Phase 3G RTL, consume RTL helper results, or derive
expectations from DUT ConfigID, donor, feasibility, or ledger outputs.

With seed `20260910`, RTL and golden compared ConfigID A/B/C/D, donor A/B/C/D,
final 12-bit ledger, `group_repairable`, and failure position inferred from
commit-valid state. EARLY ran 50 bring-up plus 1000 regression vectors with 0
mismatches. GROUP ran 50 bring-up plus 1000 regression vectors with 0
mismatches. Invariant violations: 0.

## Directed matrix

| Case | Policy | Expected / actual | Result |
|---|---|---|---|
| 1 all-local success | EARLY | cfg `0/0/0/0`, ledger `0x000` | PASS |
| 2 release helps later borrower | GROUP | A cfg4, B cfg2, ledger `0x011` | PASS |
| 3 borrow+release helps later borrower | GROUP | A4, B3, D5, ledger `0x215` | PASS |
| 4 donor conflict | GROUP | B consumes A_ROW; C fails, ledger `0x011` | PASS |
| 5 alternate donor fallback | GROUP | D cfg5 falls back to B_COL, ledger `0x204` | PASS |
| 6 config fallback after borrow failure | EARLY | A falls back to cfg0, ledger `0x000` | PASS |
| 7 release->borrow transition | GROUP | A release observed by B borrow, `0x011` | PASS |
| 8 no double allocation | GROUP | second A_ROW borrower rejected | PASS |
| 9 first-SA failure | EARLY | A fails, `0x000` | PASS |
| 10 middle-SA failure | EARLY | B fails, prior ledger retained | PASS |
| 11 last-SA failure | EARLY | D fails, prior ledger retained | PASS |
| 12 restart/reset | EARLY | reset starts a fresh zero ledger | PASS |
| 13 busy-start negative | EARLY | second start while busy ignored | PASS |
| 14 EARLY vs GROUP fairness | both | EARLY cfg0; GROUP cfg4 | PASS |
| 15 EARLY/GROUP final ledger differs | both | EARLY `0x000`; GROUP `0x001` | PASS |

The test executable prints each matrix row with policy, expected behavior,
actual ConfigIDs, donors, repairability, ledger, and PASS/FAIL. Matrix result:
`15/15 PASS`.

## Counterexample statistics

The same 1000-vector seed corpus produced: EARLY fail / GROUP pass = 2; EARLY
pass / GROUP fail = 1; both pass with a different final ledger = 20. A complete
representative EARLY-fail/GROUP-pass trace, including maps, per-SA decisions,
ledger evolution, failure position, and final results, is saved in
`tb/recam/phase3hi_counterexample_early_fail_group_pass.txt`.

## Observability correction

`ledger_released_borrower_o` now continuously reflects registered state after
the final atomic commit. This is an output-observability fix, not an allocation
semantic change.
