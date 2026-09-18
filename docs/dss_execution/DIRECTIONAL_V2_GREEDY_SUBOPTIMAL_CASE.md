# Directional V2 GROUP_GREEDY suboptimal case

**Dataset status:** QUICK_SWEEP / DEVELOPMENT / NON-FORMAL.  This is an
observed replay from the frozen 14k V2 validation corpus, not a formal-rate
result.

## Case and fault list

`N=2`, `Rs=Cs=2`, `share_m=1`, `F_GROUP=16`, `group_id=172`.

| SA | faults `(row,column)` |
| --- | --- |
| A | (365,213), (786,67), (1,213), (365,792), (366,215) |
| B | none |
| C | (185,604), (185,869), (253,497), (254,499), (253,299), (251,301), (183,604) |
| D | (420,805), (538,65), (420,388), (420,112) |

All candidates below come from the frozen `FrozenDate2x2M1` V2 slots.  V2
GROUP_GREEDY visits A, B, C, D and tests slots in RTL priority `1,0,3,2`.
It commits the first ledger-legal slot; it does not evaluate future-SA
feasibility before that commit and has no rollback.

## Full relevant candidate set

| SA | slot/config | action | legal PatternIDs and demand |
| --- | --- | --- | --- |
| A | 0 / 0 | LOCAL | 3,4: 2R+2C |
| A | 2 / 5 | BORROW_ONLY | 3,5: 2R+2C; 7,9: 1R+3C |
| A | 3 / 4 | RELEASE_AND_BORROW | 2,3: 1R+3C |
| B | 0 / 0, 1 / 1, 2 / 2, 3 / 3 | LOCAL / RELEASE_ONLY / BORROW_ONLY / RELEASE_AND_BORROW | all feasible; each has 0R+0C |
| C | 2 / 2 | BORROW_ONLY | 3,7: 3R+2C |
| D | 0 / 0 | LOCAL | 1: 2R+0C; 2,3: 1R+1C |
| D | 1 / 4 | RELEASE_ONLY | 1: 1R+1C |
| D | 2 / 5 | BORROW_ONLY | 1: 2R+2C; 2,3: 1R+2C; 4,7: 1R+3C |
| D | 3 / 4 | RELEASE_AND_BORROW | 1,2: 1R+3C |

## Sequential GROUP_GREEDY trace

The initial ledger has sixteen unowned physical lines (eight rows and eight
columns).  Owner strings in the retained raw trace encode the same state.

| SA | selected slot / Config / Pattern | demand | action | ledger effect |
| --- | --- | --- | --- | --- |
| A | 0 / 0 / 3 | 2R+2C | LOCAL | legal; reserves A's 2R+2C |
| B | 1 / 1 / 1 | 0R+0C | RELEASE_ONLY | legal; no demand |
| C | none | only slot 2 / Config 2 / Pattern 3 is candidate-feasible: 3R+2C | BORROW_ONLY | rejected by the ledger after A's reservation |

`FIRST_IRREVERSIBLE_GREEDY_COMMIT` is A's locally legal LOCAL choice
`slot=0, ConfigID=0, PatternID=3, 2R+2C`.  It is legal for the current prefix,
but consumes the resource arrangement C later needs.  The
`DOWNSTREAM_FAILURE` is C: its only V2 candidate is locally valid yet has no
legal allocation under that committed prefix.  D is never selected.

## V2 GROUP_GLOBAL witness

The exact V2 oracle retains every slot/PatternID candidate, then performs a
joint A→B→C→D tuple search through the same `PhysicalResourceLedger`.

| SA | GROUP_GREEDY choice | V2 GROUP_GLOBAL choice | consequence |
| --- | --- | --- | --- |
| A | slot 0 / Config 0 / P3, 2R+2C | slot 2 / Config 5 / P7, 1R+3C | permits B→A column transfer and leaves an A row for C |
| B | slot 1 / Config 1 / P1, 0R+0C | slot 0 / Config 0 / P1, 0R+0C | supplies the column transferred to A |
| C | no ledger-legal choice | slot 2 / Config 2 / P3, 3R+2C | receives one A row and repairs |
| D | not reached | slot 0 / Config 0 / P1, 2R+0C | repairs locally |

The final GLOBAL ledger is legal: `used=6R+5C`, `unused=2R+3C`, with
transfers `B→A` (column) and `A→C` (row).  Thus the case demonstrates the
cost of sequential first-legal commitment under one candidate/resource
contract; it does **not** compare V2 GLOBAL with generic GLOBAL.

The full raw replay trace is retained at
`tmp/directional_v2_global_validation/quick14k_20260918/traces/n2_f16_group_172_trace.txt`.
