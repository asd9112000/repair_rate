# RTL simulation scripts

This directory is for HDL/RTL simulation automation only. C++ system-level simulator runners
remain in `scripts/legacy/`, `scripts/group/`, and `scripts/device/`.

RTL simulation artifacts belong under `results/logs/` or another documented `results/`
subdirectory; they must not be mixed into C++ `reports/`.

Phase 3B shared-analyzer lint plus functional/equivalence regression:

```bash
bash scripts/simulation/run_recam_phase3b_functional.sh
```

The command intentionally exits nonzero if the frozen Phase 3A CFG0 reference
and Phase 3B/C++ semantics disagree. It does not run synthesis.
