# N2 → N3 RC-GROUP CA-LIVE scaling at matched 20 ns

## Comparator provenance

The N2 comparator was re-read from the committed canonical N2 evidence:

- `dss_latency/N2_CONTINUOUS_ANALYSIS/seven_case_20ns/N2_SEVEN_CASE_20NS_PPA.csv`, row `G2X2_RC_GROUP_CA_LIVE`
- `dss_latency/N2_CONTINUOUS_ANALYSIS/seven_case_20ns/N2_LATENCY_SUMMARY.csv`, same row
- `dss_final/continuous_analysis_live/G2X2_N2_RC_GROUP_CA_LIVE/CASE.md`

It independently confirms N2 area `171399.415025 um²`, GE `17175.6669`, design WNS `0.00 ns`, critical-path slack `+0.02 ns`, critical path `19.88 ns`, and policy latency `5 cycles`.  In particular, `+0.02 ns` is the N2 critical-path slack, not its Design WNS.

## Structural and measured scaling

| Metric | N2 RC-GROUP | N3 RC-GROUP | N3/N2 or delta |
|---|---:|---:|---:|
| RS/CS | 2/2 | 3/3 | — |
| MAX_K | 5 | 7 | +2 |
| Hybrid entries | 7 | 17 | +10 |
| Analyzer payload | 272 bits | 524 bits | +252 bits |
| PatternID width | 4 bits | 6 bits | +2 bits |
| GROUP store | 80 bits | 112 bits | +32 bits |
| Reconstruction retained | 472 bits | 656 bits | +184 bits |
| Private pivot capture | 440 bits | 616 bits | +176 bits |
| Config evaluations / SA | 4 | 4 | 0 |
| Policy latency | 5 cycles | 5 cycles | 0 |
| Total area | 171399.415025 um² | 685591.004318 um² | 3.99996117× / +299.996117% |
| GE | 17175.6669 | 68702.0006 | 3.99996117× / +299.996117% |
| Combinational area | 138614.415895 um² | 640132.420814 um² | 4.61807970× / +361.807970% |
| Sequential area | 32784.999130 um² | 45458.583504 um² | 1.38656656× / +38.656656% |
| Total cells | 7942 | 27448 | 3.45605641× / +245.605641% |
| DESIGN_WNS_NS | 0.00 | −24.41 | −24.41 ns |
| CRITICAL_PATH_SLACK_NS | +0.02 | −24.41 | −24.43 ns |
| CRITICAL_PATH_NS | 19.88 | 44.23 | 2.22484909× / +122.484909% |

## Latency evidence

N3 policy latency remains five cycles:

- 10,000 formal GENERAL control cases: five cycles in every case.
- Six directed repairable/N3-specific sidecar cases: five cycles in every case.

The formal corpus was a control corpus with no repairable samples; it is not a repair-rate measurement.

## Narrow interpretation

For this matched G2X2 RC-GROUP CA-LIVE architecture point, scaling from RS=CS=2 to RS=CS=3 increases retained state and N3 cell area substantially while preserving the five-cycle policy protocol.  The frozen N3 point fails the matched 20 ns target, whereas N2 meets it.  This is a comparison of one architecture family and these synthesis boundaries only; it does not establish a universal result for every DSS architecture.

Structural bit-width scaling is reported separately from synthesized area attribution.  The N3 hierarchy report directly attributes area to the analyzer, controller/candidate store/selector, and reconstruction; it must not be inferred from the field widths alone.
