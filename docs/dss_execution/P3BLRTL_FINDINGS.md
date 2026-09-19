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

## P3BLRTLA-F04

```text
DATE: 2026-09-19
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
FINDING: Integrated 2×2 GROUP_GLOBAL requires four distinct SA snapshots (816 registered bits).
EVIDENCE: integrated_shell.v snapshot registers; integrated equivalence test GROUP_SNAPSHOT_ALIAS_ERRORS=0; P3BLRTLA state accounting.
```

## P3BLRTLA-F05

```text
DATE: 2026-09-19
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
FINDING: One shared analyzer produces the map through 16 sequential requests, defined as 4 SA × 4 configuration/action interpretations.
EVIDENCE: candidate_map_producer.v; producer and integrated regressions; P3BLRTLA integrated closure.
```

## P3BLRTLA-F06

```text
DATE: 2026-09-19
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
FINDING: The baseline retains a raw 480-bit valid/release/borrow candidate/effect map, and the DFS keeps its own captured 480-bit copy.
EVIDENCE: candidate_map_producer.v; canonical GLOBAL core; P3BLRTLA state accounting.
```

## P3BLRTLA-F07

```text
DATE: 2026-09-19
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
FINDING: OPT0 and OPT1 are separately elaborated; no runtime optimization-selection mux exists.
EVIDENCE: integrated_shell.v generate block; SYN-B synthesis-readiness report; integrated equivalence OPT0_OPT1_SEMANTIC_MISMATCHES=0.
```

## P3BLRTLA-F08

```text
DATE: 2026-09-19
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
FINDING: GLOBAL uses externally atomic shadow-staged commit: A/B/C/D update private shadow, then one publish edge changes persistent resources.
EVIDENCE: atomic_group_commit.v; atomic-commit regression ATOMICITY_VIOLATIONS=0 and PARTIAL_COMMIT_VISIBILITY_ERRORS=0; P3BLRTLA latency audit.
```

## P3BLRTLA-F09

```text
DATE: 2026-09-19
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
FINDING: Integrated registered logical state is 2042 bits; the value excludes transient combinational data and is not synthesized area.
EVIDENCE: P3BLRTLA state accounting invariant.
```

## P3BLRTLA-F10

```text
DATE: 2026-09-19
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
FINDING: GROUP_GLOBAL total latency is not constant because DFS search latency is data-dependent. The 34-cycle directed all-zero success is a witness, not a fixed latency.
EVIDENCE: integrated equivalence regression INTEGRATED_DIRECTED_SUCCESS_CYCLES=34; P3BLRTLA latency audit and production DFS FSM.
```

## P3BLRTLB-F01

```text
DATE: 2026-09-19
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_STREAMING_EARLY
FINDING: A 2-bit SA index caused an invalid wraparound adjacency between D and A in the 1×4 Single-Hop topology.
ROOT_CAUSE: Adjacency was derived from compact index arithmetic, allowing the terminal SA index D to wrap around to A under 2-bit arithmetic.
INVALID_BEHAVIOR: D → A was incorrectly treated as a legal neighboring relation.
EXPECTED_TOPOLOGY: A ↔ B ↔ C ↔ D
EXPECTED_NON_ADJACENCY: A ↮ C; A ↮ D; B ↮ D
CORRECTION: Replace arithmetic/wraparound-derived adjacency with explicit legal neighbor relations.
LEGAL_EDGES: A-B; B-C; C-D
CORRECTED_RTL: rtl/dss_1x4/policy/recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_core.v (is_adjacent_owner)
POST_FIX_RESULT: CPP_ORACLE_MISMATCHES=0; FINAL_OWNER_MISMATCHES=0; FAILURE_POSITION_MISMATCHES=0; END_TO_END_MISMATCHES=0
EVIDENCE: deterministic shared C++↔RTL 1,000-case oracle corpus and integrated Verilator top test.
```

## P3BLRTLC-F01

```text
DATE: 2026-09-19
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
FINDING: The C++ GROUP_GLOBAL policy can be reproduced with a four-depth private-ledger DFS and one atomic publish.
EVIDENCE: shared deterministic 1,000-case C++↔RTL corpus reports zero selected-tuple, objective, final-owner, adjacency, and forwarding mismatches; atomic and integrated tests report zero visibility errors.
DESIGN_IMPLICATION: Candidate reduction is retained only for equal (usedRows, usedColumns) candidates, resolving the tie with lower (PatternID, attemptIndex), exactly as the C++ policy does.
SYNTHESIS_IMPLICATION: A reusable sequential producer plus captured candidate table avoids analyzer replication while making DFS latency data-dependent.
PAPER_RELEVANCE: The GLOBAL baseline has an auditable all-or-nothing physical-state boundary distinct from STREAMING_EARLY timing.
```
