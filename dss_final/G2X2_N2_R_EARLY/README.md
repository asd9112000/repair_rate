# G2X2 N2 R EARLY RTL

`CASE_ID: G2X2_N2_R_EARLY`



Synthesizable row-only `Grid2x2RowStaticEarly` implementation. It retains the
frozen immediate `R,L,RB,B` ordering and HYP02 prefix graph, with RB aliased to
the local 2R2C analyzer configuration. Final address state contains only
committed selected physical lines.

Authoritative lifecycle and architecture records: `dss_final/INDEX.md` and `dss_final/records/N2_ARCHITECTURE_CONTRACT.md`. Final PPA: `dss_final/N2_HARDWARE_SUMMARY.md`.
