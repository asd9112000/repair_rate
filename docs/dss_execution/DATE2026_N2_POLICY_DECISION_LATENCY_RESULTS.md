# DATE2026 N2 Final-Snapshot Policy-Decision Latency

`STATUS: COMPLETE`
`METRIC: final-snapshot policy-decision latency`
`SYNTHESIZABLE_RTL_CHANGED: NO`

## Scope and counting

This experiment begins with a caller-owned finalized analyzer summary. It
measures controller latency from the rising edge accepting `start_i && !busy_o`
to terminal `done_o`. The accepting edge is cycle zero, so an event on edge
`k` has latency `k - 0` cycles.

It does not measure BIST, raw-fault collection, online-CAM allocation,
post-BIST/last-fault timing, gate-level propagation, multi-group scheduling,
or device-level latency.

## Paired snapshot corpus

The non-synthesizable wrapper
`tb/n2_policy_decision_latency/tb_n2_policy_decision_latency_top.sv` drives
the same held-final snapshot to the frozen N2 EARLY and GROUP tops. The
monitor is `tb/n2_policy_decision_latency/tb_n2_policy_decision_latency.cpp`.

Seed: `20260922`; preflight/formal vectors: `1000 / 10000`.

The generator is a constrained extension of the golden-model-backed state
construction in `tb/recam/recam_shared_config_analyzer_test.cpp`:

- compact pivot and hybrid-record prefixes;
- unique active 9-bit rows and 13-bit physical columns;
- threshold masks restricted to active pivots and nested as
  `gt3 subset gt2 subset gt1`;
- valid in-dictionary hybrid row/column cross-edges;
- explicit conventional-overflow terminal snapshots.

The monitor validates every snapshot before either controller is driven.
`SEMANTIC_VALIDITY: PASS`.

## Directed and functional checks

The directed trace contains a successful all-local vector, an overflow failure,
and the maximum observed repairable EARLY path under this constrained corpus:

| Case | EARLY done | GROUP done |
| --- | ---: | ---: |
| Fast success | 4 cycles | 17 cycles |
| Overflow failure | 4 cycles | 17 cycles |
| Maximum observed repairable EARLY path | 12 cycles | 17 cycles |

The source permits up to four rank examinations per SA, but no unmeasured
16-cycle claim is made. The frozen corpus and directed constrained search
observed a maximum of 12 cycles.

For each vector the monitor checks termination, no timeout, legal EARLY commit
count (exactly four for success), atomic GROUP terminal commit mask, and
selected-output stability after terminal completion.

## Formal result

The primary comparable metric is `start_accept -> done_o`.

| Policy | Population | N | Min | Mean | Median | P95 | Max | Mean @20 ns |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| EARLY `L_done` | All | 10000 | 4 | 5.2006 | 4 | 11 | 12 | 104.012 ns |
| EARLY `L_done` | Repairable | 4619 | 4 | 5.6397 | 4 | 12 | 12 | 112.795 ns |
| EARLY `L_done` | Unrepairable | 5381 | 4 | 4.8236 | 4 | 8 | 11 | 96.473 ns |
| GROUP `L_done` | All | 10000 | 17 | 17.0000 | 17 | 17 | 17 | 340.000 ns |

EARLY's secondary first-commit metric, where a commit exists, has
`N=5895` and min/mean/median/p95/max of `1 / 1.8207 / 1 / 4 / 4` cycles.
For repairable vectors, `L_final_commit` has `N=4619` and
`4 / 5.6397 / 4 / 12 / 12` cycles. Failed vectors record
`L_final_commit=NA`.

`ALL_17_CYCLES: YES` for GROUP. Its final `sa_commit_valid_o=4'hf` is
coincident with terminal `done_o`; it is not an EARLY-style early commit.

Paired `GROUP - EARLY L_done` over all formal vectors has
min/mean/median/p95/max `5 / 11.7994 / 13 / 13 / 13` cycles
(`100 / 235.988 / 260 / 260 / 260 ns`). Both-repairable pairs have `N=4619`
and mean `11.3603` cycles. Outcome counts are `BOTH_REPAIRABLE=4619`,
`EARLY_ONLY=0`, `GROUP_ONLY=0`, and `BOTH_UNREPAIRABLE=5381`.

## RECAM and synthesis context

```text
RECAM_POLICY_DECISION_CYCLES: N/A_COMBINATIONAL
RECAM_CRITICAL_PATH: 20.00 ns
RECAM_WNS: 0.00 ns
COMMON_CLOCK: 20.0 ns
```

| Case | Decision organization | Decision cycles | Latency @20 ns | Critical path |
| --- | --- | --- | --- | --- |
| RECAM N2 2R2C | combinational | N/A | combinational | 20.00 ns |
| G2X2 RC EARLY | candidate-dependent | mean 5.2006 | mean 104.012 ns | 19.97 ns |
| G2X2 RC GROUP | fixed global decision | 17 | 340 ns | 19.90 ns |

No synthesis, SDF generation, or gate-level simulation was run.

## Artifacts and reproduction

```text
make test_n2_policy_decision_latency
```

Authoritative formal raw data, preflight data, summary, directed trace, and
result README are in `results/date2026/latency/n2_policy_decision/`.
Historical Phase-4I/E0L values are not merged because they used older V2
control timing. Paper wording is **final-snapshot policy-decision latency**.
