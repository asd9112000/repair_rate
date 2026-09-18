# H5O-A — Synthesis Representation Reuse Audit

> Status: COMPLETE — read-only audit.  No RTL was modified in H5O-A.

## Scope and frozen reference

This audit concerns only the isolated `RS=CS=3, SHARE_M=1` target.  It does
not reinterpret the legacy/group-level simulator, authorize S1/E0/E1, or
change the H0S0-CL-004 status.

| Field | Frozen value |
|---|---|
| Original representation | `ORIGINAL_H4_VERIFIED_GENERIC_RTL` |
| Git base | `3304c86` |
| Target aggregate RTL SHA-256 | `854572fd9cd7ec10470ebc261887e4cd5f5d5d1de17b725a671e90c2c940f4b7` |
| Protected evidence | `results/dss_v2_rs3cs3m1/h5_hardware_retry2/` (not overwritten) |
| DC method | DC W-2024.09-SP2; TSMC018 `slow.db`; slow corner; 20.0 ns; zero IO delay; `compile -map_effort low`; NAND2X1 area 9.979200 |

The original H5 result remains valid for this exact generic RTL
representation.  It is not a final normalized architecture-scaling result.

## H5 measured evidence

| Mode | Total area | GE | Critical path | WNS | DC wall time | Analyzer mapped area |
|---|---:|---:|---:|---:|---:|---:|
| EARLY | 12,190,913.30 | 1,221,632.33 | 62.52 ns | -42.65 ns | 9,428.95 s | 12,178,895.02 (99.9%) |
| GROUP-NoScratch | 12,053,273.51 | 1,207,839.66 | 64.14 ns | -44.24 ns | 8,635.48 s | 11,995,819.93 (99.5%) |

The EARLY path runs from `core/sa_q_reg[0]` to
`core/selected_pattern_flat_o_reg[12]`; the GROUP path runs from `core/sa_q[0]`
to `core/store/store_q_reg[5]`.  The frozen timing reports classify both as
analyzer/candidate-evaluation dominated.  This establishes that analyzer
representation is the first investigation target; it does **not** establish
that one source construct alone causes all area or timing.

The completed retry-2 DC logs map 8,682 `DW01_add_width32` instances, 4,480
`DW_cmp_*` instances, and 35 `DW_leftsh` instances in each target mode.  The
source correlation is strongest for the runtime-visible `integer` arithmetic,
comparison/rank loop, and `1 << target_k` inside `nth_pattern`; the logs do
not attribute a unique cell count to a single source line.  Therefore costs
below are labelled measured only at module/top level and inferred at source
construct level.

## Review tool record

The requested secondary review tool was installed in an isolated Python 3.11
environment; no system Python or laboratory-wide package set was changed.

| Field | Record |
|---|---|
| `SKILL_NAME` | `readable-verilog-generator` |
| `SKILL_VERSION` | `v2.0.0` |
| `SKILL_RELEASE_OR_COMMIT` | `007372fb63fe1d0b51e5bb10f4569d0b62ae6730` |
| `SKILL_INSTALL_PATH` | `/home/asd9112000/.codex/skills/readable-verilog-generator` |
| Private interpreter | `venv/python311/bin/python3` = Python 3.11.11; reviewer environment `venv/h5o-skill/` |
| Install verification | official dry-run and install receipts succeeded; installed `INSTALL_RECEIPT.json` records tree SHA-256 `4c20963d768d362014f4386ecb724cddcd875c2b28b55b051d56a84f76aff603` |
| Discoverability | installed skill directory and receipt are present as `$readable-verilog-generator` |
| Review result | read-only `review --non-strict` was run on the target analyzer; it returned `UNSUPPORTED_VERILOG_DIALECT` because this reviewer accepts `.v`/Verilog-2001, while the frozen source is SystemVerilog `.sv` |

The tool therefore supplied no applicable style finding.  It was treated only
as a secondary reviewer; H1/H2/H4 semantics and H5 DC evidence take priority.
The source will not be converted to Verilog-2001 merely to satisfy the tool.

## Mandatory hotspot assessment

`Time constant` means fixed by the frozen RS=CS=3,m=1 architecture point, not
constant for all DUT inputs.  `Constant foldable` means an intended H5O
specialization can remove runtime genericity without changing the supported
physical configurations.  `Share predecode` is deliberately conservative:
`YES` is an audit candidate, not permission to edit it without a later
step-specific proof.

| Hotspot | Source location | Current representation / synthesis function | Measured or inferred cost; DC evidence | Time constant? | Constant foldable? | Table driven? | Fixed-width rewrite? | Share predecode? | Semantic risk | Recommendation |
|---|---|---|---|---|---|---|---|---|---|---|
| `nth_pattern` | analyzer lines 44–67 | Per candidate lane enumerates 128 masks, popcounts 7 bits, rank/selects the requested valid mask | **Inferred high:** 35 lanes × 128 masks × 7-bit tests before candidate matrix work.  Direct source correlation to retry-2 `DW01_add_width32=8682`, `DW_cmp_*=4480`, `DW_leftsh=35`; analyzer is 99.9%/99.5% of mapped area. | YES | YES | YES | YES | High: order defines bitmap and one-based PatternID | TABLE_DRIVEN |
| Candidate mask generation | lines 44–67; use line 160 | Generic descending-mask construction is invoked once for each of 35 lanes | **Inferred high:** duplicates a known finite 20/10/35/15 set across lanes; same DC and analyzer evidence as above. | YES | YES | YES | YES | High: descending orientation and inactive lanes must match exactly | SEMANTICS_PRESERVING_RESTRUCTURE |
| Popcount / candidate-rank | lines 48–63 | `integer ones`, `seen`, nested bit loop, rank comparison | **Inferred high:** repeated arithmetic/comparators in the generic enumerator; no per-line DC attribution. | YES | YES | YES | YES | High: changing rank convention reorders candidate IDs | TABLE_DRIVEN |
| Integer / `int` operations | lines 30, 45–47, 72–73, 95–173 | 32-bit `integer` loop/control and function temporaries; some loop indices may elaborate away, function temporaries do not safely assume that | **Measured correlation, split required:** DC maps width-32 adders/comparators, but logs do not prove every `integer` survives. | Mixed | Mixed | NO | YES | NO | Medium: mechanical replacement can change signedness/bounds | FIXED_WIDTH_REWRITE |
| Candidate-index calculation | lines 159–173 | 32-bit loop index feeds `nth_pattern`, candidate limit, valid bitmap, and `candidate[5:0]+1` | **Inferred medium:** generic rank/index is amplified by 35 lanes; the loop itself may unroll. | YES | YES | YES | YES | NO | High: PatternID 0/1/35 and lowest-valid selection are frozen | FIXED_WIDTH_REWRITE |
| 7×7 matrix evaluation | lines 112–122, 163–169 | Fixed K≤7 matrix build and per-lane 49-cell feasibility test | **Inferred material:** 35 × 49 candidate matrix predicates remain after P0; mapped area is analyzer-dominated, but no isolated DC cell count. | Envelope YES; contents NO | Partly | NO | Partly | YES, only if identical primitives are proven | High: matrix encodes Must/pivot/hybrid feasibility | SHARE_PREDECODE |
| 35 candidate lanes | lines 159–173 | Fully parallel fixed 35-wide superset, invalid lanes masked by class count | **Inferred material:** mandatory parallel replication of matrix predicate; this is architectural representation, not authority to serialize. | Width YES | Only inactive masks | YES | YES | YES | High: candidate parallelism and lowest-valid priority are required | REUSE_AS_IS |
| Address loops | lines 95–122, 137–154 | 7 pivot/address dictionary initialization, lookup, and matrix insertion | **Inferred medium:** fixed bounds can unroll; input addresses and occupancy are runtime facts. | Bounds YES | Bounds only | NO | Partly | YES | High: address 0/6 and dictionary-overflow behavior are H4 gates | REUSE_AS_IS |
| Hybrid loops | lines 124–157 | 17-entry descriptor/pointer/differing-address update and dictionary extension | **Inferred medium:** fixed 17×7 envelope, but data-dependent matching remains. | Bounds YES | Bounds only | NO | Partly | Potentially | High: Hybrid 0/16, overflow, and update order are H4 gates | REUSE_AS_IS |
| Must decode | lines 116–121, `threshold_bit` lines 69–82 | Threshold vectors selected by canonical row/column count and transpose | **Inferred medium:** repeated threshold muxes feed fixed matrix cells; no isolated DC attribution. | Class decode YES; bits NO | Partly | YES | YES | YES | Very high: Must changes feasible-set semantics | SHARE_PREDECODE |
| Transpose normalization | lines 85–86, 102–108, 117–120, 126 | Canonical rows/cols plus row/column input exchange and descriptor polarity | **Inferred medium:** conditional muxing feeds much of analyzer; no isolated mapped cost. | Physical class mapping YES | Partly | YES | YES | Potentially | Very high: transpose pairs must reconstruct identical canonical result | REUSE_AS_IS |
| Canonical config decode | analyzer lines 32–42, 85–86; target config table under `rtl/dss_v2/rs3cs3m1/` | Four canonical candidate classes, seven physical configurations, transpose reuse | **Inferred low/medium:** finite decoder is small relative to analyzer total, but it controls class selection. | YES | YES | YES | YES | YES | Very high: H1/H2 target identity and transpose map are frozen | TABLE_DRIVEN |
| GROUP candidate store | `rtl/dss_v2/rs3cs3m1/*group*`; H5 hierarchy | 112-bit candidate-history storage and capture interface | **Measured bounded:** 47,534.26 mapped area versus GROUP analyzer 11,995,819.93. | Interface/width YES | NO | NO | NO | NO | Very high: 112-bit history and atomic commit are frozen | REUSE_AS_IS |
| GROUP priority reader | GROUP policy/core path | Reads stored candidates under GROUP-NoScratch priority/action contract | **Inferred low relative to analyzer:** included in GROUP endpoint but not analyzer-dominant hierarchy. | Policy order YES | NO | NO | NO | NO | Very high: A→B→C→D, first failure, no rollback | REUSE_AS_IS |
| Ledger | target EARLY/GROUP policy/core RTL | Four-resource availability and commit bookkeeping | **Inferred low relative to analyzer:** no H5 hierarchy evidence implicates it in initial cost. | Semantics fixed; values runtime | NO | NO | NO | NO | Very high: independent H4 golden checks atomic selected-only commits and no rollback | REUSE_AS_IS |

## Priority and H5O-B minimal plan

| Priority | Decision at H5O-A gate |
|---|---|
| P0 — `nth_pattern` / masks | Proceed first.  Replace only the known canonical candidate enumeration with an exact fixed table. |
| P1 — synthesis-visible widths | Audit after P0 with compile/elaboration evidence; narrow only temporaries proven runtime-visible and mathematically bounded.  Do not replace every `integer`. |
| P2 — canonical decode | Candidate for finite table/decode only after P0/P1 evidence; preserve all seven physical IDs and transpose reuse. |
| P3 — matrix primitive | Consider shared predecode only if post-P0 reports still identify repeated identical terms.  Keep all 35 lanes parallel. |
| P4 — Must / transpose / descriptor | Preserve initially; any change requires focused H4R Must and transpose proof. |
| P5 — GROUP selector/history | REUSE_AS_IS. |
| P6 — ledger | REUSE_AS_IS. |

### Candidate-order preservation method

The reference order is the existing `nth_pattern` order: descending masks in
the active K-bit range, filtered to exactly `rows` set bits.  A deterministic
offline verifier will enumerate the same rule for all canonical classes and
check every lane before the table is accepted.  The planned canonical table
manifest is:

```text
classes: 3R3C=20, 3R2C=10, 4R3C=35, 4R2C=15
order: descending numeric mask; filter popcount(mask)==rows; only mask<2**K
manifest format: R,C:comma-separated two-digit hexadecimal masks plus LF
SHA-256: 5448ee2482742be89d31118b89e1ab2666b5507f27880787ed6c2710b77e54b0
```

The table will retain the 35-wide output bitmap, leave inactive lanes zero,
and retain `PatternID=0` for invalid/no-solution and `candidate_index+1` for
the first valid lane.  The table verifier is offline tooling only; no runtime
enumeration may be introduced by it.

### Sequential implementation and validation plan

1. In the isolated H5O worktree, add the deterministic table verifier and the
   exact table source.  Check all 80 canonical lane values against the frozen
   enumeration algorithm, then run lint/elaboration and the focused analyzer
   regression.
2. Wire the fixed table into the analyzer and remove the synthesized generic
   `nth_pattern` path.  Re-run the table equivalence check, lint/elaboration,
   and focused analyzer regression before any P1 work.
3. Only if remaining DC/elaboration evidence supports it, make one narrowly
   bounded synthesis-visible width change and repeat the same checks.  Do not
   start P2–P4 merely because they appear in this audit.
4. Run H4R only after the selected H5O-B changes pass those step gates: all
   seven configurations, candidate counts and 35-bit bitmap, PatternID 0/1/35,
   K=7, pivot/address 6, Hybrid 16, Must, transpose pairs, GROUP store,
   EARLY/GROUP 1000-vector zero-mismatch tests, and frozen 2,2,1 directed
   15/15 plus 50/1000 zero-mismatch regressions.
5. Only after H4R passes, synthesize to the new
   `results/dss_v2_rs3cs3m1/h5_hardware_optimized/` location using the frozen
   H5 method and 14,400-second cap.  Never overwrite retry-2.

## H5O-A exit gate

```text
PRIMARY_SYNTHESIS_HOTSPOT = generic nth_pattern / mask enumeration / rank-select
SECONDARY_SYNTHESIS_HOTSPOTS = synthesis-visible 32-bit arithmetic and compares;
                                repeated 35-lane matrix predicates; canonical
                                threshold/transpose decode (unproven until later)
SOURCE_TO_DC_EVIDENCE = nth_pattern lines 44–67 and candidate loop lines 159–173
                        correlate with 8,682 DW01_add_width32, 4,480 DW_cmp_*,
                        35 DW_leftsh mappings and 99.9%/99.5% analyzer area;
                        per-line area is not claimed
MINIMAL_RESTRUCTURING_PLAN = P0 fixed exact candidate table, then only
                             evidence-backed P1–P4 steps
CANDIDATE_ORDER_PRESERVATION = deterministic 80-lane reference-table verifier,
                               manifest checksum, unchanged bitmap/PatternID rules
H4R_VERIFICATION_PLAN = full H4 target suite plus frozen 2,2,1 regression as
                         enumerated above
H5O_A_GATE = PASS
```

```text
H5 = COMPLETE (original generic RTL evidence remains frozen)
H5_ORIGINAL_RESULT_VALID = YES
H5_FINAL_NORMALIZED_SCALING_RESULT = NOT_YET_FROZEN
H5O-A = COMPLETE
H5O-B = AUTHORIZED
H4R = AUTHORIZED_AFTER_H5O-B
H5O-SYNTHESIS = AUTHORIZED_AFTER_H4R_PASS
H5P_AUTHORIZED = NO
S1/E0/E1 = UNAUTHORIZED
```
