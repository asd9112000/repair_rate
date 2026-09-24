# G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg architecture

This supporting ablation is a provenance-backed composite rather than a
byte-identical historical build. Its purpose is to retain the pre-optimization
hardware representation while exposing current canonical external semantics.

```text
active-SA snapshot -> 7 parallel Config analyzers -> registered 3x160 maps
                 -> SYN-B OPT1 -> recovered GLOBAL DFS -> atomic commit
                 -> canonical 4 SA x 5 pivot address retention (440 bits)
```

Historical/provenance-backed portions are the 3x160 effect maps, OPT1 class
collapse, deterministic GLOBAL DFS, and atomic group commit. The seven-lane
bank is deliberately physically explicit and has no Config analyzer time mux.

Current correction/port portions are the 9-bit row plus 13-bit physical-column
and Hybrid identity contract, canonical role masks, and canonical 440-bit
GROUP pivot retention. In particular, physical column 1 and 257 remain
distinct; no 5-bit word-column value enters repair-side identity or retention.

## Canonicalization for controlled ablation

The case retains the historical DFS mechanism, but its `R,L,RB,B` traversal is
not the final selection authority. The dense map is projected to its first
PatternID per stored slot and the frozen canonical 81-path selector chooses the
visible tuple. This semantic normalization is necessary because historical
priority changes ConfigID/action selection. Role legality is sourced from
`dss_v2_group_slot_decode.sv`: SA0/SA3 use `{0,4,5,6}`, while SA1/SA2 use
`{0,1,2,3}`.

The historical DFS and atomic commit remain live diagnostic hardware paths;
canonical selection provides externally visible repairability, ConfigID, action,
PatternID, donor/release/borrow metadata,
and final retained-pivot reconstruction.

No compact 80-bit store, candidate compression, new DFS pruning, or canonical
case modification is used. The future archive location is
`dss_final/ablation/G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg/`; it is intentionally
not populated before synthesis/review.
