# Remaining-four N2 CA-LIVE latency archive

Measurement boundary for every table in this directory:

- Measured: policy-side `state_update` to policy solution response.
- Not measured: raw-fault arrival to state update.
- Raw-fault end-to-end: architectural estimate only.

`CONFIG_RANK != MEASURED_POLICY_LATENCY_CYCLES`. The R-EARLY cases have only
ranks 1 through 3, while their directly measured maximum is four cycles.
Rank distributions and latency statistics are therefore archived separately.

The GROUP distribution was replayed with the exact 10,000-vector corpus and
functional lockstep enabled. It is histogram evidence, not a count inferred
from the reported 5.5-cycle mean.

## GROUP cycle decomposition

The observed five- or six-cycle outcome follows the actual R-GROUP controller
and the existing full-top done-event schedule. `scan_slot_q` visits canonical
records 0, 1, and 2; the final-record branch freezes the SA only when its
`test_done` condition has already arrived. `ST_DECIDE` then registers
`solution_ready_o` on the next controller edge.

- When `test_done_seen_q` is set on the final-SA state-update edge, the final
  record can freeze the SA directly; the registered group response is visible
  at the five-cycle measurement boundary.
- When done is delivered after scan completion, the final record enters
  `ST_WAIT_DONE`; its done edge moves to `ST_DECIDE`, and the registered
  response is visible one edge later, at six cycles.

Thus the measured distinction is the observed done-gate/controller transition
and registered response visibility. It is not an assumed “three configs plus
one cycle” rule. The exact replay alternates the pre/post completion ordering
through the pre-existing full-top harness, producing 5,000 samples in each
measured bin for both GROUP cases.

## Descriptive-only EARLY reference

| Case | First-config legal selection | Mean policy latency (cycles) |
| --- | ---: | ---: |
| G2X2 RC EARLY | 26,014 / 34,277 = 75.89% | 1.31114 |
| G2X2 R EARLY | 2,740 / 5,034 = 54.42988% | 1.71812 |
| L1X4 R EARLY | 2,557 / 4,311 = 59.31338% | 1.46254 |

This is descriptive only. These measurements alone do not establish that
resource type or topology caused the first-config-selection differences.
