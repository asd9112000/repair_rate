# P3-BL-RTL-A — SYN-B synthesis readiness

```text
SYN_B_SYNTHESIS_READY: YES
TOP_OPT0: recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_top
TOP_OPT1: recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top
CLOCK_RESET: clk_i; synchronous active-low rst_ni
RUNTIME_OPT_SELECTION_MUX: NO
UNRESOLVED_BLACK_BOXES: NO
TEST_ONLY_CONSTRUCTS_IN_SYNTHESIS_PATH: NO
DC_SYNTHESIS_RUN: NO (out of scope)
```

The source manifest is the two package files, `dss_v2_group_slot_decode`, `recam_shared_config_analyzer`, candidate transition adapter, candidate producer, chosen search core, atomic commit, integration shell, and selected top. The Makefile integrated target lists this closure in compile order.

Likely later PPA cost centers are the shared analyzer, two concurrent 480-bit maps, DFS stack/control, selected tuple registers, shadow commit state, commit validation, and persistent ledger. No foundry area or timing claim is made here.

```text
SYNTHESIS_COMPARISON_BOUNDARY: integrated SYN-B top
SYN_B_CONTAINS: snapshot storage; shared analyzer; candidate producer; 480-bit candidate map; GLOBAL DFS; selected tuple state; shadow atomic commit; persistent ledger
HISTORICAL_61751_AREA_GLOBAL_RESULT: search-core-only
DIRECT_COMPARISON_HISTORICAL_61751_VS_SYN_B: FORBIDDEN
```

The old 61,751-area GLOBAL result must never be compared directly with this
integrated SYN-B boundary. A later comparison with SYN-A must use the defined
integrated synthesis object on both sides.
