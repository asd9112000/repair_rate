# L1X4 N2 R EARLY RTL

`CASE_ID: L1X4_N2_R_EARLY`



Synthesizable `Line1x4RowStaticEarly` implementation. The controller visits
A through D and commits immediately in `R,L,RB,B` priority. Endpoint legality
and upstream-release checks enforce the frozen directed single-hop topology.
`dss_early_selected_address_mux.sv` emits selected physical lines only in the immediate commit transaction; no four-SA accumulated final-solution warehouse exists.

Authoritative lifecycle and architecture records: `dss_final/INDEX.md` and `dss_final/records/N2_ARCHITECTURE_CONTRACT.md`. Final PPA: `dss_final/N2_HARDWARE_SUMMARY.md`.
