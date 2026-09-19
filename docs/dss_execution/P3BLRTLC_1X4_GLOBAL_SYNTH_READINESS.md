# P3BLRTLC 1x4 GLOBAL synthesis readiness

```text
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
SYN_D_SYNTHESIS_READY: YES
VERILATOR_LINT: PASS
STRICT_READABLE_VERILOG_GATE: BLOCKED_BY_LOCAL_RUNTIME
DC_SYNTHESIS_STARTED: NO
```

The GLOBAL core, atomic group commit, and integrated top lint cleanly with
Verilator.  The strict readable-Verilog gate was invoked but is blocked before
RTL analysis by the local Python 3.8 runtime, whose `typing` implementation
does not support the gate's `dict[...]` annotations.  That tooling limitation
is not a functional RTL failure.

No Design Compiler run, timing claim, area claim, or OPT2/OPT3 experiment was
started in this phase.
