# R3 Common-Load Matrix V2 — User Sweep Handoff

## Frozen formal matrix

R3 Common-Load Matrix V2 is the primary group-level formal matrix:

| RS = CS | F_GROUP |
|---:|---|
| 2 | 8, 12, 16, 20, 24, 28, 32 |
| 3 | 8, 12, 16, 20, 24, 28, 32 |

The formal contract is 14 corpus points, nine canonical policies per point,
and 100,000 groups per policy.  A complete V2 sweep therefore contains 126
completed `paired_policy_results_v1.csv` sidecars and 12,600,000
policy-group evaluations.

The frozen seed for a point is `20260922 + RS * 1000 + F_GROUP`.  Each point
uses one fixed-total `multinomial_uniform` corpus and replays it through every
policy.  This remains a group-level experiment; it is not device-level data.

## Existing artifact audit

This is a read-only snapshot taken while a prior N3/F12 job was active.  A
`COMPLETE_VALID` sidecar had all 100,000 contiguous group IDs, the required
CSV schema, one nonempty corpus ID, binary repair results, and the expected
physical budget (`8 * RS`).  The corpus files were also checked for the frozen
seed, 100,000 groups, and exact `F_GROUP` faults.  Completed policies replayed
the source corpus ID.

| Point | COMPLETE_VALID | PARTIAL | INVALID | MISSING | Reuse status |
|---|---:|---:|---:|---:|---|
| N2/F8 | 9 | 0 | 0 | 0 | complete |
| N2/F12 | 9 | 0 | 0 | 0 | complete |
| N2/F16 | 9 | 0 | 0 | 0 | complete |
| N2/F20 | 9 | 0 | 0 | 0 | complete |
| N2/F24 | 9 | 0 | 0 | 0 | complete |
| N2/F28 | 1 | 0 | 0 | 8 | reuse LOCAL only |
| N2/F32 | 0 | 0 | 0 | 9 | missing |
| N3/F8 | 0 | 0 | 0 | 9 | missing |
| N3/F12 | 6 | 0 | 0 | 3 | reuse completed policies |
| N3/F16 | 0 | 0 | 0 | 9 | missing |
| N3/F20 | 0 | 0 | 0 | 9 | missing |
| N3/F24 | 0 | 0 | 0 | 9 | missing |
| N3/F28 | 0 | 0 | 0 | 9 | missing |
| N3/F32 | 0 | 0 | 0 | 9 | missing |

At that snapshot, 52 of 126 policy sidecars were reusable and 74 were still
needed.  The current `DynamicSpareSharing` executable SHA-256 was
`c5c2b252bc1cfce10be0510809c32fe92fe82b7ef37612ca83831baa9f809420`, matching
the recorded R3 manifest executable hash.

### N2/F24, N2/F28, and N2/F32

- **N2/F24: FORMAL_REUSABLE.** All nine sidecars are complete and replay the
  fixed-total corpus `dss_paired_corpus_v1-f2731e605f96dff3`; its seed is
  20262946, the frozen V2 derivation.
- **N2/F28: PARTIAL_REUSABLE.** The LOCAL sidecar is complete and replays the
  fixed-total corpus `dss_paired_corpus_v1-b3c007d43dd096f0`; its seed is
  20262950.  The other eight policies are missing.
- **N2/F32: MISSING.** No policy sidecar exists.

The previous active process was last confirmed to be running a valid V2
N3/F12 policy job.  It may be allowed to finish, but do not run a second
command against the same result root concurrently.

## Manual commands

Run commands only after any existing process using the same output root has
exited.  `--resume` validates complete sidecars and runs only missing or
incomplete policy units.  It does not overwrite validated results.

Required N2 command:

```bash
./scripts/run_date2026_repair_sweep.sh \
  --scope group \
  --rs 2 --cs 2 --share-m 1 \
  --f-group-list 8,12,16,20,24,28,32 \
  --groups 100000 --seed 20260922 \
  --policies canonical --topologies canonical \
  --formal --resume \
  --output-root results/date2026/repair_rate/r3_formal_group
```

Required N3 command:

```bash
./scripts/run_date2026_repair_sweep.sh \
  --scope group \
  --rs 3 --cs 3 --share-m 1 \
  --f-group-list 8,12,16,20,24,28,32 \
  --groups 100000 --seed 20260922 \
  --policies canonical --topologies canonical \
  --formal --resume \
  --output-root results/date2026/repair_rate/r3_formal_group
```

The runner can express the entire V2 matrix in one invocation because V2 uses
the same F_GROUP list for N2 and N3.  After no other process uses the root,
this is operationally safer than separate N2/N3 invocations:

```bash
./scripts/run_date2026_repair_sweep.sh \
  --scope group --share-m 1 \
  --groups 100000 --seed 20260922 \
  --policies canonical --topologies canonical \
  --formal --resume \
  --output-root results/date2026/repair_rate/r3_formal_group
```

Do not run the N2 and N3 commands concurrently.  Separate selected-point
invocations regenerate aggregate files for their selected subset; after both
are complete, use the full-matrix command above once more to reconcile all 14
points into the same aggregate.  With 126 complete sidecars, that final pass
does not launch a policy simulation.

## Monitoring commands

Running process:

```bash
ps -ef | grep -E 'run_date2026|r3_formal_group|DynamicSpareSharing' | grep -v grep
```

Completed-sidecar count by V2 point:

```bash
for point in n2_f8 n2_f12 n2_f16 n2_f20 n2_f24 n2_f28 n2_f32 \
             n3_f8 n3_f12 n3_f16 n3_f20 n3_f24 n3_f28 n3_f32; do
  printf '%s: ' "$point"
  find "results/date2026/repair_rate/r3_formal_group/raw/$point" \
    -name paired_policy_results_v1.csv -type f 2>/dev/null | wc -l
done
```

Tail the next missing N3/F12 policy log from the snapshot (the command retries
until the file exists):

```bash
tail -F results/date2026/repair_rate/r3_formal_group/logs/n3_f12_two_pairwise_m1_pair_global.log
```

The backend creates `manifest/COMPLETE.json` only after the selected runner
point set completes.  It is not proof that both N2 and N3 V2 sets have been
aggregated unless the final full-matrix reconciliation command was used.

## Post-sweep validation and aggregation

There is no separate finalized V2 aggregate command.  The existing backend
performs its validation and current aggregation during the full-matrix resume
command shown above.  Run it only after the sidecar-count command reports 9
for every one of the 14 points.

The current backend validates fixed totals, same-corpus replay, contiguous
group IDs, expected result-row counts, physical spare budget, and applicable
same-universe GLOBAL dominance.  It writes:

- `aggregate/r3_repair_rate_summary.csv` with Wilson CI fields and search
  mean/p95/p99/max fields;
- `aggregate/r3_paired_outcomes.csv`;
- `aggregate/r3_fault_imbalance.csv`; and
- the corresponding plots under `plots/`.

The final V2 authoritative aggregation backend is still pending: it does not
yet provide every requested final statistic as a standalone artifact (notably
median search complexity and the final V2 report/table set).  Do not label the
current aggregate as the final paper aggregate until that backend is frozen.

## Safety warnings

- Never remove or overwrite raw sidecars to force a rerun.  Resume rejects
  incomplete sidecars and reuses complete ones.
- Do not mix this group-level corpus with device-level results.
- Do not alter generator, seed, candidate, ledger, GLOBAL DFS, replay, or CSV
  semantics during the sweep.
- Do not use arbitrary GLOBAL cutoffs or heuristic fallbacks.
- No RTL, synthesis, or device simulation is part of this handoff.
