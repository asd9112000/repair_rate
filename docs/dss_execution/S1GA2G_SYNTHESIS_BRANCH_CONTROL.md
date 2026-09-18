# S1G-A2G Synthesis / GROUP-Policy Reconciliation Branch Control

## Current control state

```text
PHASE:
S1G-A2G SYNTHESIS / GROUP POLICY RECONCILIATION

STATUS:
PAUSED

RECONCILIATION:
COMPLETE

SYNTHESIS:
NOT STARTED / NOT RESUMED

NEXT ACTION:
WAIT FOR EXPLICIT USER AUTHORIZATION
```

The controlling reason for this pause is that repair-rate C++ simulation and
policy exploration have higher immediate priority than matched-boundary
synthesis characterization.

## Frozen evidence and architecture interpretation

`DATE-LAT-T1-1M` remains frozen at one million groups.  Its raw CSV is not to
be regenerated or overwritten without explicit authorization.  The accepted
reconciliation in
`POST_T1_SYNTHESIS_AND_GROUP_POLICY_RECONCILIATION.md` remains authoritative:

```text
S1G-A2G = EARLY-family retained-state overlap architecture
CURRENT_GROUP_RTL_POLICY = GROUP-NoScratch V2

GROUP waits for all A/B/C/D candidate records = YES
GROUP final allocation order = A -> B -> C -> D
GROUP rollback = NO
GROUP global exhaustive search = NO
GROUP retained state = 80-bit candidate history + 18-bit ledger
                     + 6-bit FSM/counters + 53-bit result/status = 157 bits
```

S1G-A2G is not the GROUP policy architecture and the GROUP policy must not be
reinterpreted as a global exhaustive-search implementation.

## Frozen synthesis disposition

```text
RECENTLY_PAUSED_SYNTHESIS_TARGET:
S1G-A2G retained-state overlap architecture

RECENTLY_PAUSED_SYNTHESIS_POLICY_FAMILY:
EARLY

S1GA2G_SYNTHESIS_STATUS:
PAUSED

BASELINE_EARLY_SYNTHESIS:
NEEDS_RERUN

GROUP_NO_SCRATCH_V2_SYNTHESIS:
NEEDS_RERUN
```

Phase 4E/4F are preserved as valid historical 2x2 decision-boundary synthesis
evidence.  They omit the retained collector/overlap boundary, so neither can
be used as a matched direct S1G-A2G cost delta.

## Prohibited work while paused

- Do not run Design Compiler or any matched-boundary EARLY, GROUP, or S1G-A2G synthesis.
- Do not create a new PPA comparison table.
- Do not modify S1G-A2G, baseline EARLY, or GROUP-NoScratch V2 RTL.
- Do not resume S1G-B or alter GROUP candidate-history storage.
- Do not perform GROUP-GLOBAL RTL work; that exploration belongs to the C++
  repair-rate branch unless separately authorized.

## Resume gate

Only explicit user authorization can resume this branch.  The future task is
matched-boundary synthesis characterization of baseline EARLY,
GROUP-NoScratch V2, and S1G-A2G under one frozen source boundary, library,
corner, clock, I/O assumptions, and compile methodology.  Its objective is to
measure the hardware cost of the measured overlap-latency benefit.
