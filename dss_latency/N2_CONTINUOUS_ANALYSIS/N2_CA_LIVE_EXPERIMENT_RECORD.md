# N2 CA-LIVE experiment record

## Scope and immutable boundary

This record closes the N2 continuous-analysis live-state policy experiment.
It covers the EARLY and GROUP CA-LIVE policy-engine tops, their directed and
lockstep evidence, and matched 20.0 ns TSMC018 slow-corner synthesis.  The
comparison boundary intentionally excludes the upstream persistent
fault/CAM collector, matching the old-canonical policy-engine boundary.

`RAW_FAULT_END_TO_END` is therefore **NOT_IMPLEMENTED**.  Raw-fault latency
is not an RTL measurement in this experiment.

## Functional closure

The frozen source closure and individual file roles are recorded in
[`provenance/CA_LIVE_SOURCE_FREEZE.md`](provenance/CA_LIVE_SOURCE_FREEZE.md).
The aggregate controller/top digests are:

```text
EARLY_LIVE_SHA: 4393c2e33bf27bedb4e4733183def16196cdbacf26aa21a997e619d20f56b229
GROUP_LIVE_SHA: 772fbfac5bac58191f4bed577a8e7cfa3fea7575bf6877b9288f8b0cc62e413a
ANALYZER_HASH_MATCH: PASS
SELECTOR_HASH_MATCH: PASS
```

EARLY and GROUP retained functional lockstep against their canonical policy
reference.  The EARLY directed regression confirms same-edge fault update
wins over an old candidate.  The GROUP directed regression confirms that the
same edge suppresses an old map write; the 1,000-vector full-top check also
locks physical address reconstruction to the frozen analyzer/reconstruction
integration.

## Measured policy latency

The measured origin is accepted live policy-state update; the measured
destination is policy solution ready.

| Case | min | mean | median | p95 | max |
| --- | ---: | ---: | ---: | ---: | ---: |
| EARLY CA-LIVE | 1 | 1.31114 | 1 | 3 | 4 cycles |
| GROUP CA-LIVE | 5 | 5 | 5 | 5 | 5 cycles |

EARLY selected legal-solution rank distribution:

| Rank | Count | Denominator | Fraction | Policy latency |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 26014 | 34277 | 75.89% | 1 cycle |
| 2 | 6263 | 34277 | 18.27% | 2 cycles |
| 3 | 1598 | 34277 | 4.66% | 3 cycles |
| 4 | 402 | 34277 | 1.17% | 4 cycles |

**75.89% is the fraction of selected legal solutions, NOT repair rate.**
Vectors ending at an earlier unrepaired SA do not contribute to this selected
legal-solution denominator.

## Matched 20 ns PPA closure

The matched flow used DC W-2024.09-SP2, TSMC018 `slow.db`, slow corner,
20.0 ns clock period, zero I/O delay, the accepted `compile -map_effort low`
methodology, and `NAND2X1 = 9.979200 um^2`.

| Case | CA-LIVE area | Old-canonical area | Area overhead | Critical path |
| --- | ---: | ---: | ---: | ---: |
| EARLY | 104668.503253 um^2 | 102296.779959 um^2 | +2.318% | 19.76 ns |
| GROUP | 171399.415025 um^2 | 166140.376634 um^2 | +3.165% | 19.88 ns |

Detailed cell, area, timing, constraint, and CA-SB comparison evidence is in
[`CA_LIVE_20NS_SYNTHESIS_SUMMARY.md`](CA_LIVE_20NS_SYNTHESIS_SUMMARY.md).
The 20 ns raw reports and source manifests are retained in each CA-LIVE
case's `synthesis/by_period/20ns/` directory.

## Architecture findings

- Each synthesized CA-LIVE top has exactly one shared analyzer.
- No synthesized `[1087:0]` state-bank vector or wide 4:1 state-selection
  cone is present.
- EARLY retains direct live analysis with no per-SA analyzer-input bank.
- GROUP retains the existing 80-bit candidate map and 440-bit reconstruction
  retention; it has no map clear, no generation tags, and no earlier-SA replay.

The full state and interface contracts are preserved in
[`STATE_DUPLICATION_AUDIT.md`](STATE_DUPLICATION_AUDIT.md) and
[`LIVE_STATE_INTERFACE_SCHEDULING_CONTRACT.md`](LIVE_STATE_INTERFACE_SCHEDULING_CONTRACT.md).
