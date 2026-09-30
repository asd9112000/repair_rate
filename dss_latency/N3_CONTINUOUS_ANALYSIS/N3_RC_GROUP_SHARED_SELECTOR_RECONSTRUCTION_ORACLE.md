# N3 RC-GROUP Shared Selector and Reconstruction Oracle

## Scope

This Layer-B change is verification infrastructure only. It extracts independent C++ reference models for the frozen N3 GROUP candidate store, directional resource legality, selection priority, and retained-state reconstruction. It does not modify production RTL, generate a generic full-top corpus, run latency characterization, freeze production sources, or run Design Compiler.

## Frozen width contracts

- Candidate store: 4 SAs x 4 records x (1 valid + 6 PatternID) = 16 x 7 = 112 bits.
- Private pivot capture: 4 x 7 x (9 row + 13 physical column) = 616 bits.
- Reconstruction metadata: 4 commit-valid + 12 selected-Config + 24 selected-PatternID = 40 bits.
- Retained reconstruction state: 616 + 40 = 656 bits.
- PatternID is 6 bits end-to-end; reconstructed physical addresses are 13 bits.

The width arithmetic is enforced by `static_assert` in the shared headers.

## Shared models

- `tb/n3_continuous_analysis/n3_rc_group_selector_oracle.hpp/.cpp`
  - represents the logical 16-record store without inflating the production 112-bit contract;
  - derives Config and action from SA role plus record slot;
  - enforces the frozen directional donor/release constraints;
  - rejects borrow requests without the required donor release;
  - searches the same canonical tuple order and reports Config, action, PatternID, borrow, release, and donor metadata.
- `tb/n3_continuous_analysis/n3_rc_reconstruction_oracle.hpp/.cpp`
  - consumes retained pivots and registered commit/Config/PatternID metadata;
  - uses the canonical 80-row fixed-mask table through the Layer-A loader;
  - independently produces line-valid, row/column identity, and full 13-bit address results.

Neither shared model defines `main()` or calls production RTL as its expected model.

## Affected tests

- `tb_n3_rc_group_selector_oracle.cpp`: exhaustive comparison of all 65,536 candidate-valid masks against the frozen selector RTL. All masks matched; the independent legality rule yields exactly 81 legal tuples, and an analyzer-valid but resource-illegal tuple is rejected.
- `tb_n3_rc_reconstruction_oracle.cpp`: directed RTL/oracle comparison proving seventh-pivot reconstruction, physical columns 1 versus 257, and the maximum 13-bit address 8191.
- `tb_n3_rc_group_full_top.cpp`: unchanged seed/corpus semantics (`0x4e335247`, T0/PatternID1); now obtains selector and reconstruction expectations from shared Layer-B models.
- `tb_n3_rc_group_high_pattern_full_top.cpp`: unchanged deterministic witness (`0x4e33524f`, witness 2474, PatternID 20); now links Layer A and Layer B normally and compares the full six-bit reconstruction against its low-four-bit alias.
- `tb_n3_rc_group_latency.cpp`: existing skeleton only. The prohibited test-source inclusion/main-macro workaround was replaced by normal Layer-A linkage and compile-checked. It was not executed.

The control and preemption tests contain no duplicated selector/reconstruction oracle and were not changed. Their regression is not required by this refactor.

## Regression evidence

The Verilator builds used the corresponding production top, the affected C++ test, and normal linkage to the applicable oracle `.cpp` files with the test directory on the include path.

Results:

```text
N3_RC_GROUP_SELECTOR_ORACLE_PASS valid_masks=65536 mismatches=0 legal_tuples=81 illegal_individual_combination_rejected=PASS
N3_RC_RECONSTRUCTION_ORACLE_PASS mismatches=0 physical_column_1_vs_257=PASS seventh_pivot=PASS address_13bit_max=8191 pattern_id_width=6
N3_RC_GROUP_FULL_TOP_LOCKSTEP_PASS vectors=1000 seed=0x4e335247 mismatches=0 repairable=1000 unrepairable=0 selected_config=T0:4 per vector selected_pattern=1:4 per vector physical_column_1_vs_257=PASS reconstruction_oracle_match=PASS
N3_RC_GROUP_FULL_TOP_LOCKSTEP_PASS vectors=10000 seed=0x4e335247 mismatches=0 repairable=10000 unrepairable=0 selected_config=T0:4 per vector selected_pattern=1:4 per vector physical_column_1_vs_257=PASS reconstruction_oracle_match=PASS
PATTERN_ID_GT_15_FULL_TOP_PASS witness_index=2474 capacity=3R3C config=0 pattern=20 binary=0b010100 analyzer=20 store_write=20 store_read=20 selector=20 registered=20 reconstruction=20 high_bits_preserved=PASS reconstruction_oracle_match=PASS truncated_alias_behavior_rejected=PASS
SEVENTH_PIVOT_DISTINCTION:PASS
HYBRID_GT_7_DISTINCTION:PASS
PATTERN_ID_GT_15_ANALYZER_WITNESS:PASS
```

The pre-refactor and post-refactor full-top runs preserve the same vector counts, seed, repairability counts, selected Config, and selected PatternID. The archived high-pattern evidence and the refactored run both select witness 2474 and PatternID 20. Therefore GROUP and reconstruction oracle semantic drift is `NONE`.

## Integrity and phase boundary

- Production RTL SHA-256 values are identical before and after Layer B.
- Shared helper main counts are zero; each test executable has exactly one `main()`.
- No test `.cpp` inclusion, main macro, weak main, or main alias remains under `tb/n3_continuous_analysis`.
- `git diff --check` passes.
- Generic vector generator: not created.
- Latency harness: existing skeleton only; not executed.
- Latency characterization, source freeze, DC, commit, push, and merge: not started.

