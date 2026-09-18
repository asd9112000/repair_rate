# H4 — RS=CS=3, SHARE_M=1 Functional Verification

> Status: COMPLETE
>
> Target: 2×2 Directional CAM, `RS=3`, `CS=3`, `SHARE_M=1`
>
> Policies: V2-derived EARLY and GROUP-NoScratch
>
> Revision tested: `3304c86`

## Scope and reproduction

H4 verifies the isolated H3 target RTL.  It does not characterize synthesis,
modify production C++ simulator semantics, or authorize H5/S1/E0/E1.

```bash
bash scripts/simulation/run_dss_v2_rs3cs3m1_h4_functional.sh
bash scripts/simulation/run_recam_phase3hi_functional.sh
```

The machine-readable target result is
`results/dss_v2_rs3cs3m1/h4_functional/summary.txt`; the command transcript is
`results/dss_v2_rs3cs3m1/h4_functional/verification.log`.

## Independent policy golden

`tb/dss_v2/recam_dss_v2_rs3cs3m1_policy_equivalence_test.cpp` contains a
self-contained C++ policy golden.  It independently implements target role-slot
configuration identity, EARLY/GROUP rank, directional donor order, resource
ledger availability, atomic release/borrow commit, A→B→C→D traversal,
first-failure termination, and no rollback.  It does not call the DUT, consume
DUT selection/feasibility/donor outputs to construct an expected result, or
reuse RTL decision helpers.  The shared target-ID table is inert H1/H2
configuration data only.

For each vector, the bridge sends identical sixteen `(candidate-valid,
PatternID)` records to the selected RTL policy and to the C++ golden.  The
comparison covers repairability, failure position, A/B/C/D commit mask,
physical target ConfigID, PatternID, release/borrow action, valid-qualified
donor, and final legacy-compatible 12-bit four-resource ledger.  Any mismatch
prints seed, vector, policy, the complete 4×4 candidate map, and RTL/golden
observable tuples, giving a reproducible vector.

## Evidence matrix

| Evidence | Result | Verification evidence |
|---|---|---|
| Seven target physical configurations | PASS | Table test covers `3R3C, 2R3C, 3R4C, 2R4C, 3R2C, 4R3C, 4R2C`. |
| A/D semantic slots | PASS | `LOCAL, RELEASE_ONLY, BORROW_ONLY, RELEASE_AND_BORROW` map to target IDs/envelopes in table test. |
| B/C semantic slots | PASS | Same action slots map independently to `3R3C,3R2C,4R3C,4R2C`. |
| Candidate counts / 35-bit bitmap | PASS | Analyzer checks `20/10/35/15/10/35/15`; inactive lanes are zero by exact popcount. |
| PatternID boundaries | PASS | Invalid overflow gives PatternID 0; first valid candidate is 1; the 35th lane and PatternID width are exercised. |
| K=7 / 7×7 matrix | PASS | `3R4C/4R3C` 35-lane class exercised. |
| Pivot/address boundary | PASS | entries 0 and 6 and full seven-pivot occupancy are checked. |
| Hybrid boundary | PASS | entries 0 and 16 and full seventeen-entry occupancy are checked. |
| Must thresholds | PASS | For every configuration, physical `row > C_available` and `column > R_available` predecoded inputs reduce the feasible set; transpose path is included. |
| Symmetric class | PASS | `3R3C` checked without transpose. |
| Transpose pairs | PASS | `2R3C↔3R2C`, `3R4C↔4R3C`, and `2R4C↔4R2C` have identical canonical bitmap/PatternID results under neutral inputs. |
| Role/action identity | PASS | Test checks action semantics separately from target numeric ID. |
| GROUP candidate history | PASS | 112-bit store test writes/reads every SA×slot boundary with PatternIDs above 15. |
| EARLY directed | PASS | local, release-only, primary/alternate donor, release+borrow, conflict, and failures A/B/C/D. |
| GROUP directed | PASS | local, GROUP-vs-EARLY priority discrimination, and failures A/B/C/D. |
| Ledger invariants | PASS | Independent ledger comparison checks single legal donor allocation, released/free donor availability, atomic selected-only commits, failure ledger, no post-failure commit, and no rollback on every directed/random vector. |
| EARLY random | PASS | seed `20260910`, 1000 vectors, 0 mismatches. |
| GROUP random | PASS | seed `20260910`, 1000 vectors, 0 mismatches. |
| Paired outcomes | PASS | `BOTH_PASS=579`, `EARLY_ONLY=0`, `GROUP_ONLY=116`, `BOTH_FAIL=305`; corpus characterization only, not a dominance assertion. |
| Frozen 2,2,1 regression | PASS | `run_recam_phase3hi_functional.sh`: 15/15 directed; EARLY/GROUP 50 and 1000 vectors all zero mismatch. |
| Configuration-table isolation | PASS | Target table and analyzer live only under `rtl/dss_v2/rs3cs3m1/`; they are instantiated by target-named cores.  No runtime selector can substitute them for frozen 2,2,1 tables, so cross-point selection is structurally impossible. |
| H0S0-CL-004 | OPEN | Existing C++ `GroupCompressed` remains semantically distinct and is not used as this GROUP-NoScratch oracle. |
| Synthesis characterization | N/A | Explicitly forbidden in H4; reserved for H5. |
| Production simulator semantics | PASS | No `src/`, `inc/`, production simulator workflow, or golden experiment semantics changed. |

## Random result

```text
seed                 = 20260910
EARLY vectors        = 1000
EARLY mismatches     = 0
GROUP vectors        = 1000
GROUP mismatches     = 0
BOTH_PASS            = 579
EARLY_ONLY            = 0
GROUP_ONLY            = 116
BOTH_FAIL             = 305
```

The zero `EARLY_ONLY` result is specific to this deterministic corpus and is
not interpreted as an EARLY/GROUP policy invariant.

## H4 closure

All H4 functional gates pass.  The target retains the H2 architectural minimum
of 112 candidate-history bits; any additional group-core state is control,
ledger, or output diagnostic state rather than candidate-history payload.

```text
H4 = COMPLETE
READY_FOR_H5 = YES
NEXT_PHASE_AUTHORIZED = NONE
```
