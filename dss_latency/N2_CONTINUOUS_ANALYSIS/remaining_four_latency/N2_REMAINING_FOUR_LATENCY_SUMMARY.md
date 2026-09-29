# Remaining-four N2 CA-LIVE latency evidence

This archive keeps the four N2 remaining-case measurements separate from the
existing G2X2-RC CA-LIVE experiments. Every result uses the deterministic
10,000-vector corpus identified in `provenance/CORPUS_IDENTITY.md`.

| Case | Measured boundary | Samples | Min / mean / median / p95 / max | Functional crosscheck |
| --- | --- | ---: | --- | --- |
| G2X2-R EARLY | state update to commit | 5,034 | 1 / 1.71812 / 1 / 4 / 4 | 10,000 / 10,000 PASS; 0 mismatches |
| L1X4-R EARLY | state update to commit | 4,311 | 1 / 1.46254 / 1 / 2 / 4 | 10,000 / 10,000 PASS; 0 mismatches |
| G2X2-R GROUP | final-SA update to solution-ready | 10,000 | 5 / 5.5 / 5 / 6 / 6 | 10,000 / 10,000 PASS; 0 mismatches |
| L1X4-R GROUP | final-SA update to solution-ready | 10,000 | 5 / 5.5 / 5 / 6 / 6 | 10,000 / 10,000 PASS; 0 mismatches |

The exact GROUP histogram is measured, not inferred from its mean:

| Case | 5 cycles | 6 cycles | Arithmetic |
| --- | ---: | ---: | --- |
| G2X2-R GROUP | 5,000 (0.500000) | 5,000 (0.500000) | 5,000 + 5,000 = 10,000 |
| L1X4-R GROUP | 5,000 (0.500000) | 5,000 (0.500000) | 5,000 + 5,000 = 10,000 |

The two observed bins reproduce the stored aggregate for both GROUP cases:
minimum 5, mean 5.5, median 5, p95 6, and maximum 6. There are no other
latency bins. See `GROUP_LATENCY_DISTRIBUTION.csv` for machine-readable
counts and `EARLY_CONFIG_RANK_DISTRIBUTION.csv` for the separately measured
EARLY rank distribution.
