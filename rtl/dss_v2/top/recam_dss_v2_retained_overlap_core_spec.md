# `recam_dss_v2_retained_overlap_core` specification

## Purpose

This isolated S1G-A2G controller schedules one nine-slot analyzer across
SA A, B, C, and D.  It uses four independently coherent 821-bit retained
collector banks and commits only a candidate evaluated for the bank's latest
fault generation.  It does not alter the baseline EARLY or GROUP controllers.

## Decision and generation contract

- A accepted fault is written atomically to precisely one selected SA bank.
- A bank's fault generation increments on that write and invalidates its
  previous analysis result.
- On a simultaneous selected-SA update and analyzer completion, the update
  wins; the controller restarts its four-role sweep.
- Ledger input is asserted only at final A-to-D commit.  No provisional
  analyzer operation writes or rolls back ledger state.

```wavedrom
{ signal: [
  { name: 'fault_valid_i (selected SA)', wave: '010...' },
  { name: 'fault_generation',            wave: '3.4...' },
  { name: 'analysis_valid',              wave: '0.1...' },
  { name: 'owner role sweep',            wave: '3.4.5.6' },
  { name: 'commit (only matching gen)',  wave: '0...10' }
] }
```

## Interface conventions

`fault_valid_i` and `fault_sa_i` describe an isolated collector event, not a
BIST integration interface.  `test_done_valid_i` records the pre-existing
test-complete contract.  `fault_ready_o` is high only while the transaction is
active and the addressed retained bank has capacity.
