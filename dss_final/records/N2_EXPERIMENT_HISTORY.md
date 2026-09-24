# N2 experiment history

1. **Historical baseline — modeling evidence.** RECAM used a 5-bit
   repair-column representation; its 50,877.288438 µm² result remains
   historical policy evidence.
2. **Physical-column mismatch — bug/modeling correction.** Physical repair
   columns were distinguished from the five-bit logical BIST word column.
3. **Corrected GROUP — architecture implementation.** Three GROUP cases were
   corrected to 13-bit physical columns without changing their policy.
4. **Pivot retention — architecture requirement.** GROUP retains all four SA
   pivot banks: 440 raw row/column address bits.
5. **Initial corrected EARLY — implementation artifact.** EARLY initially
   accumulated four selected solutions in a 300-bit output bank.
6. **Storage-boundary audit — evidence review.** The accumulated bank was
   identified as unnecessary final-result storage for EARLY.
7. **Streaming EARLY correction — implementation correction.** `line_address_q`
   (260 bits), `line_is_row_q` (20 bits), `line_valid_q` (20 bits), and 32
   completed-result metadata bits were removed; the 300-bit warehouse is not
   part of final EARLY architecture.
8. **Streaming EARLY resynthesis — final result.** The three EARLY cases were
   resynthesized with immediate observable commit transactions.
9. **Corrected RECAM resynthesis — final result.** `RECAM_N2_2R2C` established
   the 13-bit physical-column baseline at 72,116.352610 µm².
10. **Seven-case closure — final result.** One RECAM plus six DSS packages were
    promoted as the matched N2 FINAL hardware set at commit `6351a82`.

11. **POST-CLOSURE SUPPORTING ABLATION — pre-optimization GROUP evidence.** `G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg` preserves a seven-parallel-analyzer, 3x160 registered-map, OPT1/GLOBAL-DFS composite while applying frozen canonical selection semantics. It passed exact 1000-vector lockstep and matched DC synthesis; it is supporting evidence, not an eighth canonical N2 result.

Pre-storage-audit EARLY reports and historical 5-bit RECAM reports are
preserved for provenance. They are not canonical PPA inputs.
