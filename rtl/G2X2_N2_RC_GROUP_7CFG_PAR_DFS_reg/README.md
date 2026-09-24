# G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg

This supporting ablation measures the provenance-backed pre-optimization hardware
representation with canonical DATE2026 selection semantics. It retains seven
parallel analyzers, three registered 160-bit maps, historical OPT1/DFS, atomic
commit, and 440-bit pivot retention.

The visible repair tuple is selected by the exact frozen canonical 81-path
priority. Historical SYN-B `R,L,RB,B` priority remains a live diagnostic path,
not the externally visible policy, because it is not semantics-equivalent to
the canonical selector.

Run the focused checks through the existing Verilator helper with
`tb/dss_canonical/tb_recam_n2_preopt_group_lockstep.sv` and
`tb/dss_canonical/tb_recam_n2_preopt_group_lockstep.cpp`.
