# Phase 3J Area Audit / Cost Breakdown

This is an attribution-only audit of the accepted Phase 3J implementation.
It introduces no functional, policy, traversal, or RTL optimization change.
Official results remain the constrained 20 ns Phase 3J reports.

## Commands and assumptions

All reports use TSMC018 ARM CBDK `slow.db`, `slow` condition, NAND2X1 area
9.979200, and the existing DC flow. The Full DSS clock is bound to `clk_i`.
Diagnostics were isolated from official reports:

```bash
CLOCK_PERIOD_NS=25.0 PHASE3J_RESULT_ROOT=results/phase3j/diagnostic_25ns bash scripts/synthesis/run_recam_phase3j_dc.sh
CLOCK_PERIOD_NS=30.0 PHASE3J_RESULT_ROOT=results/phase3j/diagnostic_30ns bash scripts/synthesis/run_recam_phase3j_dc.sh
```

## Hierarchical area, 20 ns

The reports preserve hierarchy. Values are inclusive unless called local, so
children must not be summed with their parents.

| Full EARLY hierarchy | Cell area | % total | Comb area | Seq area |
|---|---:|---:|---:|---:|
| Full top | 117,219.0105 | 100.0% | 91,336.2918 | 25,882.7187 |
| `d` local controller/map/glue | 26,750.9089 | 22.8% | 9,340.5311 | 17,410.3778 |
| shared scheduler (inclusive) | 73,836.1015 | 63.0% | 69,249.0 | 4,587.1057 |
| shared analyzer | 66,318.4373 | 56.6% | 66,318.4373 | 0 |
| allocator (inclusive) | 16,632.0001 | 14.2% | 12,746.7649 | 3,885.2352 |
| fallback selector | 6,210.3888 | 5.3% | 4,982.9472 | 0 |
| ledger checker | 222.8688 | 0.2% | 222.8688 | 0 |

Full GROUP shares the same scheduler/analyzer and ledger-state area. Its total
is 117,139.1769; allocator is 16,578.7777 and fallback is 6,210.3888. DC did
not emit per-hierarchy cell counts, so only top-level counts are proven:
EARLY 5,218 combinational / 485 sequential / 5,703 total leaf cells; GROUP
5,204 / 485 / 5,689 respectively.

## Why Full Phase 3J exceeds Phase 3D/3E partial integration

| Comparison | Area delta | Relative increase |
|---|---:|---:|
| Full EARLY - Phase 3D EARLY partial | +43,685.611436 | +59.409% |
| Full GROUP - Phase 3E GROUP partial | +43,446.110633 | +58.955% |

The partial tops contained one scheduler/map and one policy selector. Full
Phase 3J adds four stored maps plus a group controller (`d` local 26,750.9089
area), the allocator/fallback/ledger subsystem (16,632.0001 area for EARLY),
and a 1,270.6849-area scheduler remap increase under the full integration.
This inclusive-hierarchy reconciliation accounts for the observed jump; it is
not evidence of analyzer replication.

## Cumulative synthesis ladder

| Level | Diagnostic boundary | Area | GE | Comb | Seq | WNS | Delta area |
|---|---|---:|---:|---:|---:|---:|---:|
| A | corrected RECAM baseline | 50,877.288438 | 5,098.33 | 2,413 | 0 | 0.00 | -- |
| B | shared analyzer (Phase 3B) | 67,229.870944 | 6,737.00 | 3,514 | 0 | 0.00 | +16,352.582506 |
| C | scheduler / one ConfigPatternMap (Phase 3C) | 72,289.325428 | 7,244.00 | 3,610 | 85 | 0.00 | +5,059.454484 |
| D-EARLY | policy selector (Phase 3D partial) | 73,533.399055 | 7,368.67 | 3,699 | 85 | 0.00 | +1,244.073627 |
| D-GROUP | policy selector (Phase 3E partial) | 73,693.066257 | 7,384.67 | 3,711 | 85 | 0.00 | +1,403.740829 |
| E | resource fallback alone | not separately synthesized | -- | -- | -- | -- | -- |
| F | registered ledger alone | not separately synthesized | -- | -- | -- | -- | -- |
| G | commit/controller alone | not separately synthesized | -- | -- | -- | -- | -- |
| H-EARLY | Full Phase 3J | 117,219.010491 | 11,746.33 | 5,218 | 485 | +0.01 | +43,685.611436 from D |
| H-GROUP | Full Phase 3J | 117,139.176890 | 11,738.33 | 5,204 | 485 | +0.01 | +43,446.110633 from D |

E/F/G are intentionally reported as unmeasured rather than fabricated; their
combined contribution is available from the measured hierarchy above.

## Duplication audit

Synthesis hierarchy contains exactly one `d/scheduler/shared_analyzer`, one
`d/allocator`, one `d/allocator/f` fallback selector, and one
`d/allocator/f/ledger` checker. There is no instantiated EARLY or GROUP local
selector in the Full top: frozen ranking is inside the one fallback selector.
The top contains four captured 80-bit map registers plus the scheduler's
working 80-bit map, as required by sequential A->B->C->D allocation. One
allocator owns the sole registered 12-bit ledger and controller traversal.
No hierarchy evidence indicates duplicated analyzers, fallback cones, ledgers,
or controllers. Whether unconnected verification-result registers are fully
optimized is not separately attributable in the DC report, so no claim is made.

## Sequential state

The 485 synthesized sequential cells exactly match the proven logical-state
budget: scheduler = 85 bits (80-bit working ConfigPatternMap plus 5-bit
scheduler control); top = 327 bits (four 80-bit captured maps plus 7-bit
controller); allocator = 73 bits (12-bit ledger, current-SA/control,
commit-valid, and selected-result registers). Thus ConfigPatternMap-related
state is 400 bits, scheduler control is 5 bits, controller-only state is 7
bits, and allocator/ledger/result state is 73 bits (including the 12-bit
ledger). This is a logical-bit attribution; DC does not
publish per-register cell counts beyond the top-level 485.

## Timing sensitivity

| Clock | EARLY area / GE / cells | GROUP area / GE / cells | WNS |
|---|---|---|---:|
| 20 ns official | 117,219.010491 / 11,746.33 / 5,703 | 117,139.176890 / 11,738.33 / 5,689 | +0.01 ns |
| 25 ns diagnostic | 111,856.853688 / 11,209.00 / 5,514 | 111,843.548088 / 11,207.67 / 5,511 | 0.00 ns |
| 30 ns diagnostic | 109,681.388047 / 10,991.00 / 5,517 | 109,681.388047 / 10,991.00 / 5,516 | +0.01 ns |

20->30 ns reduces EARLY by 7,537.622444 (6.430%) and GROUP by 7,457.788843
(6.367%). Therefore timing pressure is measurable but does not explain the
roughly 59% partial-to-full area jump.

## Critical path

At 20 ns, both policies have the same critical path:
`d/scheduler/scan_index_reg_reg[1]` to
`d/scheduler/config_pattern_map_o_reg[68]`. The path traverses the shared
analyzer's combinational candidate/pattern logic and scheduler capture decode;
arrival is 19.71 ns, requirement 19.72 ns, slack +0.01 ns. The reported tail
includes analyzer `NAND4XL`, scheduler `CLKINVX4`, and scheduler `OAI22X1`.
The shared analyzer, not policy ranking or ledger commit, is the timing driver.

## Classification and recommendation

Intrinsic architectural cost is dominant: required five map states, registered
group controller, and allocator/ledger account for the measured full-design
increment. Timing-driven cost is medium (about 6.4% area sensitivity at 30 ns).
No accidental duplication is evidenced. Optimization opportunity is **MEDIUM**;
recommended next action is **SEMANTICS-PRESERVING AREA OPTIMIZATION**, focused
first on map/register storage and analyzer timing/buffering only after separate
authorization. No optimization is performed by this audit.
