# Phase 3C Functional Verification

## Command

```bash
bash scripts/simulation/run_recam_phase3c_functional.sh
bash scripts/simulation/run_recam_phase3b_functional.sh
make test_recam_paper_faithful
```

## Result

All commands pass.  The Phase 3C directed regression verifies:

- A/D emits only `CFG0 → CFG4 → CFG5 → CFG6`.
- B/C emits only `CFG0 → CFG2 → CFG1 → CFG3`.
- The ConfigID and full combinational result observed in each scan interval are
  captured into that exact map entry.
- A multi-valid candidate bitmap is retained intact rather than reducing it to
  the selected PatternID.
- A deterministic stable-input search produces a scan with both repairable and
  unrepairable records, and all four status bits are retained independently.
- A completed A/D transaction followed by B/C clears/replaces the map; no A/D
  ConfigID leaks into the B/C map.
- `start_i` while busy is ignored, including a simultaneous role-input change.
- `done_o` is a one-cycle pulse after the fourth capture.

The integration regression confirms Phase 3A/Phase 3B CFG0 equivalence over
2,008 vectors, CFG0–CFG6 directed coverage, MAX_K=5, Hybrid entry[6],
asymmetric 27-bit-address packing, transpose, threshold handling, and line
expansion behavior remain passing.  `make test_recam_paper_faithful` also
passes.

Functional verification preceded synthesis.  The subsequently authorized 20 ns
synthesis record is [PHASE3C_SYNTHESIS_20NS.md](PHASE3C_SYNTHESIS_20NS.md);
the functional contract and results above are unchanged.
