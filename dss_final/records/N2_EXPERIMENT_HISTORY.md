# N2 experiment history

1. **NON-CA canonical.** Corrected 13-bit physical-column RECAM and six DSS
   cases established valid non-CA canonical reference hardware.
2. **CA-StateBank prototype.** CA_SB captured four 272-bit analyzer-input
   snapshots per SA path.
3. **State-duplication audit.** The prototype had duplicated analyzer state, a
   wide selector, and a mismatched PPA boundary.
4. **CA-LIVE architecture.** Authoritative upstream state drives a single
   analyzer through the 272-bit live active-SA interface.
5. **Matched verification.** Functional/oracle or C++/RTL checks, latency
   replays, and matched 20 ns synthesis evidence were collected.
6. **Seven-case dataset.** One RECAM baseline plus six CA-LIVE DSS rows were
   assembled in `seven_case_20ns/`.
7. **CA-LIVE canonical freeze.** The six verified working RTL roots were
   byte-copied with closure hashes into `continuous_analysis_live/`.

CA_SB was not adopted because it used four 272-bit duplicated analyzer-input
snapshots, a wide state selector, and a synthesis-boundary mismatch. CA-LIVE
was adopted because it uses live upstream authoritative state, one analyzer,
no duplicate analyzer-state bank, a matched boundary, functional verification,
and small matched PPA overhead.
