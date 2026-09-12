# Phase 3A.1 — RECAM Matrix Builder Audit

> Status: COMPLETE — analysis only.  This audit does not authorize a Phase 3A
> RTL change or Phase 3B implementation.
>
> Evidence: `rtl/recam/recam_2r2c_matrix_builder.sv` and terminal TSMC018
> `slow.db` structural-mapping reports under
> `results/phase3a/dc_tsmc018_slow/recam_2r2c_matrix_builder/`.

## 1. Current measured result

| Metric | Matrix Builder | Complete Analyzer |
|---|---:|---:|
| Cell area | 19,758.816270 | 20,084.803477 |
| Combinational cells | 1,158 | 1,183 |
| Sequential cells | 0 | 0 |
| Critical path | 38.17 ns | 43.21 ns |
| WNS at 20 ns | -18.17 ns | -23.21 ns |

The Matrix Builder is 98.4% of Complete Analyzer cell area.  These are
structural mapped-area and post-map STA results, not timing closure.

## 2. Functional responsibilities

The module starts from a four-entry pivot view plus four full row/column Hybrid
tuples.  It performs all of the following combinationally:

1. Copies valid pivot addresses into row and column dictionaries and builds a
   3-bit pivot population count.
2. Initializes the 4×4 diagonal and applies `row_must_i`/`col_must_i`.
3. Processes each valid Hybrid entry in source index order.
4. Searches the current, potentially extended dictionaries for a matching row
   and matching column; the first match wins.
5. Either writes a matrix cell, fills a matrix row/column, or appends a row or
   column dictionary entry when capacity remains.
6. Flattens the final 4×4 matrix.

The source `for` loops are elaborated and fully unrolled; they do not create
iterative hardware or sequential cycles.

## 3. Area-dominant structures

DC flattened the Matrix Builder, so RTL blocks do not have independent
hierarchical area.  The mapped reference counts identify these useful proxies:

| Mapped category | Cells | Cell area | Interpretation |
|---|---:|---:|---|
| XOR/XNOR | 298 | 7,930.138 | Equality, population/count and selection logic; 271 are XOR2 and 27 XNOR2. |
| AOI/OAI | 311 | 5,691.470 | Priority guards, conditional matrix updates, and mux decomposition. |
| Boolean + inversion | 538 | 5,877.749 | Valid gating, write enables, reductions, and complements; includes 204 inverters. |
| Explicit mapped mux cells | 11 | 259.459 | `MX2X1`/`MXI2X1`; most muxing maps into AOI/OAI/Boolean networks rather than cells named MUX. |

The dominant source structure is not a single macro.  It is the four-stage
unrolled, data-dependent Hybrid update chain.  Each stage can compare against
dictionary values written by an earlier stage, then conditionally select a free
dictionary location and conditionally update 16 matrix bits.

### Address comparisons and priority logic

For every one of four Hybrid entries, source code scans four dictionary slots
for both row and column equality: 4 × 4 × 2 = **32 nominal 10-bit equality
comparisons**.  Some are gated, optimized, or replicated in the mapped logic,
so this is an RTL-operation count, not a one-to-one cell count.

`!row_match` and `!col_match` make each four-slot scan a first-match priority
selection.  `pivot_count`, `row_count`, and `col_count` are 3-bit values;
free-entry detection is `pivot_count < 4`, `row_count < 4`, and `col_count <
4`.  The result is a combinational free dictionary insertion pointer, not an
Address-CAM allocation pointer.

### Variable-index and write-mux expansion

The following variable indices infer selection/write networks:

- `row_dict[index]` and `col_dict[index]` reads while scanning;
- `matrix[row_index][col_index]` writes;
- `matrix[row_index][col_count[1:0]]` and
  `matrix[row_count[1:0]][col_index]` writes;
- `row_dict[row_count[1:0]]` and `col_dict[col_count[1:0]]` writes.

For fixed dimensions, synthesis lowers these variable accesses into mux/decode
and per-bit write-enable logic.  The 11 named mux cells therefore understate
the effect: the AOI/OAI/Boolean population implements most of the mux trees.

## 4. Critical-path explanation

The worst reported path is:

```text
pivot_valid_i[1]
→ XOR/XOR/AND/NAND/OAI population-count and valid-prefix logic
→ unrolled Hybrid match / priority / free-dictionary update logic
→ variable-index matrix-write mux and final matrix accumulation
→ matrix_flat_o[3]
```

DC reports 67 logic levels, 38.17 ns arrival, and -18.17 ns slack.  The first
two cells are `U2146` and `U2145` (`XOR2X1`), driven by pivot-valid signals;
the path ends at `U1033` (`NAND4X1`) driving `matrix_flat_o[3]`.  It contains
the priority/mux network formed by `NAND4`, `NOR4`, `AOI`, `OAI`, `XNOR`, and
other Boolean cells.  It contains no 10-bit address input at its startpoint.

Accordingly, the measured worst path is dominated by **state-dependent Hybrid
classification, priority/free-index selection, and matrix-write muxing**,
following pivot-valid population/count logic.  It is not dominated by the
six-pattern analyzer.  Address equality is a major area contributor and feeds
the same Hybrid path, but this particular worst path is pivot-valid initiated.

## 5. Analyzer-boundary audit

The intended boundary is `CAM logical state → Matrix → PatternID`.  Current
inputs already provide pivot validity, pivot addresses, Hybrid validity, and
Must flags, but the Matrix Builder reconstructs parts of the Hybrid logical
relationship from full addresses.

| Item | Current Phase 3A behavior | Classification |
|---|---|---|
| Pivot matching | Recomputed for each Hybrid by 10-bit row/column equality against the evolving dictionaries. | **Uncertain / requires RECAM-reference verification.** The specification defines Hybrid entries relative to an Address-CAM pointer, so this is a candidate logical-state field rather than necessarily analyzer work. |
| Hybrid pivot pointer | Not supplied; inferred by address searches. | **Belongs before Analyzer**, subject to preserving the paper-defined Hybrid pointer semantics. |
| Hybrid descriptor | Not supplied; inferred from row-match/column-match outcome. | **Belongs before Analyzer**, subject to reference verification of the exact descriptor encoding. |
| Logical entry membership | `pivot_valid_i` and `hybrid_valid_i` are inputs; no membership is recomputed. | **Belongs before Analyzer.** |
| Free Address-CAM index | Not recomputed.  `row_count`/`col_count` only select a free matrix-dictionary location. | Address-CAM allocation: **before Analyzer**. Matrix-dictionary insertion: **belongs inside Analyzer**. |
| Must classification | `row_must_i` and `col_must_i` are consumed, not derived. | **Belongs before Analyzer.** |

`docs/RECAM_SPEC.md` specifies a Hybrid entry as enable, Address-CAM pointer,
address descriptor, and one row-or-column address.  The current Phase 3A
full-row/full-column Hybrid tuple is therefore a conservative functional input
representation, not a demonstrated minimal logical-CAM interface.

## 6. Suspected avoidable RTL expansion

- Four sequentially dependent Hybrid stages are expressed as one procedural
  combinational block.  Every stage can observe dictionary updates from earlier
  stages, so synthesis materializes a deep chain rather than independent work.
- Full 10-bit row and column values are carried for every Hybrid entry, then
  their relation to pivots is rediscovered.  This creates the 32 nominal
  equality operations and associated priority logic.
- Dynamic array reads and dynamic array/matrix writes expand to mux/decode
  networks across all four dictionary slots and sixteen matrix cells.
- Matrix state is repeatedly reconstructed and conditionally rewritten during
  each Hybrid stage.  This is required by the current representation but is a
  large duplicated logical-state reconstruction cone.

There are **no accidental 32-bit datapaths** in the mapped design: loop
variables are elaboration-time `integer`s; runtime counts are explicitly
3-bit and indices 2-bit.  DC's DesignWare report contains narrow width-2,
width-3, and width-10 operators, not a width-32 arithmetic operator.  There is
also no separately inferred large priority-encoder macro; the first-match
priority function is distributed through the mapped Boolean/AOI/OAI network.

## 7. Safe RTL cleanup candidates

These preserve the Phase 3A algorithm and boundary, but require separate
authorization and full regression before implementation:

- Express the fixed 4×4/4-entry structure with statically indexed generated
  wires/cases rather than variable array writes, while preserving exact
  source-order Hybrid behavior.
- Factor repeated comparison predicates and one-hot match results so a given
  Hybrid/dictionary comparison is calculated once and fanned out.
- Encode fixed matrix update cases explicitly to avoid re-materializing a
  general variable-index write mux for each matrix bit.
- Keep widths exact (the current runtime widths are already narrow) and retain
  the same four-entry Hybrid capacity.

## 8. Architecture-changing candidates

These change the analyzer interface/resource partition and must not be made
under Phase 3A authority:

- Present Hybrid `pivot_pointer` and `descriptor` as part of the CAM logical
  state, replacing address-based pivot reclassification.
- Present the Hybrid's single differing address rather than a full row/column
  pair, matching the documented Hybrid logical entry.
- Precompute one-hot dictionary membership or normalized matrix coordinates in
  the collector/CAM state.  This moves work before the analyzer boundary.
- Change Hybrid processing order, add pipeline registers, or serialize Hybrid
  conversion.  These affect architecture or timing behavior and are excluded.

## 9. Recommendation for a future Phase 3B Matrix Builder

Do not reuse the current raw-address, evolving-dictionary reconstruction as the
shared analyzer's default microarchitecture.  After explicit Phase 3B
authorization and RECAM-reference confirmation, use a canonical logical CAM
view with pivot-valid entries, Must flags, and Hybrid
`{valid, pivot_pointer, descriptor, differing_address}` metadata.  The shared
Matrix Builder should consume that view and perform direct fixed-coordinate
matrix updates.  Preserve ordered Hybrid semantics and the fully parallel
candidate evaluator; do not use pattern serialization as a timing remedy.
