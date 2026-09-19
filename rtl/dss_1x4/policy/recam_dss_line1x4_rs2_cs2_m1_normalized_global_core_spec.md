# 1x4 Single-Hop GLOBAL core specification

The core captures the 180-slot producer table on `start_i`, reduces same-demand
candidates by `(PatternID, attemptIndex)`, and explores every canonical
A→B→C→D tuple with private speculative eight-row ownership state.  It reports
the C++ objective winner but has no persistent resource side effects.

Reset is active-low and synchronous.  `search_done_o` pulses after the full
search; `group_repairable_o` and the selected tuple remain stable until the
next accepted start.  `candidate_evaluations_o` and `dfs_cycles_o` count search
edges, not a fixed latency guarantee.

```text
TIMING: start capture -> data-dependent DFS -> search_done
CORNER_CASES: no candidate; non-neighbor request; exhausted donor; equal-demand candidate reduction
VERIFICATION: shared C++ GLOBAL oracle, directed objective ties, no-forwarding, D/A boundary
```
