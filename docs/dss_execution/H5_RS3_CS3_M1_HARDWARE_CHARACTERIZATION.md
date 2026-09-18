# H5 — RS=CS=3, SHARE_M=1 Hardware Characterization

> Status: COMPLETE — synthesis completed; 20-ns timing is **not closed** for
> either original 3,3,1 target representation.

## Methodology and provenance

The target reuses the accepted Phase-4E/4F methodology: Synopsys Design
Compiler W-2024.09-SP2, TSMC018 Arm CBDK `slow.db`, slow operating condition,
20.0-ns clock, zero IO delay, `tsmc18_wl10`, accepted Phase-3J constraints,
`compile -map_effort low`, and NAND2X1 normalization.

The frozen 2,2,1 baselines are compatible Phase-4E/4F results and are reused;
they have the same tool version, library/corner, clock, IO methodology, compile
mode, boundary class, and NAND2X1 area basis.  The target H4-verification
provenance is Git `3304c86` plus target source SHA-256
`854572fd9cd7ec10470ebc261887e4cd5f5d5d1de17b725a671e90c2c940f4b7`.

Raw target reports and metadata are retained in
`results/dss_v2_rs3cs3m1/h5_hardware_retry2/{EARLY,GROUP}/`.  The
machine-readable four-point result is
`results/dss_v2_rs3cs3m1/h5_hardware_retry2/hardware_characterization.csv`.

## Four-point comparison

| Point | Area | Comb. area | Seq. area | Cells (comb./seq.) | GE | Critical path | WNS / TNS | Timing |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| P0 2,2,1 EARLY | 80,871.44 | 76,769.99 | 4,101.45 | 4,114 (4,039 / 75) | 8,104.00 | 19.70 ns | +0.01 / 0.00 ns | CLOSED |
| P1 3,3,1 EARLY | 12,190,913.30 | 12,186,139.92 | 4,773.38 | 298,016 (297,933 / 83) | 1,221,632.33 | 62.52 ns | −42.65 / −3531.71 ns | NOT_CLOSED |
| P2 2,2,1 GROUP-NoScratch | 105,393.66 | 96,611.96 | 8,781.70 | 5,319 (5,162 / 157) | 10,561.33 | 19.74 ns | +0.00 / 0.00 ns | CLOSED |
| P3 3,3,1 GROUP-NoScratch | 12,053,273.51 | 12,042,276.44 | 10,997.08 | 286,964 (286,767 / 197) | 1,207,839.66 | 64.14 ns | −44.24 / −4951.16 ns | NOT_CLOSED |

`GE = total mapped cell area / NAND2X1 area`; the common reference is
`slow/NAND2X1 = 9.979200`.  `STATUS=PASS` in the target metadata means DC
completed, reports were emitted, top/library resolved, and no macro/black-box
area is reported.  It does not mean the 20-ns timing constraint closed.
The original representation completed within the hard cap: EARLY consumed
9,428.95 s wall-clock and GROUP consumed 8,635.48 s wall-clock.

## Scaling observations

| Comparison | Area delta | GE delta | Comb. delta | Seq. delta | Architectural-state delta | Critical-path delta |
|---|---:|---:|---:|---:|---:|---:|
| EARLY: 3,3,1 − 2,2,1 | +12,110,041.87 (+14,974.44%) | +1,213,528.33 (+14,974.44%) | +12,109,369.93 (+15,773.57%) | +671.93 (+16.38%) | +0 bits | +42.82 ns (+217.36%) |
| GROUP: 3,3,1 − 2,2,1 | +11,947,879.86 (+11,336.43%) | +1,197,278.33 (+11,336.44%) | +11,945,664.47 (+12,364.58%) | +2,215.38 (+25.23%) | +40 bits | +44.40 ns (+224.92%) |
| 3,3,1 GROUP − EARLY | −137,639.79 (−1.13%) | −13,792.67 (−1.13%) | −143,863.48 (−1.18%) | +6,223.69 (+130.38%) | +124 bits | +1.62 ns (+2.59%) |

The original target analyzer dominates this mapped implementation: 99.9% of
P1 and 99.5% of P3 mapped cell area.  In the same hierarchy reporting style,
the corresponding 2,2,1 analyzer areas are 70,938.81 (EARLY) and 73,756.27
(GROUP), versus 12,178,895.02 and 11,995,819.93 for the original target
representation.  This observed scaling is consistent with the larger K=7,
49-cell matrix, 35-candidate, 7-address, and 17-Hybrid envelope and especially
with the generic enumerative RTL representation; it is not proof that one
individual architectural structure alone caused the delta.

The 3,3,1 EARLY critical path is `core/sa_q_reg[0]` to
`core/selected_pattern_flat_o_reg[12]`, traversing target configuration decode
and analyzer/candidate evaluation.  GROUP is `core/sa_q_reg[0]` to
`core/store/store_q_reg[5]`, traversing analyzer/candidate evaluation before
candidate-store capture.  Both are therefore classified as
**analyzer/candidate-evaluation dominated**.

## Module and state accounting

| Component | 2,2,1 EARLY | 3,3,1 EARLY | 2,2,1 GROUP | 3,3,1 GROUP |
|---|---:|---:|---:|---:|
| Analyzer | 70,938.81 | 12,178,895.02 | 73,756.27 | 11,995,819.93 |
| Core total | 9,932.63 | 11,649.05 | 31,620.76 | 56,824.89 |
| GROUP candidate store | N/A | N/A | 21,129.29 | 47,534.26 |
| Target/group sequential cells | 75 | 83 | 157 | 197 |

Architectural state is distinct from mapped sequential cells.  The frozen
3,3,1 GROUP candidate-history payload is 112 bits.  Its 197 retained target
state bits/cells include the 112-bit history plus ledger, control, validity,
selected-result, and diagnostic state.  EARLY retains 73 architectural state
bits and maps to 83 sequential cells.  The raw mode-reused CAM requirements
are **280 bits for 2,2,1** and **536 bits for 3,3,1**; these are
`RAW_STORAGE_REQUIREMENT`, not compiled-CAM area.  This RTL maps CAM-related
logic as registers/comparators rather than as a compiled CAM macro.

## Limitations and readiness

No repairability or cost-per-repairability claim is made.  `H0S0-CL-004`
remains OPEN; DynamicSpareSharing GroupCompressed is not frozen V2
GROUP-NoScratch.  Thus hardware provenance is sufficient for a future S1 once
that simulator-semantic conflict is cleared, but S1/E0/E1 are not authorized.

The original target RTL completed within the authorized 14,400-second cap, so
H5O is **not needed and was not started**.  The large mapped size and timing
failure are results for this exact original H4-verified RTL representation;
they are not silently optimized away.

```text
H5 = COMPLETE
HARDWARE_READY_FOR_S1 = YES
READY_FOR_S1 = NO   (H0S0-CL-004 remains OPEN)
READY_FOR_A0 = YES
NEXT_PHASE_AUTHORIZED = NONE
```
