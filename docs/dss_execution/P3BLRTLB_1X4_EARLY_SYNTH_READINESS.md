# P3-BL-RTL-B — SYN-C synthesis readiness

```text
SYN_C_SYNTHESIS_READY: YES
TOP: recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_top
CLOCK_RESET: clk_i; synchronous active-low rst_ni
UNRESOLVED_BLACK_BOXES: NO
TEST_ONLY_CONSTRUCTS_IN_SYNTHESIS_PATH: NO
VERILATOR_LINT: PASS
DC_SYNTHESIS_RUN: NO (out of scope)
```

The source manifest is the adapted shared analyzer, candidate producer,
streaming EARLY core, and integrated top. The C++ corpus generator and
Verilator testbench are verification-only files.
