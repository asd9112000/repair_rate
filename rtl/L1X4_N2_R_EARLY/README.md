# L1X4 N2 R EARLY RTL

Synthesizable `Line1x4RowStaticEarly` implementation. The controller visits
A through D and commits immediately in `R,L,RB,B` priority. Endpoint legality
and upstream-release checks enforce the frozen directed single-hop topology.
Final addresses are emitted only in the immediate commit transaction.
