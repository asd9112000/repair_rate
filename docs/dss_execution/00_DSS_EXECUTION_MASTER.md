# DSS Thesis Execution Master

> 文件狀態：Current
> 適用範圍：DSS thesis execution control
> 建立時間：2026-09-13T06:28:23+08:00
> 最後修改時間：2026-09-13T06:28:23+08:00

## Execution boundary

```text
CURRENT_EXECUTION = DSS_THESIS_EXECUTION
FROZEN_BASELINE_PROJECT = DATE_2026_DSS_V2
FROZEN_BASELINE_PHASE = PHASE_4K

BASELINE_LAYOUT = 2x2
BASELINE_TOPOLOGY = Directional
BASELINE_MEMORY_IMPL = CAM
BASELINE_RS = 2
BASELINE_CS = 2
BASELINE_SHARE_M = 1
BASELINE_POLICIES = EARLY, GROUP-NoScratch

PRIMARY_NEW_TARGET = 2x2 Directional CAM, RS=3, CS=3, SHARE_M=1
PRIMARY_NEW_POLICIES = EARLY, GROUP-NoScratch
```

The DATE 2026 DSS V2 baseline is frozen evidence.  This execution extends DSS
V2 semantics; it does not restore legacy Phase-3 `ConfigPatternMap` as the
primary implementation.  Phase-3 and `dss_2x2` RTL may be considered as
implementation references only after a completed
`PRE_IMPLEMENTATION_REUSE_AUDIT`.

```text
DESIGN_GOAL = extensible implementation
VERIFICATION_REQUIREMENT = required research points only
REQUIRED_VERIFIED_POINTS = 2,2,1 and 3,3,1
```

This is not a commitment to arbitrary `Rs=1..N`, `Cs=1..N`, or `m=1..N`
support.  The governing principle is: **design for extensibility; verify the
required research points.**

## Authority and scope boundaries

Existing simulator authority remains in `docs/README.md`,
`DOCUMENT_INDEX.md`, `ARCHITECTURE.md`, `RECAM_SPEC.md`, `EXPERIMENTS.md`,
`REPORTS.md`, `EXPERIMENT_CATALOG.md`, and `HIERARCHICAL_RECAM.md`.  Those
documents define simulator behavior, experiment methodology, report scope, and
the group/device separation.  This directory controls only thesis-execution
authorization, cross-dependency gates, and new evidence accumulation.

Existing RTL authority remains in `docs_verilog/`, especially
`PHASE_STATUS.md`, `00_PROJECT_MASTER.md`, `02_PHASE_CONTROL.md`,
`01_COMPARISON_CONTRACT.md`, `DECISION_LOG.md`,
`PHASE3B_ANALYZER_INTERFACE.md`, `PHASE4E_TO_4K_MASTER_EVIDENCE.md`, and
`TSMC018_SYNTHESIS_FLOW.md`.  `docs_verilog/PHASE_STATUS.md` remains the RTL
project pointer and is not this workstream's status file.

## Simulator and artifact ownership

The existing simulator scopes are `legacy`, `group`, and `device`.  They remain
semantically separate.  The primary scope for DSS repair-rate,
EARLY-vs-GROUP, imbalance, and greedy-loss study is:

```text
PRIMARY_SIMULATOR_SCOPE = group
PRIMARY_SIMULATOR_FAMILY = DynamicSpareSharing
```

`HierarchicalRECAM` device repair rates are not substitutes for group-level DSS
results, and group/device repair rates must never be combined.

## Two-level evaluation and repair ownership roadmap

```text
LEVEL 1 — GROUP SCOPE
authority = DynamicSpareSharing
unit = one independent 4-SA repair group
questions = DSS repairability, EARLY versus GROUP-NoScratch, imbalance,
            borrowing, and greedy-loss behavior

LEVEL 2 — DEVICE SCOPE
authority = HierarchicalRECAM
unit = many repair groups in one modeled device
questions = persistent cross-group CAM occupancy, finite global CAM pressure,
            residual Tier-2 demand, and device repairability
```

The evidence domains are distinct.  Group samples do not retain online-CAM
occupancy across samples.  Device runs use one finite persistent global online
CAM pool whose occupancy accumulates across groups.  No result may average,
merge, or substitute group repair rates for device repair rates.

The intended repair ownership hierarchy is:

```text
Tier 0: local SA spare rows/columns
Tier 1: DSS spare-line borrowing within one four-SA group
Tier 2: finite persistent device-wide online CAM repair
```

Tier-1 borrowing changes physical spare ownership and availability inside its
group.  It is not Tier-2 CAM reuse.  Offline BIRA scratch may be cleared or
reused, but committed Tier-2 online-CAM assignments persist for the modeled
device.  Global online CAM is never per-SA, per-group, or reset after a group.

Future device evidence is roadmap-only until separately authorized.  Its
provenance must include device geometry, modeled groups, policy, global CAM
capacity/granularity, fault corpus, seed, command, and `cam_scope` equivalent
to `GLOBAL_LOGIC_DIE`.  Hardware area/GE/timing comes only from RTL synthesis;
device repairability, CAM occupancy, and overflow come only from device
simulation.

```text
C++ simulation = reports/
new DSS group experiments = reports/group/<experiment>/<run-id>/
RTL synthesis and cycle-accurate verification = results/
new execution control and evidence = docs/dss_execution/
```

An RTL value copied into simulator metadata must retain configuration, RTL
revision/commit, tool, library, corner, constraint, report path, and source
report.  Analytical C++ hardware proxies must not be written as RTL synthesis
fields.

## Experiment workflow boundary

`docs/EXPERIMENT_CATALOG.md` remains the workflow-navigation authority.  The
existing DATE-2X2 group workflow is:

```text
manifest = experiments/date_submission_2x2.json
multi-spare manifest = experiments/date_submission_2x2_spare_sweep.json
runner = scripts/group/date_submission/run.py
analyzer = scripts/group/date_submission/analyze.py
run root = reports/group/date_submission_2x2/<run-id>/
```

No DSS_X0 workflow is registered.  A future workflow may be registered only
after its manifest, runner, output schema, and source-of-truth CSV exist.

## Frozen implementation findings for future audit

The production V2 point is verified at `RS=CS=2, SHARE_M=1`, with four fixed
directional resources and single-line sharing.  The shared analyzer has a fixed
ConfigID set and frozen 2,2,1 sizing (`MAX_K=5`,
`MAX_ADDRESS_ENTRIES=5`, `MAX_HYBRID_ENTRIES=7`).  The older
`rtl/dss_2x2/analyzer/` family exposes parameters such as `RS`, `CS`,
`SHARED_ROWS`, `SHARED_COLS`, and `MAX_BORROWS`; that does not demonstrate
generic integrated-architecture support.

## Frozen project rule

```text
PRE_IMPLEMENTATION_REUSE_AUDIT = MANDATORY
```

No future RTL implementation phase may start from an abstract request.  It must
first inspect `rtl/dss_v2/`, `rtl/recam/`, `rtl/dss_2x2/analyzer/`, `tb/`,
`tests/`, `tests/golden/`, `scripts/simulation/`, `scripts/synthesis/`, and
relevant frozen documentation/evidence.  An incomplete audit means:

```text
RTL_IMPLEMENTATION_AUTHORIZED = NO
```

Scratch is audit-first work.  `A0` must establish a state-lifetime and retained
information benefit before a Scratch RTL phase can be proposed.
