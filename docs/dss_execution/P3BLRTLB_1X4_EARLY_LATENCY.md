# P3-BL-RTL-B — 1x4 EARLY latency

The top captures four snapshots, accepts the producer start, retains ten
candidate requests, starts the EARLY core, then tests candidates until a first
legal candidate for each SA or first exhaustion. Commit happens on the same
core edge as candidate acceptance and remains persistent.

```text
T_capture: fixed registered capture
T_candidate_generation: 10 fixed capture edges
T_candidate_eval_{A..D}: data-dependent
T_commit_{A..D}: one edge per accepted SA candidate
T_done: one registered observation edge
SUCCESS_LATENCY: DATA_DEPENDENT
```

The all-zero-snapshot directed success takes 18 post-start testbench cycles:
the four SAs each accept attempt 0, PatternID 1 on their first evaluation.
The directed first-failure measurements are A=44, B=60, C=61, and D=47
cycles. These are witnesses, not statistical latency claims.
