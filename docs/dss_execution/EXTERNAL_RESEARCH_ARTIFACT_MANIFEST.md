# External Research Artifact Manifest

> Scope: immutable research artifacts intentionally retained outside ordinary
> Git. This manifest tracks their identity; it does not authorize regeneration,
> rewriting, moving, or deletion of any listed artifact.
>
> Manifest revision: 2026-09-18
> Source Git revision recorded by the artifact generators: `3304c86acd7e8114033e684acd721bf6daace57c`

## Integrity method

For a single file, `SHA-256` is the file-content digest. For a directory, the
digest is a tree digest: calculate SHA-256 for every regular file, sort those
`sha256sum` records by relative path, then calculate SHA-256 over the sorted
record stream. This makes the exact directory content and filenames part of
the recorded identity.

## Immutable external artifacts

| Artifact | Current location | Size (bytes) | SHA-256 | Generation command / configuration | Research purpose and dependent result |
|---|---|---:|---|---|---|
| Frozen normalized 1k paired corpus | `01_my_note/repair_rate_matrix_v2_normalized_1k/corpus/` | 20,275,232 | `f6c259a87ce229f5fd83c5cbfbacf01db78cde12a1aef10e0def2ab0b21aadf1` | `./scripts/run_date2026_repair_sweep.sh --scope group --groups 1000 --seed 20260922 --policies canonical --topologies canonical --preflight --output-root tmp/repair_rate_matrix_v2_normalized_1k`; per-point seed, corpus ID, and replay key are frozen in `manifest/r3_manifest.json`. | Frozen quick-sweep input for normalized group-policy closure and canonical witnesses; summarized by `R3_NORMALIZED_1K_METADATA_AND_WITNESS_CLOSURE.md`. |
| Frozen normalized 1k policy metadata | `01_my_note/repair_rate_matrix_v2_normalized_1k/aggregate/r3_normalized_policy_metadata.csv` | 72,661,550 | `253ad1e473a14ed2681e11ff11f808db909ce75b53beeb7526e2d38886293423` | Same frozen 1k preflight command and manifest above; source revision recorded there is `3304c86`. | Policy labels, candidate contracts, and metadata supporting normalized 1k tables/figures and witness interpretation. |
| Formal 100k paired corpus | `results/date2026/repair_rate/r3_formal_group/corpus/` | 1,703,120,387 | `3dab8d6ab02994cd24cab9a5e8c861634d804e91c447c2a6a06ba8b6002fa32b` | `./scripts/run_date2026_repair_sweep.sh --scope group --share-m 1 --groups 100000 --seed 20260922 --policies canonical --topologies canonical --formal --resume --output-root results/date2026/repair_rate/r3_formal_group`; per-point corpus SHA-256 values are in `manifest/corpus_index.csv`. | Exact formal group-level fault corpus for the R3 repair-rate result; summarized by `R3_FORMAL_GROUP_REPAIR_RATE_RESULTS.md`. |
| Formal 100k raw policy outputs | `results/date2026/repair_rate/r3_formal_group/raw/` | 13,109,975,840 | `eb2b8fb58d8f77eb19abf1de1f519741150114db8cf91cb955c71fa79cb5bc3f` | Same formal R3 command; configuration, policy matrix, binary SHA-256, and source revision are in `manifest/r3_manifest.json`. | Immutable per-policy/group evidence behind formal repair-rate, paired-outcome, and imbalance aggregates. |
| DATE group-latency raw data | `results/date2026/t1_group_latency/raw/` | 195,157,837 | `31c4d85844fff1103cca797ae3d0d0682280f582c8d06a2f4b9eb8cd68bcbf4f` | `make t1_date_latency_experiment_b`, then `build/bin/T1DateLatencyExperiment results/date2026/t1_group_latency stage_a 10000`, `... formal 100000`, and `... formal_1m 1000000`. | Frozen group-latency evidence for `T1_DATE2026_GROUP_LATENCY_FORMAL_RESULTS.md` and the DATE evidence index. |

## Ordinary-Git boundary

Git tracks the source, runner, configuration, compact manifests, checksums,
aggregate summaries, and human-readable closure documents required to identify
these artifacts. The listed corpora and raw outputs remain outside ordinary
Git; a future archive policy may assign their immutable storage location, but
must preserve these paths and digests or document an explicit successor.

The normalized 1k artifact currently resides beneath `01_my_note/`, which is
intentionally excluded from the P1-NAME checkpoint. Its compact provenance is
therefore represented here rather than by adding that tree wholesale.
