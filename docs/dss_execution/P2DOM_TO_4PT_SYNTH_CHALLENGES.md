# P2DOM to four-point synthesis challenges

## C1 — four-point PPA comparison

```text
CLAIM: Four synthesis points can be fairly compared now.
EVIDENCE: P3_4PT_BOUNDARY_AUDIT.md.
ASSUMPTIONS: Comparable integrated RTL boundaries exist.
POSSIBLE_REVIEWER_CHALLENGE: GROUP_GLOBAL is search-core-only and 1x4 is C++ only.
DEFENSE: The claim is not made; synthesis is blocked before producing misleading PPA.
SAFE_TO_CLAIM: NO
```

## C2 — OPT1 speedup

```text
CLAIM: OPT1 lowers GLOBAL hardware latency by the 2,625,640-to-4,680 ratio.
EVIDENCE: Safe candidate-evaluation bound and functional equivalence test.
ASSUMPTIONS: Candidate evaluations equal cycles.
POSSIBLE_REVIEWER_CHALLENGE: The current wrapper retains the 40-slot cursor.
DEFENSE: OPT1 makes no RTL-cycle or PPA claim; OPT2 is required for state/cursor redesign.
SAFE_TO_CLAIM: NO
```

## C3 — reuse 2x2 key on 1x4

```text
CLAIM: 1x4 can use the 2x2 three-field key.
EVIDENCE: None; 1x4 C++ uses demand/ledger candidates and middle donor order.
ASSUMPTIONS: 1x4 has the same explicit-release model.
POSSIBLE_REVIEWER_CHALLENGE: It does not.
DEFENSE: Port is blocked until a 1x4 hardware contract and theorem are frozen.
SAFE_TO_CLAIM: NO
```
