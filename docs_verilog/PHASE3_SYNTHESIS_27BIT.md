# Phase 3 Corrected 27-bit Synthesis Record

## Scope and conclusion

This record supersedes the pre-correction Phase 3A numbers for the frozen
27-bit geometry (`Row=9`, `ColumnWord=5`, five Address entries, seven Hybrid
entries).  It covers only the combinational analyzer boundary: stored logical
Address/Hybrid CAM state plus inputs through repair/pattern outputs.  CAM
storage, fault collection, scheduling, and ConfigPatternMap storage are not
included.

The corrected Phase 3A baseline and the Phase 3B shared multi-Config analyzer
both close at the primary 20 ns target in the same timing-driven DC flow, with
zero sequential cells.  The 15 ns timing-driven attempts did not reach a
terminal tool result in this environment; the retained 15 ns records are
structural mappings with post-map STA and must not be read as timing closure.

This is sufficient evidence to mark Phase 3B complete at its authorized
analyzer scope.  It does not authorize Phase 3C or any timing-driven RTL
redesign.

## Common methodology

- Tool/library: Design Compiler W-2024.09-SP2, TSMC018 `slow.db`, operating
  condition `slow`, WLM `tsmc18_wl10`.
- Constraint boundary: 0 input/output delay with a virtual block clock.
- Primary result: constraints loaded before `compile -map_effort low`.
  DC reports that this legacy effort setting defaults to medium; it is retained
  so the corrected Phase 3A and Phase 3B runs use the same method.
- Structural comparison: `compile -exact_map` before constraints, followed by
  post-map STA.  It is valid for mapped structure/area and diagnostic STA, but
  is not a timing-driven closure result.
- NAND2 reference: `slow/NAND2X1`, cell area `9.979200`.  GE is total cell
  area divided by that area; it is not a literal NAND2 count.

## Timing-driven primary result (20 ns)

| Design | Cell area | Leaf cells | Comb. / seq. cells | GE | Critical path | WNS / TNS | Result |
|---|---:|---:|---:|---:|---:|---:|---|
| Corrected Phase 3A | 50,877.288438 | 2,413 | 2,413 / 0 | 5,098.33 | 20.00 ns | 0.00 / 0.00 ns | PASS |
| Phase 3B shared analyzer | 67,229.870944 | 3,514 | 3,514 / 0 | 6,737.00 | 19.98 ns | +0.02 / 0.00 ns | PASS |

Phase 3B relative to corrected Phase 3A is +16,352.582506 area (+32.14%),
+1,101 leaf cells (+45.63%), and +1,638.67 GE (+32.14%).  Its reported
critical startpoint is `config_id_i[2]` and endpoint `pattern_id_o[1]`; the
Phase 3A path starts at `pivot_valid_i[1]` and ends at `repairable_o`.

## Structural mapping and post-map STA (diagnostic only)

| Target | Design | Cell area | Leaf cells | Comb. / seq. | GE | Critical path | WNS | TNS |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| 20 ns | Corrected Phase 3A | 28,071.490005 | 1,684 | 1,684 / 0 | 2,813.00 | 71.92 ns | -51.92 ns | -1,217.91 ns |
| 20 ns | Phase 3B | 45,545.069181 | 2,459 | 2,459 / 0 | 4,564.00 | 76.69 ns | -56.69 ns | -841.00 ns |
| 15 ns | Corrected Phase 3A | 28,071.490005 | 1,684 | 1,684 / 0 | 2,813.00 | 71.92 ns | -56.92 ns | -1,342.91 ns |
| 15 ns | Phase 3B | 45,545.069181 | 2,459 | 2,459 / 0 | 4,564.00 | 76.69 ns | -61.69 ns | -925.68 ns |

At either structural target, Phase 3B adds 17,473.579176 area (+62.25%), 775
leaf cells (+46.02%), and 1,751 GE (+62.25%) over Phase 3A.  The structural
area differs from the timing-driven 20 ns result because the latter performs
timing-driven sizing/buffering.

## Hierarchy and path interpretation

The corrected Phase 3A 20 ns structural area decomposes exactly as follows:

| Block | Cell area | Leaf cells | GE | Critical path |
|---|---:|---:|---:|---:|
| Matrix Builder | 27,732.197202 | 1,658 | 2,779.00 | 67.92 ns |
| Pattern Analyzer | 179.625601 | 12 | 18.00 | 1.42 ns |
| Pattern Encoder | 159.667202 | 14 | 16.00 | 2.64 ns |

Matrix Builder therefore dominates Phase 3A: its unrolled Hybrid/dictionary
reconstruction and variable-index matrix updates account for nearly all of the
structural area and critical delay.  Phase 3B is a flat synthesized module.
Its timing-driven path begins at ConfigID and traverses configuration selection,
transpose normalization, dictionary/candidate processing, and result selection.
The report proves the ConfigID-to-PatternID path is critical; it does not prove
that one particular Hybrid loop iteration is the limiting arc.

## Reproducibility and caveats

- Timing-driven 20 ns reports: `results/phase3a/dc_tsmc018_slow/phase3a_corrected_20ns_fast` and `results/phase3b/dc_tsmc018_slow/phase3b_shared_20ns_fast`.
- Formal structural 15 ns reports: `results/phase3a/dc_tsmc018_slow/phase3a_corrected_15ns_structural` and `results/phase3b/dc_tsmc018_slow/phase3b_shared_15ns_structural_retry`.
- The non-terminal 15 ns timing-driven runs are deliberately excluded from all
  closure claims.
- All accepted reports have zero sequential cells and zero sequential-cell
  area.  The flow rejects a terminal report with nonzero sequential cells.
- DC emits legacy signed-to-unsigned index/part-select warnings and the
  virtual-clock no-source warning.  Existing functional Verilator checks have
  zero WIDTH warnings; no synthesis-only RTL modification was made.

The raw reusable logical CAM state remains 5 Address entries x 28 bits plus 7
Hybrid entries x 20 bits = 280 bits.  That accounting is distinct from, and is
not added to, the analyzer standard-cell area above.
