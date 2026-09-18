# `recam_dss_v2_retained_collector_bank` specification

## Intent

This isolated S1G-A2G module stores the frozen 821-bit historical collector
state for one SA. It accepts a bounded raw collector fault event, atomically
updates the complete record, derives validity/cfg/Must/count information, and
publishes the 232-bit post-Must analyzer view for a selected ConfigID.

## Reset and timing

`rst_ni=0` or `clear_i=1` clears the complete retained generation. An accepted
fault (`fault_valid_i && fault_ready_o`) updates the packed record and
increments `fault_generation_o` on the active clock edge. The same edge
invalidates old analysis. An analysis completion is accepted only when its
generation equals the retained fault generation.

```wavedrom
{ "signal": [
  { "name": "fault_valid_i", "wave": "010" },
  { "name": "fault_generation_o", "wave": "x3.", "data": ["N", "N+1"] },
  { "name": "analysis_valid_o", "wave": "10." },
  { "name": "retained_state_o", "wave": "x3.", "data": ["generation N", "generation N+1"] }
] }
```

## Frozen packing

The LSB-first record is exactly 821 bits: pivots `[102:0]`, reachable Hybrid
prefix `[260:103]`, reachable CAM-reuse prefix `[484:261]`, row counters
`[652:485]`, and column counters `[820:653]`. H11-H13 and R11 are unreachable
under `MAX_FAULTS=12` and do not occupy architectural storage.
