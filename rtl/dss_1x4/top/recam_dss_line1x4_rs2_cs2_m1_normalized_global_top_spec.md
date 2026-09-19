# SYN-D 1x4 Single-Hop GROUP_GLOBAL integrated-top specification

The top captures four 279-bit collector snapshots, invokes the unchanged
ten-request SYN-C producer, starts the GLOBAL search after the candidate table
is complete, and starts atomic commit only for a complete legal tuple.

```text
RESET: synchronous active-low
SUCCESS: producer -> GLOBAL search -> four shadow stages -> one publish -> done
FAILURE: producer -> exhausted search -> done; persistent rows unchanged
EXTERNAL_COMMIT_VISIBILITY: selected tuple and repairable result publish only after commit acceptance
```

The top has no OPT2/OPT3 selector, no runtime optimization mux, and no
black-box dependency.  Its candidate table is exposed only as debug evidence.
