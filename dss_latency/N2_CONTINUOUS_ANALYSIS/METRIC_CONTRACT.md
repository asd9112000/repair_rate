# Metric contract

- `STATE_UPDATE_TO_SOLUTION_READY` starts at the registered state-generation
  update edge and excludes BIST-done waiting.
- `FAULT_INPUT_TO_SOLUTION_READY` adds the separately modeled one-cycle
  fault-to-state-update boundary.
- Commit latency is recorded independently and requires BIST-done gating.
