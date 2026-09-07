# RTL results boundary

This directory is reserved for RTL-derived measurements and their provenance. C++
repair-rate and architecture-model outputs remain under `reports/`.

Expected consolidated outputs:

- `synthesis_results.csv`
- `latency_results.csv`
- `module_area_breakdown.csv`
- `logs/`

Do not create zero-filled or header-only CSV files as placeholders. Every result row must be
traceable to the RTL commit, architecture/configuration, tool and version, technology
library or macro source, PVT corner, timing constraint, command/script, and raw log.
