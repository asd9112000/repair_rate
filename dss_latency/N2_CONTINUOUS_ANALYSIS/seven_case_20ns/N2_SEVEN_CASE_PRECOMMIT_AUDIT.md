# N2 seven-case pre-commit Closure-A audit

```text
NUMERICS_INTACT: PASS
RAW_RESULTS_INTACT: PASS
CA_NONCA_IDENTITY: PASS
HISTORICAL_RETENTION: PASS
CODE_RESULT_PROVENANCE: PARTIAL
PROVENANCE_LIMITATION_DOCUMENTED: PASS
COMMIT_SCOPE_ISOLATED: PARTIAL
REPOSITORY_RULES_SATISFIED: PARTIAL
STATUS_SYNC: PASS
DATASET_RESEARCH_USABLE: YES
READY_FOR_COMMIT: NO
```

The four protected numerical CSV checksums verify, seven PPA/provenance rows
exist, anchors match raw reports, and no raw report was modified in Closure-A.
The four remaining R/L1X4 rows are P3 PARTIAL provenance as documented in
`provenance/N2_SEVEN_CASE_PROVENANCE_LIMITATIONS.md`.

Commit scope remains partial because pre-existing tracked `my_note/` deletions
and multiple unrelated repair-rate scripts/results have not been assigned by
the user. No file was staged, committed, reset, or deleted during this phase.
