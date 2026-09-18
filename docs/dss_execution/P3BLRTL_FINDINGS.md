# P3-BL-RTL findings

## P3BLRTLA-F01

```text
DATE: 2026-09-19
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
OBSERVATION: SYN-A provides one externally selected SA snapshot; it does not contain four-SA snapshot storage.
EVIDENCE: recam_dss_canonical_rs2_streaming_early_top has one snapshot port family and current_sa_o request output.
DESIGN_IMPLICATION: SYN-B captures four flattened 204-bit snapshots, for 816 bits total.
SYNTHESIS_IMPLICATION: Reusing one snapshot for all GLOBAL depths would be an invalid smaller design.
PAPER_RELEVANCE: Preserves distinct per-SA fault state in group-global repair.
```

## P3BLRTLA-F02

```text
DATE: 2026-09-19
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
OBSERVATION: Full GLOBAL candidate generation can use one analyzer in sixteen sequential requests.
EVIDENCE: producer regression obtains 23 valid slots per zero-fault SA and captures 92 valid candidates after sixteen requests.
DESIGN_IMPLICATION: Candidate generation is area-conscious but contributes fixed front-end latency.
SYNTHESIS_IMPLICATION: Analyzer replication is not needed for the frozen baseline.
PAPER_RELEVANCE: Separates analyzer sharing cost from DFS cost.
```

## P3BLRTLA-F03

```text
DATE: 2026-09-19
ARCHITECTURE: GROUP_GLOBAL
OBSERVATION: GLOBAL legality can be committed with shadow staging without exposing partial ledger state.
EVIDENCE: atomic-commit test covers successful publish, staged failures, invalid donor, and nonempty pre-state rejection with zero corruption.
DESIGN_IMPLICATION: GROUP_GLOBAL has delayed all-or-nothing commit while EARLY retains incremental commit.
SYNTHESIS_IMPLICATION: Commit shadow/validation is an integrated-top cost absent from core-only synthesis.
PAPER_RELEVANCE: Makes the fair external boundary explicit despite intentional policy timing differences.
```
