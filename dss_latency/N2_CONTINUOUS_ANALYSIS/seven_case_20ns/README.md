# Authoritative N2 seven-case matched 20 ns dataset

## Dataset identity and current gate

```text
DATASET_ID: N2_SEVEN_CASE_20NS_CA_LIVE_MATCHED
STATUS: PARTIAL
NUMERICAL_DATASET: COMPLETE
PROVENANCE: PARTIAL
PRECOMMIT_GATE: PENDING
CLOSURE_COMMIT: NOT_YET
```

Cases: `RECAM_N2_2R2C`, `G2X2_N2_RC_EARLY`, `G2X2_N2_RC_GROUP`,
`G2X2_N2_R_EARLY`, `G2X2_N2_R_GROUP`, `L1X4_N2_R_EARLY`, and
`L1X4_N2_R_GROUP`.

Hardware contract: `ROW_ADDR_W=9`, `PHYS_COL_ADDR_W=13`,
`WORD_COL_ADDR_W=5`, `HYBRID_LINE_ADDR_W=13`.

Synthesis: Synopsys DC W-2024.09-SP2; TSMC018 `slow.db`; slow corner; 20 ns;
matched canonical constraints; `NAND2X1=9.979200 um^2`.

**Identity rule:** G2X2-RC rows here are CA-LIVE matched implementations.
They are not the valid historical non-CA canonical points of 102,296.780 and
166,140.377 um². The latter remain a CA-overhead reference only.

All PPA entries use DC `Cell Area`, not `Design Area`. The three anchor checks
pass: RECAM `72116.352610 um^2`, G2X2-RC EARLY `104668.503253 um^2` / `+0.05
ns`, and G2X2-RC GROUP `171399.415025 um^2` / `+0.02 ns`.

`N2_SEVEN_CASE_20NS_PPA.csv` is the primary numerical table. Derived tables
point to raw reports through `N2_SEVEN_CASE_20NS_PROVENANCE.csv`.

## Result retention policy

1. Never overwrite raw synthesis reports.
2. Never delete superseded experiment results.
3. Canonical results retain source/report hashes where captured.
4. Historical and current results have explicit status tags.
5. Derived CSVs point to raw reports.
6. Plot/table generation consumes canonical CSVs, not copied numbers.
7. Newer numbers are not automatically final.
8. Final status requires provenance, gate, and commit.
