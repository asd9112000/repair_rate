# E0-L — DATE 2x2 Formal Group-Level Latency Results

`E0L_STATUS: COMPLETE`

This run executes the frozen `E0L_DATE_2X2_LATENCY_V1` recipe: 10,000 paired
2x2 directional-CAM groups, `RS=2`, `CS=2`, `SHARE_M=1`, seed `20260914`,
with EARLY and `GROUP_NO_SCRATCH_V2` driven by the same analyzer-derived
candidate corpus.  The production RTL was not modified.  The BIST timeline is
`MODEL_DERIVED`; all system metrics are integer cycles.

Corpus SHA-256:

```text
a8201b748aa272a42d3587448b363bccf58f66a29c9eacdc4e3eff6f5e707d76
```

## Outcome counts

| BOTH_PASS | EARLY_ONLY | GROUP_ONLY | BOTH_FAIL |
| ---: | ---: | ---: | ---: |
| 5944 | 0 | 1749 | 2307 |

## All-terminal group metrics (cycles)

| Metric | EARLY mean / median / p95 | GROUP mean / median / p95 | GROUP-EARLY mean / median / p95 |
| --- | --- | --- | --- |
| DSS Decision | 4.6517 / 4 / 7 | 20.9871 / 20 / 23 | 16.3354 / 16 / 18 |
| Group Post-BIST ARCH | 4.6517 / 4 / 7 | 20.9871 / 20 / 23 | 16.3354 / 16 / 18 |
| Group Fault-Tail ARCH | 2855.575 / 2158 / 7916 | 2871.9104 / 2175.5 / 7931 | 16.3354 / 16 / 18 |

All 10,000 formal groups are positive-fault (`7` faults per SA); zero-fault
count is zero.  Fault-tail is dominated by serial-BIST tail, whereas the
policy difference is the DSS decision component.  The BOTH_PASS-only DSS
delta is mean `16.1285`, median `16`, p95 `17` cycles.

## Integrity

```text
CSV_ROWS: 20000
CORPUS_ALIGNMENT: PASS
handoff_gap_0_count: 0
handoff_gap_1_count: 20000
handoff_gap_other_count: 0
post_bist_identity_failures: 0
fault_tail_identity_failures: 0
paired_alignment_failures: 0
PRODUCTION_RTL_CHANGED: NO
REPAIR_SEMANTICS_CHANGED: NO
```

`+1` remains `HARNESS_ARTIFACT`; raw fields retain it and ARCH fields subtract
it.  Mapped-delay sensitivity only: EARLY uses 19.70 ns/cycle and GROUP uses
19.74 ns/cycle.  No Post-BIST or Fault-tail quantity is converted to ns.

Artifacts are in `results/date_2x2_latency/E0L_DATE_2X2_LATENCY_V1/`.
No repair-rate, SA system-latency, device-latency, or manuscript work is
authorized by this result.

```text
GROUP_LATENCY_RESULT_READY_FOR_DATE: YES
SA_LEVEL_SYSTEM_LATENCY_READY: PARTIAL
REPAIR_RATE_RESULT_READY: NO
DEVICE_LATENCY_READY: NO
NEXT_PHASE_AUTHORIZED: NONE
```
