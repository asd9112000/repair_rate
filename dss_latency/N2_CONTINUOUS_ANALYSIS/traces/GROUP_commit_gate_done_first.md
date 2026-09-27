# GROUP commit gating: BIST done before solution

The full-top lockstep asserts all BIST-done inputs before analysis completes.
No commit occurs before `solution_ready_o`; the commit becomes eligible on the
same post-edge state in which the registered solution is visible.
