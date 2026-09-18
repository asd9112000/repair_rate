# P3-BL-RTL-A — latency accounting

The producer has sixteen sequential analyzer capture cycles. SEARCH starts only after its complete raw map is retained. Candidate visits remain a search-work metric and are not relabeled as cycles.

| Segment | Cycles / rule |
|---|---|
| Accepted start to producer request | registered top-control handoff |
| Candidate generation | 16 fixed capture cycles |
| Map-ready to search start | registered handoff |
| GLOBAL search | data-dependent bounded DFS cycles |
| Search done to commit start | registered handoff |
| Commit | 5 fixed cycles: stage A/B/C/D then publish |
| Commit acceptance to done | one registered top observation cycle |

The integrated directed success test reports the measured total below; the all-invalid-C negative case is intentionally search-length dependent for OPT0 and remains a functional, not latency, comparison.

```text
CANDIDATE_PRODUCTION_REQUESTS: 16
CANDIDATE_GENERATION_CYCLES: 16
COMMIT_CYCLES: 5
INTEGRATED_DIRECTED_SUCCESS_CYCLES: 34
INTEGRATED_DIRECTED_FAILURE_CYCLES: 13192 (C snapshot overflow; OPT0 exhaustive failure path)
```
