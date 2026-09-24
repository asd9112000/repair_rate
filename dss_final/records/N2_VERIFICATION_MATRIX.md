# N2 verification matrix

`PASS` denotes preserved final evidence. `HISTORICAL` denotes retained evidence
from the earlier case closure. `N/A` means the test belongs to another
architecture boundary and was not claimed for that case.

| Coverage | RECAM | RC EARLY | RC GROUP | R EARLY | R GROUP | L1 EARLY | L1 GROUP |
|---|---|---|---|---|---|---|---|
| Verilator lint | PASS | PASS | HISTORICAL | PASS | HISTORICAL | PASS | HISTORICAL |
| Physical `1 != 257` | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Physical-column boundaries | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Low-column old/new equivalence | PASS | N/A | N/A | N/A | N/A | N/A | N/A |
| Independent validity-map oracle | N/A | PASS | N/A | PASS | N/A | PASS | N/A |
| Random oracle vectors | N/A | PASS | N/A | PASS | N/A | PASS | N/A |
| C++ policy lockstep | HISTORICAL | N/A | HISTORICAL | N/A | HISTORICAL | N/A | HISTORICAL |
| Directed tests | PASS | PASS | HISTORICAL | PASS | HISTORICAL | PASS | HISTORICAL |
| Working-to-synthesis hash | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Working-to-archive hash | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Archive-to-synthesis hash | PASS | PASS | PASS | PASS | PASS | PASS | PASS |

RECAM-specific evidence includes the 1000-vector low-column comparison and
historical Phase-3A/3B regressions. EARLY oracle evidence covers 81 legal paths
for both G2X2 cases and 27 paths for L1X4, each with 65,536 validity maps and
1,000 random vectors. GROUP retains physical-column and pivot-retention tests;
this matrix does not claim EARLY-only streaming tests for GROUP.
