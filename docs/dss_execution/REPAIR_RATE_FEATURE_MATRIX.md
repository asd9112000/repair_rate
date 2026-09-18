# Repair-Rate R0 Feature Matrix

> 文件狀態：Current
> 適用範圍：repair-rate program / implementation readiness
> 建立時間：2026-09-16T00:45:03+08:00
> 最後修改時間：2026-09-16T00:45:03+08:00
> 本文件權威主題：concise feature-by-scope and R1 gap matrix

This matrix accompanies `REPAIR_RATE_SWEEP_READINESS_AUDIT.md`. READY means
source support plus focused evidence. PARTIAL means source support exists but
the formal contract, focused validation, or scope support remains incomplete.

| Feature | DynamicSpareSharing (group) | HierarchicalRECAM (device) | Next action |
|---|---|---|---|
| four-SA physical ledger | READY | READY as Tier 1 | none |
| 2×2 topology | READY | READY | none |
| 1×4 line layout | READY (`neighbor`, row-only) | MISSING | defer device implementation to R4 |
| local/no sharing | READY | READY | none |
| directional m=1 | READY | READY | none |
| directional m=2 | READY | PARTIAL | focused device regression |
| pairwise row-only m=1 | READY (`edge`) | READY | none |
| pairwise row-only m=2 | READY | PARTIAL | focused device regression |
| same-corpus replay | READY | READY | add corpus signature to schema |
| fixed group total | READY | READY | none |
| fixed-total multinomial | MISSING | inherits generator gap | R1 before R2 |
| controlled fixed-total imbalance | MISSING | inherits generator gap | optional Dirichlet-multinomial |
| per-SA imbalance metrics | PARTIAL (raw A/B/C/D) | PARTIAL (total only) | versioned derived fields |
| C1 EARLY | READY | PARTIAL | paired device workflow |
| C2 GROUP-GREEDY | PARTIAL (`group_no_scratch_v2`) | PARTIAL | resolve C++/RTL slot priority |
| C3 GROUP-GLOBAL | PARTIAL (`group_compressed_legacy`) | PARTIAL via common core | freeze objective, identity, counters |
| V2 candidate history | four role slots/SA | common-core result only | expose ConfigID/PatternID trace |
| C3 candidate history | all valid RECAM candidates per capacity attempt | common-core result only | report/cap tuple count |
| persistent online CAM | not applicable by scope | READY, one pool | none |
| group/device result separation | READY | READY | preserve scope roots |
| CAM capacity accounting | PARTIAL | PARTIAL | distinguish envelope/generic/physical widths |
| CAM occupancy accounting | PARTIAL | PARTIAL | add versioned per-run and quantile fields |
| SRAM model | separate backend executable | canonical backend | retain separate semantics |
| formal output schema | PARTIAL | PARTIAL | add alongside legacy CSVs |

## Search-space guardrails

| Policy/envelope | Candidates per SA | Group tuples | Decision behavior |
|---|---:|---:|---|
| C2 V2 role store | 4 slots | 256 hypothetical | no enumeration; greedy A→B→C→D commit |
| generic C3, 2×2 directional 2,2,m=1 | up to 16 valid RECAM candidates | up to 65,536 | complete legality test then one final commit |
| historical directional analyzer | 7 ConfigIDs | 2,401 | historical ConfigID global search, not generic C3 semantics |

## R1 entry checklist

- Approve C3’s objective and whether partial repair tuples are legal.
- Decide whether C2 uses active RTL priority `1,0,3,2` or preserves the C++
  `0,1,2,3` order under a distinct policy identity.
- Add fixed-total multinomial allocation with deterministic tests.
- Add imbalance, physical-budget, tuple-count, and paired C1/C2/C3 fields to
  a versioned schema.
- Add a one-point manifest/runner around the group core; do not begin R2 until
  the paired-corpus and schema checks pass.
- Keep device 1×4, CAM RTL calibration, and device occupancy/schema completion
  in the R4/R5 queue.

## R1 policy-contract update (2026-09-16)

R0 remains historical audit evidence. R1 adds named policies without changing
the legacy aliases or their output identity.

| Feature | R0 status | R1 status | Evidence / next action |
|---|---|---|---|
| C1 EARLY | READY | READY | `findEarlyChoice`: A→B→C→D sequential commitment. |
| historical C2 V2 | PARTIAL | PRESERVED | `group_no_scratch_v2` retains C++ slot order `0,1,2,3`. |
| canonical C2 GROUP-GREEDY | PARTIAL | PARTIAL | `group_greedy_rtl_canonical` uses RTL rank `1,0,3,2`; resolve generic 2×2 versus RS3 target ConfigID-table conflict before publishing numeric IDs. |
| C3 historical compressed | PARTIAL | PRESERVED | `group_compressed_legacy` remains its historical identity. |
| canonical C3 GROUP-GLOBAL | PARTIAL | READY, group-core | `group_global` reuses exhaustive PatternID tuple search; success requires one legal full A/B/C/D tuple. |
| C3 objective/tie-break | PARTIAL | FROZEN | min borrowed, min used lines, A/B/C/D PatternID tuple, attempt tuple. |
| same-corpus replay | READY | READY group / PARTIAL device | const group input regression passes; add corpus_id sidecar for device comparison. |
| C2 priority characterization | MISSING | COMPLETE, non-formal | 128 vectors: success 0, Config 128, Pattern 128, ledger 128 mismatches; first seed 5000. |
| C2 vs C3 counterexample | MISSING | FOUND, synthetic | Directed candidate-state fixture: canonical greedy fails, global succeeds. Not a repair-rate result. |
| formal CSV policy schema | PARTIAL | PARTIAL | R1 defined raw/summary/debug split; implement versioned sidecar in R2. |

## R1A closure update

| Feature | Status | Evidence |
|---|---|---|
| 2×2 ConfigID contract | READY | Versioned `frozen_date_2x2_m1` and `rs3cs3_m1`; no global numeric-table assumption. |
| RS3 target lockstep | READY | 16 directed and 1,000 randomized RTL lockstep vectors, zero mismatches. |
| GROUP-GLOBAL oracle | READY | Independent PatternID Cartesian oracle: directed plus 128 randomized states, zero mismatches. |
| paired corpus/result schema | READY group | Additive `dss_paired_corpus_v1` CSV sidecars; same corpus ID verified across EARLY/C2/C3. |
| 1×4 two-pairwise policy | NOT_STARTED | R1B primary model: A⇄B and C⇄D only, row-only. |
| 1×4 single-hop comparator | NOT_STARTED | R1B comparator: A—B—C—D direct-neighbor rows only. |

## R1B 1×4 row-only update

| Policy | Group-level implementation | Verification | Device level | Same corpus | CAM accounting |
|---|---|---|---|---|---|
| Two-Pairwise EARLY v1 | READY | directed pair boundary and budget cases | NOT_STARTED | READY | PARTIAL |
| Two-Pairwise Pair-Global v1 | READY | pair oracle: 384 deterministic vectors, 0 mismatch | NOT_STARTED | READY | PARTIAL |
| Single-Hop EARLY v1 | READY | directed neighbor/no-transitive cases | NOT_STARTED | READY | PARTIAL |
| Single-Hop Global v1 | READY | PatternID global oracle: 128 deterministic vectors, 0 mismatch | NOT_STARTED | READY | PARTIAL |

The 1×4 primary topology is `A⇄B` plus `C⇄D`; the single-hop line is a
comparator only. Both preserve `4 × (RS + CS)` physical lines and use
`dss_paired_corpus_v1`; no 2×2 policy is reused as a topology semantic alias.

## R2 preflight update

| Feature | Status | Evidence / next action |
|---|---|---|
| fixed-total multinomial corpus | READY | `multinomial_uniform` preserves `F_GROUP` exactly and is replayed from a materialized fault file. |
| R2 paired raw/summary schema | READY | `scripts/group/r2_repair_rate_preflight.py` writes versioned raw, paired, imbalance, summary, manifest, and log artifacts. |
| 1×4 EARLY selector boundary | READY | Generic 2×2 compressed-dominance assertion is no longer applied to 1×4 EARLY policies. |
| 1×4 CAM fields | PARTIAL | Explicit `NA` / `NOT_PROVEN`; no topology-specific formula invented. |
| Single-Hop GLOBAL 1,000-group runtime | READY | R2F demand-equivalence reduction plus incremental pruning; 1,000-vector oracle mismatch 0. |
| formal group sweep | NOT_STARTED | R2 preflight is complete; await separate R3 authorization. |

## R3 formal-sweep update

| Feature | Status | Evidence / next action |
|---|---|---|
| R3 formal group sweep | PARTIAL / RUNNING | 100,000 groups per point; N2/F8 completed nine same-corpus policy replays. Remaining formal points must complete before any R3 conclusion. |
| GLOBAL ledger-order consistency | READY | GLOBAL DFS now verifies the same A→B→C→D sequential ledger contract as the corresponding EARLY policy; focused oracles pass. |

### Canonical role-slot table

| Role | RTL rank slots | ConfigID order, RS=CS=3,m=1 |
|---|---|---|
| A/D | `1,0,3,2` | `1,0,3,2` |
| B/C | `1,0,3,2` | `4,0,6,5` |

See `R1_POLICY_CONTRACT_RECONCILIATION.md` for evidence, the unresolved
generic 2×2 ConfigID-table ambiguity, output contract, and exact C3 search
semantics.
