# Measurement replay run manifest

Classification: `MEASUREMENT_REPLAY_WITH_FUNCTIONAL_CROSSCHECK`.

The authorized GROUP replay compiled each full-top wrapper with the same
canonical and CA-LIVE source manifests used for functional closure. The only
instrumentation change is `tb_ca_latency_group.cpp`, which counts measured
final-SA-update-to-solution-ready latency bins and checks that all 10,000
samples are in the 5- or 6-cycle bins.

| Case | Top | Corpus | Functional replay | Histogram |
| --- | --- | --- | --- | --- |
| G2X2-R GROUP | `tb_g2x2_r_group_live_full_top` | 10,000, `0x20260928` | 10,000 / 10,000 PASS; 0 mismatches | 5: 5,000; 6: 5,000 |
| L1X4-R GROUP | `tb_l1x4_r_group_live_full_top` | 10,000, `0x20260928` | 10,000 / 10,000 PASS; 0 mismatches | 5: 5,000; 6: 5,000 |

RTL_SHA_BEFORE: the eight CA-LIVE RTL source checksums listed in
`SHA256SUMS`.

RTL_SHA_AFTER: the same eight CA-LIVE RTL source checksums listed in
`SHA256SUMS`.

RTL_SHA_MATCH: `PASS`.

RTL changed: `NO`. Canonical RTL, analyzer RTL, candidate-store RTL,
selector/path RTL, and synthesis sources were not modified. Build products
were isolated under `/tmp/ca_latency_*_group_hist_abs`.
