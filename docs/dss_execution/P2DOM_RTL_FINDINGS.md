# P2DOM RTL findings log

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

## FINDING ID: P2DOM-RTL-F01

```text
DATE: 2026-09-18
EXPERIMENT: P2DOM reference audit
OBSERVATION: Safe class collapse reduces the theoretical frontier from
2,625,640 to 4,680 candidate evaluations.
EVIDENCE: P2DOM_SEARCH_COMPLEXITY.md; the safe key includes explicit_release.
INTERPRETATION: Deterministic semantic frontier reduction, not a cycle, area,
or timing result.
LIMITATION: Class extraction can add priority/comparator delay.
PAPER/THESIS RELEVANCE: Candidate search information can be reduced while
retaining canonical tuple semantics, subject to RTL equivalence verification.
```

## FINDING ID: P2DOM-RTL-F02

```text
DATE: 2026-09-18
EXPERIMENT: P2DOM reference audit
OBSERVATION: Failed-subtree dominance is safe only from an earlier fully
explored prefix at the same depth.
EVIDENCE: P2DOM_DOMINANCE_PROOF.md and 1,000 fixed-seed maps.
INTERPRETATION: A small depth-local frontier is the correct first cache test.
LIMITATION: Benefit and PPA cost are workload- and implementation-dependent.
PAPER/THESIS RELEVANCE: Canonical ordering constrains dominance use in hardware.
```
