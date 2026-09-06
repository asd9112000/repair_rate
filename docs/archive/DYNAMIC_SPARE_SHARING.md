# Dynamic Spare Sharing Simulator

> 文件狀態：Archive
> 適用範圍：legacy-group
> 建立時間：Unknown
> 最後修改時間：2026-09-02T00:00:00+08:00
> 本文件權威主題：歷史版 DynamicSpareSharing 說明

## Scope

`DynamicSpareSharing.cpp` is the new entry point for the dynamic sharing
specification. `SharedLine.cpp` remains the legacy directional-sharing
reference and must not be silently changed to match the new model.

The simulator now supports the no-sharing baseline, directional sharing,
group-level global pools, and pairwise edge sharing through the same
`RECAMSolverAdapter`. `SharedLine.cpp` is retained only as the locked legacy
reference.

## Confirmed definitions

1. `fault_count` is the total fault count across A/B/C/D for one run.
2. Local-first, minimum reserve, and single-dimension borrowing are policy
   modifiers, not standalone physical topologies.
3. `repair_after_1/2/3_borrows` uses a group-wide borrow budget.
4. Directional Policy 1 uses A-to-C rows, D-to-B rows, B-to-A columns, and
   C-to-D columns.
5. Multiple successful choices are ordered by repair success, minimum borrow,
   minimum cycles, then minimum candidate index.
6. `hybrid_CAM_entry_width_bits` is configured directly. It is not derived
   from data/address widths without a specified sizing equation.

## Foundation types

- `SimulationConfig` keeps physical spare parameters, effective sharing
  parameters, storage mode, latency parameters, fault configuration, and
  policy modifiers separate.
- `RepairAttemptResult` is one subarray RECAM analysis at one effective
  `(R_available,C_available)` configuration. It retains a pointer-free
  `CandidateRepairOption` for every matrix-valid candidate, including actual
  used row/column counts; the foundation's selected candidate remains the
  legacy `validSolList.front()`.
- `GroupRepairResult` owns all four subarrays' attempts and the canonical
  group-wide selected borrow count.
- `RECAMSolverAdapter` creates a fresh `FaultList` and `RECAM_PE` for every
  call, preserves input fault order, and copies only pointer-free result data.

Address-CAM peak usage equals its monotonic active-entry high-water mark.
Hybrid-CAM peak usage is instrumented at the allocation boundary, before any
must-rule reclamation, so it is not confused with final active usage.

## Physical ledger and selection

Every physical spare line has one owner, one dimension, one physical ID, and
at most one current assignee. The ledger assigns an owner's non-shareable
lines first and then lends only an unused shareable line. Borrowing never
increases the physical total.

- Directional: A-to-C and D-to-B rows; B-to-A and C-to-D columns.
- Global: group-owned rows/columns may be assigned to any subarray.
- Edge: rows only across A/C or B/D; columns only across A/B or C/D.

Successful group choices are ordered by actual borrowed-line count, selected
attempt cycles, then lexicographic A/B/C/D candidate index. The final tie is
the attempt index. Candidate capacity is not charged as physical usage;
actual pointer-free remap snapshots determine used R/C counts.

Local-first analyzes four local attempts and stops on local group success.
Otherwise it adds only the next group borrow-budget stage. Minimum reserve is
enforced when lending, and single-dimension mode rejects a subarray plan that
requires both borrowed rows and borrowed columns.

## Runner and CSV output

Build and run a single configuration:

```text
make dynamic_sharing_b
./build/bin/DynamicSpareSharing 2 2 \
  --topology directional --shared-lines 1 --local-first \
  --fault-count 20 --runs 10000 --seed 20260820 \
  --hybrid-cam-entry-width-bits 64 \
  --output-dir reports/dynamic_spare_sharing/manual_run
```

CAM reuse is the default buffer mode: each configuration provisions `Rs+Cs`
buffer entries. Use `--buffer N` only for an explicit fixed-capacity run, such
as `--buffer 2` when reproducing the locked legacy `SharedLine.cpp` baseline.
`--buffer` and `--paper-cam-reuse` are mutually exclusive; the latter remains
available as an explicit, self-documenting form of the default.

Using `--fault-file` preserves the file's fault ordering; when
`--fault-count` is omitted, the runner derives and verifies the constant
A/B/C/D group total. Output contains:

- `attempts.csv`: every RECAM attempt and complete valid candidate list;
- `runs.csv`: one policy result per run, including borrowing, utilization,
  CAM/matrix/candidate/cycle metrics;
- `summary.csv`: repair rate, averages, peaks, P95 values, and repair-gain
  efficiency proxies.

### Remap-table integration output

For downstream address-remap integration, a single policy configuration can
also emit the same full/simplified log contract used by `SharedLine_SRAM`:

```text
./build/bin/DynamicSpareSharing 2 2 \
  --simplified-fault-file fault_generator/faults_simplified.faults \
  --topology edge --shared-lines 1 --local-first \
  --write-remap-tables \
  --output-dir reports/dynamic_spare_sharing/integration_run
```

`--simplified-fault-file` directly accepts one flat address per line:

```text
HBMID ChannelID BankID SubarrayGroupID SubarrayID Row Col
```

Blank lines and lines beginning with `//` or `#` are ignored. Physical groups
are processed in lexicographic `(HBMID, ChannelID, BankID, SubarrayGroupID)`
order, while source fault order within each subarray is preserved. The
original integrated fault generator remains the default when neither
`--fault-file` nor `--simplified-fault-file` is supplied.

`--write-remap-tables` writes:

- `RemapTable.txt`: header plus one canonical, physical-ledger-feasible
  `OPTION` for every successful four-subarray group, and a
  `#FAILED SUBARRAY GROUP` record for every failure;
- `RemapTable_simplified.txt`: only the corresponding `MAP`, exact-address
  `BUFFMAP`, and failure records.

The emitted option is the exact four-subarray candidate combination selected
by the dynamic allocator, so `runs.csv` and the executable remap description
cannot disagree. `BUFFMAP` retains all seven source-address fields and uses
the CAM-reuse latency. Remap output is intentionally unavailable in `--sweep`
and `--repair-rate-sweep`; run one selected policy configuration to avoid
mixing different architectures in one runtime remap table.

The reported unitless/bit-oriented hardware proxy is explicit rather than
implicit:

```text
address_entries_provisioned * (row_address_width + column_address_width)
+ hybrid_CAM_bits_provisioned
+ buffer_entries_provisioned *
    (row_address_width + column_address_width + data_width)
+ provisioned_matrix_cells
```

Latency uses the configured storage insert/lookup/read costs, one operation
per fault for insert and lookup, one read per final active CAM entry, plus the
configured per-cell, per-candidate, and per-borrow-request costs. Failed runs
have no committed spare utilization; their unsuccessful request/starvation
counters and analysis cycles are still reported.

`--sweep` generates deterministic faults for uniform, moderate imbalance,
strong imbalance, and hotspot counts, then runs no-sharing plus Directional,
Global, and Edge policies for shared-line settings 0, 1, and 2. The default
run count is 10,000. Fault generation uses a specified integer PRNG/mapping,
so it does not depend on `std::uniform_int_distribution` behavior.

### Fault-count x spare-line comparison

`--repair-rate-sweep` is a separate sweep mode for the same comparison shape
used by the legacy SharedLine analysis. It varies total A/B/C/D `fault_count`
and symmetric `Rs=Cs`, while reusing one deterministic fault-map set across
all spare counts and policies at a given fault count. The representative
policy set is:

```text
No Sharing
Directional m=1, m=2
Pairwise m=1, m=2
Global Pool m=1
```

Global Pool m=2 remains outside this publication sweep until its fully-global
`local=0/global=8` behavior is verified. A direct runner example is:

```text
./build/bin/DynamicSpareSharing 2 2 \
  --repair-rate-sweep \
  --fault-min 8 --fault-max 16 --fault-step 4 \
  --spare-min 2 --spare-max 6 \
  --fault-model moderate --spatial mixed \
  --runs 100 --seed 20260820 --local-first \
  --hybrid-cam-entry-width-bits 64 \
  --summary-only \
  --output-dir path/to/raw
```

The generated `summary.csv` includes `fault_count`. `--summary-only` is
recommended for grid sweeps because `attempts.csv` and `runs.csv` can become
very large; omit it only when run-level debugging is required.

`--fault-step` defaults to `1` in the C++ runner for backward compatibility.
The analysis wrapper defaults to total fault counts `16..64` with step `4`, so
it runs `16,20,...,64`. With the `uniform` count model, each step adds exactly
one fault to every subarray; imbalance models redistribute each selected total
according to their weights.

The end-to-end wrapper builds, simulates, plots, and records a manifest:

```text
make analyze_dynamic_spare_sharing \
  ARGS="--fault-model moderate --runs 1000"
```

It produces `repair_rate_comparison`, repair-rate heatmaps, absolute
percentage-point improvement heatmaps, and relative-improvement heatmaps as
both PNG and PDF.

## Legacy tracking contract

The regression fixture and expected results are documented in
`tests/golden/dynamic_sharing_seed_20260820_manifest.txt`.

For `Rs=Cs=2`, `sharedLine=1`:

- A/D candidates: `(2,3)`, `(1,3)`, `(2,2)`, `(1,2)`.
- B/C candidates: `(3,2)`, `(2,2)`, `(3,1)`, `(2,1)`.
- The all-local `(2,2)` combination is legacy configuration index `150`,
  corresponding to option digits `(2,1,1,2)`.

The golden regression verifies:

- every subarray's fault classification counts;
- CAM overflow and final repair result;
- complete zero-based `validSolList` and first selected candidate;
- adapter equivalence for all 512 legacy subarray/configuration attempts,
  including each valid candidate's actual used R/C and mapping signature;
- all successful configuration indices from the 256-way legacy SharedLine
  search, including exact reproduction of configuration 150.

The test also fingerprints `SharedLine.cpp` itself. A legacy source edit must
be reviewed and the golden deliberately recaptured before tests can pass.

## `sharedLine=2` boundary

The existing Hybrid-CAM equation is unchanged for positive R/C. When exactly
one effective axis is zero, its descriptor capacity is explicitly zero,
avoiding the legacy negative signed expression. A `(0,0)` local Global-pool
attempt is represented without constructing RECAM; it succeeds only for an
empty fault list. Both cases have dedicated regression coverage.

## Verification

`make test` includes physical conservation, unique-line allocation, confirmed
directional ownership, Global accounting, no-diagonal Edge transfers,
minimum reserve, single-dimension borrowing, local-first attempt reduction,
candidate `choose(R+C,R)`, component cycle sums, deterministic fault/CSV
output, the 10/32 golden baseline, and the direct legacy RECAM regression.
