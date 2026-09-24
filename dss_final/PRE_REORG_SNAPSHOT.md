# N2 FINAL archive pre-reorganization snapshot

## Purpose

This checkpoint preserves the exact N2 FINAL state before archive and
documentation reorganization. It is a documentation boundary, not a new RTL
or PPA result.

## Base authoritative commit

`6351a82479ad6e95ed3e02cd441c9fe96cc6f462`

`archive: finalize corrected N2 RECAM and DSS hardware set`

## Canonical state

- Canonical cases: **7 / 7 FINAL**.
- Canonical RECAM: `RECAM_N2_2R2C`.
- Canonical EARLY: three streaming immediate-commit implementations.
- Canonical GROUP: three delayed-decision retained-state implementations.
- Authoritative final PPA: `dss_final/N2_HARDWARE_SUMMARY.md`.

## Common address contract

```text
ROW_ADDR_W         = 9
PHYS_COL_ADDR_W    = 13
WORD_COL_ADDR_W    = 5
HYBRID_LINE_ADDR_W = 13
```

## Source-manifest fingerprints

These SHA-256 values are hashes of each working case's existing
`source_sha256.txt`. They provide the no-synthesizable-RTL-change gate for the
following documentation reorganization.

| Case | `source_sha256.txt` SHA-256 |
|---|---|
| `RECAM_N2_2R2C` | `4a592569b27734b8cee665c46aadc94e33f6d39bd82b308987a7e03fb96e25d0` |
| `G2X2_N2_RC_EARLY` | `96f74ebc109342e1beee92500aad0455e3e17d036ce28ff935b3754425646d5e` |
| `G2X2_N2_RC_GROUP_reg` | `5e9b03e2d1d78b2e510b207410c03355f7e39f9b5a3b58829d387e1de646da4e` |
| `G2X2_N2_R_EARLY` | `8cbe43bf74a2a26b362d2dac6522a7acc6576ae08b365da964d9ae0d007f07da` |
| `G2X2_N2_R_GROUP_reg` | `981308404df2a24761d9f71afd6c8b8f2d3852096da59f09d0f88914667a88fb` |
| `L1X4_N2_R_EARLY` | `05fe39d5e099aa0cd64ada938f5975079031ace7e1fb0b06572db905732c65ff` |
| `L1X4_N2_R_GROUP_reg` | `b3118fea111f29eceb42844cec1e6bc7a134203d8aa340f401ec1e10ce356b91` |

The historical 5-bit RECAM source remains preserved under `rtl/recam/`; its
final matched replacement is `RECAM_N2_2R2C`.
