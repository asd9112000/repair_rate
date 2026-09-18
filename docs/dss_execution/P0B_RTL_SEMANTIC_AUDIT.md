# P0-B — Existing 2×2 RTL Semantic Audit

> Status: complete, read-only audit
>
> Scope: frozen 2×2 directional, `m=1`, decision/resource-management
> boundary only.  This audit does not change RTL, run synthesis, regenerate
> group-level data, or claim final repair-table reconstruction.

## Decision

```text
OLD_RTL_EARLY_NORMALIZED_MAPPING: NO_EXACT_CANONICAL_MAPPING
OLD_RTL_GROUP_NORMALIZED_MAPPING: EARLY
RTL_REUSE_DECISION:
  historical/V2 EARLY       = CONTROLLER_ONLY_UPDATE
  historical/V2 GROUP       = EXACT_REUSE for normalized EARLY boundary
  normalized GLOBAL          = RTL_REWORK_REQUIRED (no existing RTL policy)
```

The historical names are misleading under the P0 taxonomy.  The historical
`EARLY` policy is not normalized `EARLY`; it is a sequential local-first-like
policy whose B/C priority is `LOCAL, BORROW_ONLY, RELEASE_ONLY,
RELEASE_AND_BORROW`.  It therefore also is not the frozen normalized
`LOCAL_FIRST` policy, whose directional V2 contract uses the same canonical
slot order for every SA.  The first mismatch is a B or C instance with both
local and release-only candidates legal: historical `EARLY` commits local,
whereas normalized `EARLY` commits release-only.

Historical `GROUP` and V2 `GROUP-NoScratch` use the release-preserving order
`RELEASE_ONLY, LOCAL, RELEASE_AND_BORROW, BORROW_ONLY`, then commit the first
physically legal candidate in A→B→C→D order.  That is the normalized
directional `EARLY` behavior (`group_greedy_rtl_canonical`), not normalized
`GLOBAL`.

## Audit basis and provenance

The current repository `HEAD` is `3304c86acd7e8114033e684acd721bf6daace57c`
(`DATE 2x2 direction rtl done`).  This is an audit observation, not a claim
that every older report recorded that revision.

| Point / historical label | Authoritative source top and source manifest | Parameter point | Result provenance / revision status |
|---|---|---|---|
| Phase 3J `EARLY` | `rtl/recam/recam_phase3j_full_dss_top.sv`, top `recam_phase3j_full_early_top`, `scripts/synthesis/dc/recam_phase3j_sources.tcl` | `Rs=Cs=2`, directional `m=1`, `GROUP_POLICY=0` | `results/phase3j/dc_tsmc018_slow/phase3j_full_early_20ns`; its metadata has no RTL Git SHA: `NOT_FROZEN` for exact revision. |
| Phase 3J `GROUP` | same Phase 3J source, top `recam_phase3j_full_group_top`, `GROUP_POLICY=1` | `Rs=Cs=2`, directional `m=1` | `results/phase3j/dc_tsmc018_slow/phase3j_full_group_20ns`; metadata has no RTL Git SHA: `NOT_FROZEN` for exact revision. |
| V2 Phase 4E `EARLY` | `rtl/dss_v2/top/recam_dss_v2_early_top.sv`, `recam_dss_v2_early_core.sv`, `scripts/synthesis/dc/recam_phase4e_sources.tcl` | `Rs=Cs=2`, directional `m=1` | `results/phase4e/dc_tsmc018_slow/v2_specialized_early_20ns`; metadata has no RTL Git SHA. |
| V2 Phase 4F `GROUP-NoScratch` | `rtl/dss_v2/top/recam_dss_v2_group_top.sv`, `recam_dss_v2_group_core.sv`, `scripts/synthesis/dc/recam_phase4f_sources.tcl` | `Rs=Cs=2`, directional `m=1` | `results/phase4f/dc_tsmc018_slow/v2_specialized_group_noscratch_20ns`; metadata has no RTL Git SHA. |
| H4/H5 `EARLY` | `rtl/dss_v2/rs3cs3m1/recam_dss_v2_rs3cs3m1_early_top.sv`, `recam_dss_v2_rs3cs3m1_early_core.sv`, `scripts/synthesis/dc/recam_rs3cs3m1_h5_early_sources.tcl` | `Rs=Cs=3`, directional `m=1` | H5 metadata records `RTL_GIT_REVISION=3304c86` and source SHA256 `854572fd9cd7ec10470ebc261887e4cd5f5d5d1de17b725a671e90c2c940f4b7`. |
| H4/H5 `GROUP-NoScratch` | `rtl/dss_v2/rs3cs3m1/recam_dss_v2_rs3cs3m1_group_top.sv`, `recam_dss_v2_rs3cs3m1_group_core.sv`, `scripts/synthesis/dc/recam_rs3cs3m1_h5_group_sources.tcl` | `Rs=Cs=3`, directional `m=1` | Same H5 revision/SHA record and `GROUP_NOSCRATCH` boundary. |

The isolated RS3 source/result tree is currently untracked in this worktree.
The recorded H5 SHA, rather than an assumption that a current untracked file
is committed, is the usable source-provenance evidence.

## Candidate and resource contract

All audited directional RTL has four candidate action classes per SA:

| Canonical slot | Action | Supported by historical RTL |
|---:|---|---|
| 0 | LOCAL | yes |
| 1 | RELEASE_ONLY | yes |
| 2 | BORROW_ONLY | yes |
| 3 | RELEASE_AND_BORROW | yes |

The four-resource directional graph, release ownership, donor priority,
single-borrower exclusion, and A→B→C→D traversal are shared.  A completed
candidate is locally eligible only when the analyzer marks it
solution-valid/repairable; a borrowing action additionally requires an
available released donor.  A commit updates the registered ledger atomically.
On exhaustion of the current SA's ranked candidates, the group fails at that
SA; previous commits remain, with no rollback.

Numeric ConfigID is architecture-point scoped and is not policy evidence.
For `Rs=Cs=2`, A/D encode slots as `0,4,5,6` and B/C as `0,1,2,3` in the
frozen DATE contract.  For the isolated RS3 table, A/D use `0,1,2,3` and B/C
use `0,4,5,6`.  In both cases the semantic slot class above is the comparison
key.

## Semantic comparison matrix

`✓` means the behavior is present; `—` means it is not part of that policy;
`NO` in **Exact match** identifies the first semantic mismatch.

| Historical RTL behavior | Candidate contract / slots | Release-only / borrow-only / release+borrow | SA / candidate priority | Ledger, commit, rollback | LOCAL_FIRST | EARLY | GLOBAL | Exact match |
|---|---|---|---|---|---|---|---|---|
| Phase 3J `EARLY`, V2 `EARLY` (`Rs=Cs=2`) | directional V2, 4 | ✓ / ✓ / ✓ | A/D `L,R,B,RB`; B/C `L,B,R,RB` | live ledger; A→B→C→D first legal commit; no rollback/backtracking | NO | NO | NO | **NO** — B/C rank differs from every frozen canonical policy. |
| H4/H5 `EARLY` (`Rs=Cs=3`) | RS3 directional V2, 4 | ✓ / ✓ / ✓ | A/D `L,R,B,RB`; B/C `L,B,R,RB` | same sequential commit/no rollback | NO | NO | NO | **NO** — same B/C priority mismatch. |
| Phase 3J `GROUP`, V2 `GROUP-NoScratch` (`Rs=Cs=2`) | directional V2, 4 | ✓ / ✓ / ✓ | every SA `R,L,RB,B` | live ledger; A→B→C→D first legal commit; no rollback/backtracking | NO | **YES** | NO | **YES, decision/resource boundary**. |
| H4/H5 `GROUP-NoScratch` (`Rs=Cs=3`) | RS3 directional V2, 4 | ✓ / ✓ / ✓ | every SA `R,L,RB,B` | same sequential commit/no rollback | NO | **YES** | NO | **YES, decision/resource boundary**. |

Normalized `GLOBAL` requires exact directional V2 candidate identity plus
joint complete-tuple search and backtracking.  None of the historical designs
enumerates complete tuples: GROUP retains candidates only to choose each SA's
next first feasible commit.  Consequently a GLOBAL implementation cannot be
created by relabeling `GROUP` or `GROUP-NoScratch`.

## Direct source evidence

- `recam_dss_v2_early_core.sv` maps B/C `rank_q=1` to ConfigID 2 and
  `rank_q=2` to ConfigID 1.  Under the frozen DATE table, these are
  `BORROW_ONLY` then `RELEASE_ONLY`.
- `recam_dss_v2_rs3cs3m1_early_core.sv` maps B/C rank to slots `0,2,1,3`,
  establishing the same action order without relying on numeric ConfigID.
- `recam_dss_v2_group_core.sv` gets its slot order from
  `dss_v2_group_priority_reader.sv`: `1,0,3,2`.
- `recam_dss_v2_rs3cs3m1_group_core.sv` explicitly defines that order as
  `RELEASE_ONLY, LOCAL, RELEASE_AND_BORROW, BORROW_ONLY`.
- `src/DynamicCsvReporter.cpp` identifies
  `directional_v2_early` as normalized `LOCAL_FIRST`,
  `group_greedy_rtl_canonical` as normalized `EARLY`, and
  `directional_v2_group_global` as normalized `GLOBAL`.  Its matching C++
  slot order is encoded in `inc/V2GroupNoScratchPolicy.hpp`.

## Equivalence evidence and boundary

Existing behavioral evidence supports the two statements below, but it must
not be overstated as full end-to-end fault-stream equivalence:

| Evidence | What it checks | Result |
|---|---|---|
| `docs_verilog/PHASE3HI_VERIFICATION.md` | independent golden map model versus Phase 3J EARLY/GROUP: ConfigID, donor, ledger, failure position, repairability; 50 directed/bring-up + 1,000 seeded random vectors per policy | 0 mismatches reported |
| `docs_verilog/PHASE4E_CHARACTERIZATION.md` | V2 EARLY versus legacy EARLY decision/resource boundary | directed and 1,000 random vectors, 0 mismatches reported |
| `docs_verilog/PHASE4F_CHARACTERIZATION.md` | V2 GROUP-NoScratch versus legacy GROUP boundary | 50 + 1,000 vectors, 0 mismatches reported |
| `results/dss_v2_rs3cs3m1/h4_functional/summary.txt` | isolated RS3 table/core policy test with independent candidate-map golden model | EARLY 1,000 and GROUP 1,000 vectors, 0 mismatches reported, seed `20260910` |

The established tests inject completed candidate maps rather than replaying
the normalized 1k raw fault corpus through both C++ and RTL.  They prove the
controller/ledger behavior at the explicit decision boundary; they do **not**
establish end-to-end equivalence for the six normalized corpus witnesses.
No such replay was started for this audit.  Thus:

```text
RTL_LOCAL_FIRST_EQUIVALENCE: NOT_DEMONSTRATED; historical EARLY has a B/C priority mismatch.
RTL_EARLY_EQUIVALENCE: GROUP/GROUP-NoScratch supported at decision/resource boundary by existing directed and random candidate-map regressions.
RTL_GLOBAL_EQUIVALENCE: NO; no audited RTL implements joint complete-tuple search/backtracking.
```

## Historical synthesis-number usability

All listed values are DC total cell area at the reported boundary; they are
not interchangeable across different boundaries or policies.

| Architecture point / reported result | Number | Normalized interpretation | Thesis use |
|---|---:|---|---|
| RS2 RECAM analyzer baseline | area 50,877.29, GE about 5,098.33, WNS 0.00 ns | analyzer-only / no full normalized sharing controller | usable only as the stated RECAM/analyzer baseline, not a policy-ladder point |
| RS2 Phase 3J historical `EARLY` | area 117,219.01, GE about 11,746.33, WNS +0.01 ns | B/C `L,B,R,RB`, not a canonical normalized policy | **not semantically usable** for LOCAL_FIRST/EARLY/GLOBAL comparison |
| RS2 Phase 3J historical `GROUP` | area 117,139.18, GE about 11,738.33, WNS +0.01 ns | normalized EARLY boundary | usable only as legacy normalized-EARLY boundary evidence |
| RS2 V2 `EARLY` | area 80,871.44, WNS +0.01 ns | same noncanonical B/C early priority | **not semantically usable** for a normalized policy figure |
| RS2 V2 `GROUP-NoScratch` | area 105,393.66, WNS +0.00 ns | normalized EARLY boundary | usable for normalized EARLY boundary, with the documented deferred reconstruction limitation |
| RS3 H5 `EARLY` | area 12,190,913.30, WNS -42.65 ns | same noncanonical B/C early priority | not semantically usable and not timing-closed |
| RS3 H5 `GROUP-NoScratch` | area 12,053,273.51, WNS -44.24 ns | normalized EARLY boundary | semantic mapping usable, timing result is **not closed** |

The P0 prompt's older RS3 values (`518,213.21`, `546,667.23`, one-stage
pipeline `500,942.54`/`536,060.19`) were not found in an authoritative report
or metadata file in this checkout.  They are therefore `NOT_PROVEN` here and
must not be attached to a normalized semantic label on the basis of this
audit.

## Reuse consequence

For the production-oriented normalized EARLY target, start from
`GROUP-NoScratch`, not a historically named EARLY core.  The shared analyzer,
candidate action table, directional topology, resource ledger, donor order,
and commit interface are reusable.  A normalized LOCAL_FIRST core needs a
small controller/priority correction (especially B/C), while normalized GLOBAL
needs a new complete-tuple-search controller and retained/search state; the
analyzer datapath need not be rewritten merely because the historical policy
name is outdated.

## Read-only RTL review delivery matrix

The readable-Verilog review entrypoint was attempted on the four V2 core
files.  It could not initialize under the installed Python 3.8 runtime because
the skill package uses `dict[str, ...]` annotations, which require Python 3.9
or newer.  No source or generated review artifact was written into the
repository.

| Gate | Status | Evidence / limitation |
|---|---|---|
| compile | NOT_RUN | no new RTL compilation in this audit |
| ast | BLOCKED | review runtime Python-version incompatibility |
| readability | BLOCKED | same review-runtime failure |
| comment | NOT_RUN | no comment change requested |
| naming | BLOCKED | same review-runtime failure |
| profile | BLOCKED | same review-runtime failure |
| testbench | EXISTING_EVIDENCE_ONLY | directed/random regressions listed above; not rerun |
| toolchain | EXISTING_EVIDENCE_ONLY | historical DC reports inspected; synthesis not run |

```text
RTL_MODIFIED: NO
SYNTHESIS_STARTED: NO
FORMAL_GROUP_DATA_TOUCHED: NO
```
