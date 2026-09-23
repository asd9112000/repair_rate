# G2X2 N2 RC EARLY RTL

Synthesizable implementation of `Hyp02StaticEarly` with the frozen 13-bit
physical-column interface. The controller commits each SA immediately in
`R,L,RB,B` priority. `dss_early_selected_address_regs` preserves only the
selected physical line required by the output interface; it does not retain
unselected row/column pivot pairs.

Run the local physical and core-oracle checks with
`scripts/simulation/run_verilator_test.sh` using the source/test pairs in
`verification/`.
