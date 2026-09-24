# N2 synthesis methodology

Canonical matched N2 PPA uses Synopsys Design Compiler W-2024.09-SP2 with
TSMC018 `slow.db`, slow corner, a 20.0 ns clock, zero I/O delay, and the
accepted common compile methodology. `NAND2X1 = 9.979200` µm².

GE is calculated as total cell area divided by the NAND2X1 area. Reported PPA
uses cell area, not DC wire-load net area. All FINAL matched comparisons use
the corrected 13-bit physical-column contract. Historical 5-bit RECAM results
are preserved for provenance only and are not the matched baseline.

The authoritative values and ratios are in
[N2_HARDWARE_SUMMARY.md](../N2_HARDWARE_SUMMARY.md).
