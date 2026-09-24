# G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg

Classification: `SUPPORTING_ABLATION`, `COMBINED_PREOPT_REFERENCE`,
`SEMANTICS_EQUIVALENT_TARGET`.

This is a `PROVENANCE_BACKED_COMPOSITE_ABLATION`, not a literal historical
snapshot. It uses seven parallel ConfigID analyzers for the scheduled SA,
three registered 160-bit maps, SYN-B OPT1/GLOBAL DFS/atomic commit, and the
canonical 440-bit GROUP pivot retention block. Canonical source files are
dependencies only and are not modified by this case.

The source hierarchy deliberately retains `L,R,B,RB` map storage while the
recovered DFS searches `R,L,RB,B`, then ascending PatternID.

## Address contract

`ROW_ADDR_W=9`, `PHYS_COL_ADDR_W=13`, `WORD_COL_ADDR_W=5`, and
`HYBRID_LINE_ADDR_W=13`. Word columns are not substituted for physical repair
columns. The map itself stores effects only; the retained pivot reconstruction
preserves 13-bit repair-column identity.
