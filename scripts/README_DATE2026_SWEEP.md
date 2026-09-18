# DATE2026 Repair-Rate Sweep Entry Point

Use one command and select an execution scope explicitly:

```bash
./scripts/run_date2026_repair_sweep.sh --scope group ...
./scripts/run_date2026_repair_sweep.sh --scope device ...
```

The common shell interface does **not** merge simulators or result semantics.
`group` runs four-subarray physical-spare sharing; `device` is reserved for a
future scheduler with device-wide state and a shared online CAM pool.

## Common usage

The dispatcher accepts `--scope group|device`. Group accepts the currently
defined shared parameters: `--rs`, `--cs`, `--share-m`, `--groups`, `--seed`,
`--policies`, `--topologies`, `--f-group-list`, `--output-root`, `--resume`,
`--dry-run`, `--preflight`, and `--formal`.

Only `--policies canonical` and `--topologies canonical` are defined for the
R3 Matrix V2 group policy set. The matrix contains both `m=1` and `m=2`
policies, so omit `--share-m` (or use the explicit
`--share-m canonical_m1_m2` selector). Other values fail rather than silently
selecting an experimental contract.

## Group-level sweep — VALIDATED

The group backend is `scripts/group/r3_formal_group_repair_rate.py`; the shell
adapter does not reimplement its corpus, policy, ledger, or metric logic.

```bash
./scripts/run_date2026_repair_sweep.sh \
  --scope group --rs 2 --cs 2 \
  --f-group-list 8 --groups 100 --preflight
```

For a formal point set:

```bash
./scripts/run_date2026_repair_sweep.sh \
  --scope group --rs 2 --cs 2 \
  --f-group-list 8,12,16,20 --groups 100000 \
  --policies canonical --topologies canonical --formal
```

The validated N3 formal invocation is separate because RS and CS remain an
equal, point-specific contract:

```bash
./scripts/run_date2026_repair_sweep.sh \
  --scope group --rs 3 --cs 3 \
  --f-group-list 12,18,24,30 --groups 100000 \
  --policies canonical --formal
```

Resume an interrupted root without regenerating complete policy sidecars:

```bash
./scripts/run_date2026_repair_sweep.sh \
  --scope group --rs 2 --cs 2 \
  --f-group-list 8,12,16,20 --groups 100000 --policies canonical \
  --formal --resume --output-root results/date2026/repair_rate/group/formal
```

The backend creates one fixed-total `multinomial_uniform` corpus per point,
materializes it, and replays it through the canonical policy matrix. It emits
per-policy raw sidecars plus repair-rate, paired, imbalance, and search
complexity aggregates. Group generic CAM fields remain distinct from device
CAM state: `cam_scope=group_generic`, 2x2 `cam_model=generic_cpp`, and 1x4
`cam_model=unavailable`/`NOT_PROVEN`.

## Device-level sweep — NOT_READY

The device interface reserves `--devices`, `--groups-per-device`,
`--banks-per-device`, `--cam-model`, `--cam-capacity`, and `--device-policy`.
It returns `DEVICE_NOT_READY` with exit status 3 and writes no artifacts. It
does not call the group simulator or reinterpret group generic CAM fields as a
device shared pool.

A future device manifest must use `scope=device`,
`cam_scope=device_shared_pool`, and include device count, groups per device,
bank/stack organization, CAM model/capacity, and allocation policy.

## Resume behavior

For group scope, `--resume` validates and reuses a policy sidecar only when it
has exactly the requested number of contiguous `group_id` records. Resume
granularity is `(RS, CS, F_GROUP, policy, topology)`. Incomplete files are not
treated as complete. The output root must already contain an R3 manifest.

Device resume is reserved separately; it must eventually include the device
configuration and shared-CAM configuration in addition to group fields.

## Output structure

New wrapper-managed outputs default below:

```text
results/date2026/repair_rate/
  group/{preflight,formal,custom}/
  device/{preflight,formal,custom}/
```

Each group experiment root contains `manifest/`, `corpus/`, `raw/`,
`aggregate/`, `plots/`, and `logs/`. The in-progress original R3 result root
is retained at `r3_formal_group/` and is not moved while its formal run is
active.

## R3 derived group figures

The R3 analysis/plot stage is downstream of the runner and reads only derived
CSV files from an explicit analysis root. It neither changes runner raw
sidecars nor reinterprets group results as device data:

```bash
python3 scripts/plot/r3_group/plot_group_repair_rate.py \
  --analysis-root <analysis-root>
python3 scripts/plot/r3_group/plot_fault_imbalance.py \
  --analysis-root <analysis-root>
```

The repair-rate source is `data/repair_rate_summary.csv`. The imbalance source
is `data/imbalance_summary.csv`; its plotter pools `(RS, policy, imbalance_bin)`
and uses total successes divided by total groups. Figures are written below
`figures/repair_rate/<comparison>/` and
`figures/imbalance/<comparison>/`, not directly under `figures/`. Each of the
five views (`all`, `topology_2_2`, `topology_1_4`, `policy_early`, and
`policy_global`) has a combined Rs2/Rs3 figure and Rs2/Rs3 single-panel
figures, each in PDF/SVG/PNG. The full policy membership and current dataset
classification requirements are authoritative in
[`docs/EXPERIMENTS.md`](../docs/EXPERIMENTS.md#71-r3-group-derived-data-figures).

## Manifest fields and policy presets

Group manifests record `scope=group`, backend/hash, git revision and dirty
state, fixed-total fault contract, seed mapping, RS/CS/share-m, point set,
canonical policy matrix, sample count, and CAM provenance. The canonical preset
uses explicit per-N membership: N2/N3 contain 12 policies, including the
directional m1 V2 ladder EARLY / GROUP_GREEDY / V2_GROUP_GLOBAL; N4 contains
9 policies and deliberately excludes that ladder because no frozen N4 V2
ConfigID/candidate contract exists.  The retained historical
`directional_m1_group_global` is `GENERIC_GROUP_GLOBAL_LEGACY` and is not a
paper-canonical Matrix V2 member.  The runner records this policy descriptor,
candidate contract, search scope, backtracking flag, and N support in the
manifest and sidecar outputs; it never substitutes generic GLOBAL at N4.

## Known limitations

- Group and device repair-rate results are not interchangeable.
- Device execution is intentionally not implemented by this wrapper.
- Historical 2x2 GROUP-GREEDY remains a cross-contract comparison to generic
  GROUP-GLOBAL, but it is outside the Matrix V2 canonical sweep.
- 1x4 CAM accounting is not proven and is not a PPA result.
