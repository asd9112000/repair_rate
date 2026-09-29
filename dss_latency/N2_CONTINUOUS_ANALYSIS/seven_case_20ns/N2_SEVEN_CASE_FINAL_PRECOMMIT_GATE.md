# N2 seven-case CA-LIVE integrated final pre-commit gate

```text
NUMERICS_INTACT: PASS
RAW_RESULTS_INTACT: PASS
COPY_INTEGRITY: PASS
CA_NONCA_IDENTITY: PASS
HISTORICAL_RETENTION: PASS
CA_LIVE_CANONICAL_FAMILY: PASS
CA_LIVE_COPY_INTEGRITY: PASS
CA_LIVE_CLOSURE_HASHES: PASS
CA_SB_MARKED_HISTORICAL: PASS
PRIMARY_CA_POINTER: PASS
CODE_RESULT_PROVENANCE: PARTIAL
PROVENANCE_LIMITATION_DOCUMENTED: PASS
PROVENANCE_CONTRADICTION: PASS
DATASET_RESEARCH_USABLE: YES
TIMING_FIELD_TERMINOLOGY: PASS
CONSTRAINT_WORDING: PASS
LATENCY_BOUNDARY_DOCUMENTATION: PASS
MY_NOTE_EXCLUDED: PASS
LEGACY_RC_NONCA_ARTIFACTS_EXCLUDED: PASS
UNRELATED_SCRIPTS_AND_RESULTS_EXCLUDED: PASS
REPOSITORY_RULES_SATISFIED: PASS
FINAL_APPROVED_MANIFEST_ENTRIES: 95
FINAL_APPROVED_FILE_SET: 284
FINAL_EXTRA_STAGED_PATHS: 0
FINAL_MISSING_STAGED_PATHS: 0
READY_FOR_N2_CLOSURE_COMMIT: YES
```

The prior 215-file N2 closure remains staged without removal. This integrated
gate adds the exact 69-file CA-LIVE canonical-freeze scope, yielding an expected
284-file staged closure. The approved manifest lists every freeze file
explicitly; no broad repository staging is authorized.

The canonical family is `dss_final/continuous_analysis_live/`; its 12 copied
SystemVerilog files are byte-identical to the six LIVE_STATE working RTL roots,
and every closure SHA-256 manifest passes. The non-LIVE RC StateBank roots are
preserved as historical prototypes and are prohibited for final matched CA PPA.

The four newer R/L1X4 rows retain `SOURCE_PROVENANCE=PARTIAL` and
`SOURCE_HASH_MATCH=UNKNOWN`. Closure snapshot hashes prove source-copy
identity now; they do not claim historical synthesis-source hash equality.

Seven-case timing documentation distinguishes `Design WNS = 0.00 ns` from
positive critical-path slack, preserves the GROUP max-cap caveat, and retains
the six policy-specific latency measurement boundaries. No commit was performed.
