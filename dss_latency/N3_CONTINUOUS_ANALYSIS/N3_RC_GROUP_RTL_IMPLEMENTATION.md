# N3 RC-GROUP CA-LIVE RTL implementation

## Scope

The isolated implementation is in
`rtl/G2X2_N3_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/`.  Its frozen N2
template source hashes are:

```
745cf6a87a94ba82a658d1252ecdf2f69f6f007de5740803afd9719c5779bb90  core
d2ddfeda78f9c3eaf6054a1c38ff2e50220bf5669f18928535bb4fbf6e6f29cf  top
```

The implementation preserves the N2 CA-LIVE state machine, update-wins
priority, four canonical evaluations per active SA, 81-path selector ordering,
registered GROUP result, and pivot capture point.  It scales only the frozen
N3 capacity parameters.

| Contract item | Implemented value |
| --- | ---: |
| physical address domain | row 9, physical column 13, Hybrid line 13 |
| analyzer semantic payload | 524 bits |
| MAX_K / pivot entries | 7 |
| Hybrid entries | 17 |
| PatternID | 6 bits |
| candidate store | 16 x `{valid, PatternID[5:0]}` = 112 bits |
| private pivot capture | 4 x 7 x (9 + 13) = 616 bits |
| full reconstruction dependency | 616 + commit 4 + ConfigID 12 + PatternID 24 = 656 bits |

The analyzer is `recam_n3_rc_fixed_mask_analyzer`.  It has one instance in the
top and uses the 80 static masks generated from the preserved fixed-mask CSV.
There is no runtime pattern enumerator and no duplicated 524-bit StateBank.

```
RTL_CREATED: YES
ARCHITECTURE_CONTROL: UNCHANGED_FROM_N2
RUNTIME_NTH_PATTERN: ABSENT
DUPLICATED_ANALYZER_STATEBANK: ABSENT
ANALYZER_INSTANCE_COUNT: 1
```
