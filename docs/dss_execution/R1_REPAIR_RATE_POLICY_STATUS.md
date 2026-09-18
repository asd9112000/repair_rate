# R1 Repair-Rate Policy Status

## Gate

`R1_STATUS: PARTIAL` — R1A 2×2 ConfigID/corpus closure is complete; R1B 1×4
policy implementation has not started. No formal sweep is authorized.

| Gate | Status |
|---|---|
| RTL GROUP rank documented (`1,0,3,2`) | PASS |
| canonical greedy control semantics | PASS |
| historical V2 behavior preserved | PASS |
| C3 search/objective/tie-break understood | PASS |
| C3 full-group success criterion | PASS |
| same-corpus group replay | PASS |
| required focused regressions | PASS |
| versioned active-RTL ConfigID contracts | PASS; frozen 2×2 and RS3 are separate |
| RS3 RTL lockstep | PASS; 16 directed + 1,000 random vectors |
| GROUP-GLOBAL PatternID oracle | PASS; directed + 128 randomized states |
| versioned paired-policy raw schema | PASS; `dss_paired_corpus_v1` sidecars |
| formal repair-rate sweep | NOT RUN |

Do not start R2/R3 until R1B topology-specific 1×4 policy work is completed
and its paired-policy validation is available.
