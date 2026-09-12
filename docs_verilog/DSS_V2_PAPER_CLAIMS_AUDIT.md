# DSS V2 — Paper Claims Audit

| Claim | Supporting phase | Supporting artifact | Safe wording | Overclaim to avoid |
|---|---|---|---|---|
| V2 EARLY reduces area versus legacy EARLY | 4E/4H | `PHASE4E_CHARACTERIZATION.md`; Phase 4E `area.rpt` | “For the implemented DSS decision/resource-management boundary, V2 EARLY reduces total cell area by 31.0083% versus the legacy EARLY reference at 20 ns.” | “The complete BISR system is 31% smaller.” |
| V2 GROUP reduces area versus legacy GROUP | 4F/4H | `PHASE4F_CHARACTERIZATION.md`; Phase 4F `area.rpt` | “V2 GROUP-NoScratch reduces total cell area by 10.0270% versus the legacy GROUP reference at the same boundary and constraint.” | “GROUP storage is eliminated.” |
| GROUP improves repairability | 4J | `phase4j_policy_tradeoff_summary.csv` | “On the fixed 90,000-pair DATE corpus, GROUP repaired 85.118889% versus EARLY’s 79.906667%, a +5.212222 pp difference.” | “GROUP always improves repairability.” |
| Largest measured primary-sweep gain | 4J | `phase4j_policy_tradeoff_summary.csv` | “At 28 faults/group in the moderate-imbalance sweep, GROUP exceeded EARLY by +12.77 pp.” | “Repairability gain grows monotonically with fault count.” |
| EARLY requires less slack to hide decision work | 4I | `phase4i4_threshold_summary.csv`; `PHASE4I_FINAL_CHARACTERIZATION.md` | “The measured `G_zero` P50 is 4 cycles for EARLY and 20 for GROUP-NoScratch.” | “EARLY has zero analysis latency.” |
| Event model matches representative RTL timing | 4I-3/4I-4 | `phase4i4_rtl_validation_summary.csv` | “The event model had zero mismatches across eight representative production-top RTL cases.” | “The model is formally cycle-exact for all workloads.” |
| GROUP_ONLY mechanism is explained | 4K | `phase4k_group_only_classification.csv`; `phase4k_config_transition_matrix.csv` | “All 4,691 GROUP_ONLY cases in this corpus first diverged at A, where GROUP selected CFG4 rather than EARLY CFG0 and released A_ROW.” | “CFG4 release proves GROUP dominates any EARLY policy.” |
| Scratch is not useful at this boundary | 4G/4H | `PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md` | “Lifetime analysis did not justify Scratch for the current frozen GROUP decision boundary.” | “Scratch is never useful for DSS.” |

Use only the safe wording or a narrower statement.  Each claim is scoped to the
implemented 2×2 Directional DSS boundary, fixed technology/constraint where
applicable, and stated corpus where applicable.
