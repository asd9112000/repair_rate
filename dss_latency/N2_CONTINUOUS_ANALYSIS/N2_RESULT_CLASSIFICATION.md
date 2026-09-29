# N2 result classification

| Result family | Status | Primary use |
| --- | --- | --- |
| Historical RECAM 5-bit | SUPERSEDED_FOR_FINAL_MATCHED_PPA | history only; its logical column width was not physical identity |
| Corrected RECAM 13-bit | CANONICAL | final N2 baseline |
| Non-CA canonical EARLY/GROUP | VALID_HISTORICAL_CANONICAL_REFERENCE | CA-overhead baseline; not seven-case anchors |
| CA-StateBank | HISTORICAL_PROTOTYPE / BOUNDARY_MISMATCH | negative methodology evidence; duplicated 4x272-bit bank |
| CA-LIVE | VERIFIED | continuous-analysis functional/latency evidence |
| Seven-case 20 ns | PARTIAL_CANONICAL_DATASET | current matched main comparison |
| Remaining-four timing sweep | NOT_STARTED | future work |
| N3 CA | NOT_STARTED | future work |

No source or raw result is moved by this classification. Existing paths remain
the provenance authority. In particular, the non-CA 102,296.780 / 166,140.377
um² points are valid reference results, while the CA-LIVE 104,668.503253 /
171,399.415025 um² points are the matched seven-case anchors.
