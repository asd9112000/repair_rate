# RECAM_N2_2R2C

`CASE_ID: RECAM_N2_2R2C`

Canonical corrected physical-column RECAM baseline. The design is
combinational and retains no artificial final-result warehouse. Its four
synthesized sources are listed in `source_manifest.txt`; their hashes gate DC
before and after synthesis.

Run `synthesis/run_matched_dc.sh` for the 20 ns TSMC018 slow-corner flow.
The historical 5-bit baseline is intentionally preserved in `rtl/recam/` and
is not the canonical matched baseline.
