# P3-BL-IF findings

## P3BLIF-F01

```text
ARCHITECTURE: all four points
OBSERVATION: A policy-core-only synthesis boundary is not comparable with the existing SYN-A integrated top.
EVIDENCE: SYN-A instantiates analyzer, EARLY policy, feasibility, and persistent ledger; SYN-B core accepts completed maps and has no ledger instance.
DESIGN_IMPLICATION: All later four-point tops must terminate at a committed physical ledger.
SYNTHESIS_IMPLICATION: Do not compare current SYN-A area with SYN-B core-only area or C++ 1x4 results.
PAPER_RELEVANCE: Prevents misleading hardware-cost comparison across architecture families.
```

## P3BLIF-F02

```text
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
OBSERVATION: The existing GLOBAL core has a complete search interface but no collector candidate producer or atomic selected-tuple commit adapter.
EVIDENCE: recam_dss_canonical_global_noscratch_core has candidate-map inputs and tuple/mask outputs; its specification says it does not mutate the committed ledger while searching.
DESIGN_IMPLICATION: Candidate production and group commit are required RTL blocks, not a minor top-level wire-up.
SYNTHESIS_IMPLICATION: SYN-B is not ready at the common integrated boundary until those blocks exist.
PAPER_RELEVANCE: Separates GLOBAL search cost from total integrated repair-path cost.
```

## P3BLIF-F03

```text
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED
OBSERVATION: 1x4 semantics are C++-proven but have no synthesizable RTL baseline.
EVIDENCE: no rtl/dss_1x4 family exists; C++ NeighborSharing and OneByFourSingleHop policies provide current behavior.
DESIGN_IMPLICATION: 1x4 RTL is a baseline implementation project, not a port of 2x2 OPT1.
SYNTHESIS_IMPLICATION: SYN-C and SYN-D cannot enter a four-point DC table yet.
PAPER_RELEVANCE: Preserves the difference between semantic oracle evidence and implementation evidence.
```

## P3BLIF-F04

```text
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
OBSERVATION: 1x4 requires explicit adjacent donor identity; B/C have ordered left/right possibilities and borrowed lines cannot be forwarded.
EVIDENCE: PhysicalResourceLedger::mayBorrow allows only neighboring row owners; R1B freezes B:A-before-C and C:B-before-D priority.
DESIGN_IMPLICATION: The 2x2 (explicit_release, actual_release, actual_borrow) key is not a 1x4 candidate interface or optimization proof.
SYNTHESIS_IMPLICATION: A new 1x4 topology/ledger adapter and independently justified search reduction are required.
PAPER_RELEVANCE: Avoids claiming a topology-independent GLOBAL optimization.
```

## P3BLIF-F05

```text
ARCHITECTURE: GROUP_GLOBAL
OBSERVATION: Search legality and physical commit must be separated, but normal success must make their behavior identical.
EVIDENCE: Existing 2x2 GLOBAL search retains speculative release obligations; existing EARLY ledger performs one committed transaction at a time.
DESIGN_IMPLICATION: GLOBAL commit publishes all-or-nothing group state and preserves tuple metadata until acknowledgement.
SYNTHESIS_IMPLICATION: Partial per-SA commits during GLOBAL search are an invalid implementation boundary.
PAPER_RELEVANCE: Makes different EARLY/GLOBAL timing explicit while retaining a comparable measured boundary.
```
