# `recam_dss_v2_retained_analyzer_path` specification

## Intent

This isolated S1G-A2G path connects the 821-bit collector bank's exact
post-Must 232-bit view to the unchanged nine-slot canonical analyzer. It owns
no resource ledger, A-to-D scheduler, or provisional commit behavior.

## Timing

The collector update is clocked. View reconstruction and analyzer evaluation
are combinational from one coherent retained generation.

```wavedrom
{ "signal": [
  { "name": "fault_valid_i", "wave": "010" },
  { "name": "retained_state_o", "wave": "x3.", "data": ["old", "updated"] },
  { "name": "analyzer_view_o", "wave": "x3.", "data": ["old view", "updated view"] },
  { "name": "candidate_valid_o", "wave": "x3." }
] }
```
