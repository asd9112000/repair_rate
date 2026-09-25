# N2 final-snapshot policy-decision latency

These values measure final-snapshot policy-decision latency. They do not include BIST, raw-fault collection, online-CAM allocation, or device-level scheduling.

- Preflight/formal vectors: 1000 / 10000
- Seed: 20260922
- Clock: 20.0 ns
- Counting: accepted `start_i && !busy_o` rising edge is cycle 0; later events use `event edge - 0`.
- Generator: constrained extension of `tb/recam/recam_shared_config_analyzer_test.cpp`: compact pivot and hybrid prefixes, unique 9-bit rows and 13-bit physical columns, nested threshold masks, valid in-dictionary hybrid cross-edges, and explicit collector-overflow terminal snapshots.
- EARLY and GROUP receive the same held-final snapshot per vector. RECAM is combinational: `RECAM_POLICY_DECISION_CYCLES=N/A_COMBINATIONAL`.
- GROUP `sa_commit_valid_o` is coincident with terminal `done_o`; it is not an EARLY-style early commit event.
