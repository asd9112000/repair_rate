# Dynamic Spare Sharing Plot Generation

`plot_results.py` reads the dynamic simulator's `summary.csv` and generates
paper/PPT figures without changing the simulator or inventing missing
experiments.

There are now two distinct plotting workflows:

- `plot_results.py` analyzes policy/hardware/latency tradeoffs for one fixed
  `Rs/Cs` experiment set.
- `make analyze_dynamic_spare_sharing` generates and plots the
  `fault_count × symmetric spare-line × policy` comparison.

`plot_results.py` Figure 1 is a line plot over Uniform, Moderate, Strong, and
Hotspot distributions. It intentionally contains only No Sharing, Directional
`m=1`, Pairwise `m=1`, Pairwise `m=2`, and Global Pool `m=1`.

Generate its fixed-condition, four-model input and plot it with:

```text
./build/bin/DynamicSpareSharing 2 2 \
  --sweep --fault-count 20 --spatial mixed \
  --runs 10000 --seed 20260820 --paper-cam-reuse \
  --summary-only --output-dir reports/dynamic_spare_sharing/fault_models/raw

python3 plot_results.py \
  reports/dynamic_spare_sharing/fault_models/raw/summary.csv \
  --rs 2 --cs 2 --storage-mode cam \
  --plots-dir reports/dynamic_spare_sharing/fault_models/plots \
  --plot-data-dir reports/dynamic_spare_sharing/fault_models/plot_data
```

The repair-rate sweep keeps the original multi-panel
`plots/repair_rate_comparison.*` output and additionally creates one fault-count
line plot per spare count under
`plots/repair_rate_vs_fault_count_by_spare/`. Matching processed CSV files are
written under the same subdirectory in `plot_data/`.

## Run

From the repository root:

```text
python3 plot_results.py
```

The default input is `reports/dynamic_spare_sharing/summary.csv`. A different
input and output location can be selected explicitly:

```text
python3 plot_results.py path/to/summary.csv \
  --plots-dir path/to/plots \
  --plot-data-dir path/to/plot_data
```

The default experiment filter is `Rs=2`, `Cs=2`, and `storage_mode=cam`.
Use `--rs`, `--cs`, and `--storage-mode` only when the input contains a
matching experiment set.

## Outputs

- `plots/core`: repair-rate, gain/cost, utilization, and final tradeoff plots.
- `plots/complexity`: candidate, cycle, matrix, and CAM scaling plots.
- `plots/efficiency`: gain per additional hardware/latency cost.
- `plots/appendix`: P95, active/average, residual-fraction, and borrowing plots.
- `plots/diagnostic`: input validation, data-derived findings, and the isolated
  Global Pool m=2 result.
- `plot_data`: the exact processed rows used by each figure plus the two summary
  tables.

Every figure is saved as a 300-dpi PNG and vector PDF.  Repair gain uses
percentage points, not relative percent improvement.

## Global Pool m=2 safeguard

The `local=0/global=8` configuration remains visible in
`plots/diagnostic/global_pool_m2_diagnostic.*` and its processed CSV.  It is
excluded from publication plots by default because its zero repair rate has
not yet been verified.

For an explicit diagnostic run that includes it in the main plots:

```text
python3 plot_results.py --include-unverified-global-m2
```

This switch does not mark the result as verified; the exported metadata keeps
the `unverified_global_m2` field.

## Missing experiments

`plots/FUTURE_EXPERIMENTS_TODO.md` lists the fault-count/spare-count sweeps,
success-only residual-spare metrics, continuous imbalance metric, and SRAM
experiments that are required before the corresponding future plots can be
generated.  The plotting script does not create placeholder values.

## Recommended result layout

New repair-rate sweeps use one self-contained, non-overwriting run directory:

```text
reports/dynamic_spare_sharing/<run-id>/
├── run_config.txt
├── raw/
│   └── summary.csv
├── plot_data/
├── plots/
└── logs/
```

Treat `raw/summary.csv` and `run_config.txt` as the reproducibility record.
`plot_data/` and `plots/` are derived artifacts and may be regenerated. Logs
are diagnostic. `reports/dynamic_spare_sharing/` is ignored by Git; copy only
the selected final paper figures/tables into a deliberately versioned paper or
release directory.

The wrapper refuses to reuse an existing run directory, preventing one sweep
from silently mixing with another. Use `--run-id` for a meaningful stable
name or `--output-dir` for an explicit location. Use `--keep-run-details`
only when `attempts.csv`/`runs.csv` are actually needed.
