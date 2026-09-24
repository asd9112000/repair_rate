# G2X2 N2 RC EARLY RTL

`CASE_ID: G2X2_N2_RC_EARLY`



Synthesizable implementation of `Hyp02StaticEarly` with the frozen 13-bit
physical-column interface. The controller commits each SA immediately in
`R,L,RB,B` priority. `dss_early_selected_address_regs` is obsolete and absent. `dss_early_selected_address_mux.sv` combinationally emits each selected physical line for the immediate `solution_commit` transaction; no four-SA accumulated final-solution warehouse exists.

Run the local physical and core-oracle checks with
`scripts/simulation/run_verilator_test.sh` using the source/test pairs in
`verification/`.

Authoritative lifecycle and architecture records: `dss_final/INDEX.md` and `dss_final/records/N2_ARCHITECTURE_CONTRACT.md`. Final PPA: `dss_final/N2_HARDWARE_SUMMARY.md`.
