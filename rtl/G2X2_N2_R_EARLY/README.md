# G2X2 N2 R EARLY RTL

Synthesizable row-only `Grid2x2RowStaticEarly` implementation. It retains the
frozen immediate `R,L,RB,B` ordering and HYP02 prefix graph, with RB aliased to
the local 2R2C analyzer configuration. Final address state contains only
committed selected physical lines.
