# GROUP commit gating: solution before BIST done

The full-top lockstep holds `sa_bist_done_i=0` through solution readiness.
`solution_commit_o` remains low. After the final BIST-done gate is asserted,
the committed result is immediately eligible with no additional controller edge.
