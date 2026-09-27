# N2 CA-LIVE timing sweep summary

All eight records use freeze commit `9bb956608f66891d0967f4f631afb19812c5a724` and the hashes in `provenance/SOURCE_FREEZE.md`.  Newly run points passed source manifests before and after DC; existing 20 ns reports were source/methodology verified and reused.

| Arch | Period | Setup | WNS / TNS ns | Area um^2 | GE | Critical path ns |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| EARLY | 10 | fail | -4.47 / -388.44 | 166243.494219 | 16659.0001 | 14.47 |
| EARLY | 15 | met | 0.00 / 0.00 | 145213.993165 | 14551.6668 | 15.00 |
| EARLY | 20 | met | +0.05 / 0.00 | 104668.503253 | 10488.6668 | 19.76 |
| EARLY | 25 | met | 0.00 / 0.00 | 88957.915904 | 8914.3334 | 25.00 |
| GROUP | 10 | fail | -4.11 / -434.63 | 245558.177297 | 24607.0003 | 14.02 |
| GROUP | 15 | met | 0.00 / 0.00 | 215969.848819 | 21642.0002 | 14.83 |
| GROUP | 20 | met | +0.02 / 0.00 | 171399.415025 | 17175.6669 | 19.88 |
| GROUP | 25 | met | 0.00 / 0.00 | 160575.309425 | 16091.0002 | 24.74 |

EARLY and GROUP close setup at the measured 15/20/25 ns points, not 10 ns.  Therefore, 15 ns is the tightest measured passing constraint.  This coarse sampling does not establish a minimum achievable period, a maximum frequency, or a timing limit.  Tightening 25 ns to 10 ns raises EARLY area 86.9% and GROUP area 52.9%; tightening 20 ns to 15 ns raises EARLY area approximately 38.7% and GROUP area approximately 26.0%.  EARLY exhibits stronger synthesis-area growth under tighter clock constraints than GROUP in this measured sweep; this observation is not generalized beyond these measured points.

EARLY is analyzer-dominated at every point. GROUP terminates in a candidate-store write register at every point, so that path remains limiting. These hierarchy conclusions are stable across 15/20/25 ns, while exact area and max-cap counts vary with the constraint.

Constraint findings: EARLY has 90 setup and 2 max-cap violations at 10 ns, and none at 15/20/25 ns. GROUP has 131 setup plus 8 max-cap at 10 ns; its max-cap counts are 4, 9, and 2 at 15, 20, and 25 ns. Rounded-at-limit `VIOLATED: increase significant digits` cases are preserved in the constraint CSV. No max-fanout or max-transition violation is reported. DC synthesis is not physical signoff.

Functional latency is unchanged: EARLY `1 / 1.31114 / 1 / 3 / 4` cycles; rank-1 selected legal solutions `26014 / 34277 = 75.89%` (denominator: selected legal solutions, not repair rate); GROUP `5 / 5 / 5 / 5 / 5` cycles. `RAW_FAULT_LATENCY` remains `ARCHITECTURAL_ESTIMATE_ONLY`.

The 20 ns point remains the paper-facing primary matched PPA comparison: both architectures have positive WNS there under the same methodology as the canonical N2 results. The 15 ns points are zero-margin measurements, while 25 ns is a looser optimization point; neither replaces the primary 20 ns values.
