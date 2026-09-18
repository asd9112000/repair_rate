# `recam_dss_canonical_global_noscratch_core` specification

## Boundary

This RS2 decision/resource core captures the complete 160-candidate map and
two per-candidate physical-effect bits. It performs an iterative, fixed-depth
four-SA DFS. It does not contain the RECAM analyzer or mutate the committed
resource ledger while searching.

Candidate address is `SA*40 + action*10 + PatternID-1`. Action encoding is
`L=0, R=1, B=2, RB=3`; traversal is `R,L,RB,B`, then ascending PatternID.
`candidate_release_i` and `candidate_borrow_i` record actual physical-line
effects of that PatternID, which can be smaller than its configuration
envelope.

## State and timing

- Dedicated no-scratch candidate history: 480 bits.
- Four snapshots of `{released[3:0], used[3:0], release_req[3:0], borrow_count[1:0]}`.
- Four six-bit cursors, selected tuple registers, depth, and FSM state.
- Future borrow is legal speculatively and sets the exact donor resource in
  `release_req`; the future owner's selected action must explicitly release it.
- `start_i` is sampled only in IDLE. Inputs may change after that edge.
- `done_o` is a registered one-cycle pulse. Result outputs remain stable until
  the next accepted start.
- RS2 ConfigID is registered with the selected tuple: A/D map L,R,B,RB to
  0,4,5,6 and B/C map them to 0,1,2,3.

```text
clk       _/\_/\_/\_/\_/\_/\_/\_
start     __/‾‾\___________________
busy      ____/‾‾‾‾‾‾‾‾‾‾‾\_______
search       A ... B ... C ... D
done      __________________/‾‾\___
result    ==================<valid>==
```

Reset is synchronous to `clk_i` and active low. Reset clears candidate
history, snapshots, selection, and outputs.

## Static topology

Resource IDs are `A_ROW=0, D_ROW=1, B_COL=2, C_COL=3`. Release resources by
SA are `0,2,3,1`; borrow resources are `2,1,0,3`, matching the canonical C++
`PhysicalResourceLedger` directional ownership.
