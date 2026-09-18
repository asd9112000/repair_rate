# Canonical Directional GLOBAL RTL Implementation Status

> Date: 2026-09-18
> Status: GLOBAL-NoScratch functional gate CLOSED; synthesis COMPLETE

## RS2 synthesis closure

The accepted W-2024.09-SP2 / TSMC018 slow.db / 20 ns / zero-I/O /
`compile -map_effort low` run completed on 2026-09-18. The mapped core reports
`61,751.290432` total cell area (`6,188.00` NAND2X1 GE), `26,917.228756`
combinational area, `34,834.061676` sequential area, and 616 sequential cells.
The worst path is `depth_q_reg[0]` to `selected_release_q_reg[3]`, with
19.87 ns critical-path length, `+0.02 ns` WNS, and 41 logic levels. The full
reports and PASS metadata are in
`results/dss_canonical/dc_tsmc018_slow/rs2_global_noscratch_20ns`.

The older blocked attempt below is retained as provenance. GLOBAL-WithScratch
remains `NOT_STARTED`.

## Architecture

`recam_dss_canonical_global_noscratch_core.v` is an iterative, fixed-depth DFS
controller, not a combinational Cartesian-product expansion or a translation
of C++ recursion. It captures the complete RS2 candidate history, visits
A→B→C→D, and uses `R,L,RB,B` plus ascending PatternID.

Per-depth speculative state is `{released_mask, used_mask, release_req_mask,
borrow_count}`. Four snapshots make rollback a register select rather than a
resource-ledger mutation. Static ownership, release resource, donor resource,
and action rank are combinational tables. The committed physical ledger is
outside this core and is updated only after a complete tuple is returned.

Because a valid PatternID can consume fewer physical lines than its Config
envelope, the input contract includes one actual-release and one actual-borrow
bit per candidate in addition to the required 160 valid bits. Dedicated
no-scratch history is therefore 480 bits. Total registered architectural state
is 630 bits, including the explicit 12-bit RS2 ConfigID trace.

## Verification

```text
READABLE_VERILOG_WORKFLOW: manual modify/validate guidance applied
AUTOMATED_SKILL_GATE: BLOCKED by existing Python 3.8.10 typing runtime
VERILATOR_LINT: PASS
DIRECTED_VECTORS: 6
SEEDED_CANDIDATE_MAP_VECTORS: 1000 (seed 20260918)
GLOBAL_NOSCRATCH_RTL_MISMATCHES: 0
```

Directed coverage includes first-choice success, future donor satisfied,
future donor violated, A/B backtracking, search exhaustion, and the canonical
N2/F16/group172 tuple. The independent test oracle compares repairability,
action, PatternID, donor, actual release/borrow, and final speculative masks.

The registered done pulse occurs with stable result outputs. Minimum search is
four candidate visits. The bounded full four-level tree contains at most
`40 + 40^2 + 40^3 + 40^4 = 2,625,640` candidate visits.

## RS2 synthesis attempt

The accepted W-2024.09-SP2 / TSMC018 slow.db / 20 ns / zero-IO /
`compile -map_effort low` runner was launched immediately after functional
closure. The executable stopped before analyze/map:

```text
Fatal: Design Compiler is not enabled. (DCSH-1)
```

Therefore area, GE, comb/seq area, sequential cells, WNS, critical path, and
logic levels are `BLOCKED`, not estimated. Metadata and the exact rerun script
are retained under `results/dss_canonical/dc_tsmc018_slow/rs2_global_noscratch_20ns`.

GLOBAL-WithScratch remains `NOT_STARTED`: no physical scratch ownership,
capacity, ports, or safe overwrite lifetime has been frozen.
