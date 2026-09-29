# Remaining-four N2 CA-LIVE structural final audit

The audit covers the four CA-LIVE top/core pairs named in
`provenance/SHA256SUMS`. It is a read-only source audit; no RTL was changed.

| Requirement | G2X2-R EARLY | G2X2-R GROUP | L1X4-R EARLY | L1X4-R GROUP |
| --- | --- | --- | --- | --- |
| `recam_shared_config_analyzer` instances in top | 1 | 1 | 1 | 1 |
| Canonical analyzer projection | 272 bits | 272 bits | 272 bits | 272 bits |
| New CA-LIVE payload-register bits | 0 | 0 | 0 | 0 |
| Snapshot / 1088-bit bank | absent | absent | absent | absent |
| Wide stored-state selection cone | absent | absent | absent | absent |
| Extra EARLY solution-payload register | absent | n/a | absent | n/a |
| Candidate store | n/a | 60 bits | n/a | 60 bits |
| Records per SA | n/a | 3 | n/a | 3 |
| RC 80-bit store / generation tags | n/a | absent | n/a | absent |

The 272-bit analyzer projection is the already-proven canonical payload:
`5 + 45 + 65 + 30 + 7 + 21 + 7 + 91 + 1 = 272`. The full field provenance
is in `../N2_CA_LIVE_ANALYZER_WIDTH_PROVENANCE.md`; CA-LIVE does not add a
payload field or payload register.

Both GROUP tops instantiate `dss_group_pivot_address_regs`, the existing
canonical 440-bit address reconstruction block. It is downstream of the
60-bit candidate-store selector and is not a copied live analyzer-state bank,
a snapshot, or a wide stored-state selection cone.

Result: **PASS**.
