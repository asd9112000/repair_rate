# Phase 3J Full Directional DSS Synthesis @ 20 ns

## Environment and scope

The official Phase 3J comparison uses Synopsys Design Compiler W-2024.09-SP2,
TSMC018 ARM CBDK `slow.db` at the `slow` operating condition, top wire-load
model `tsmc18_wl10`, zero input/output delay, and a 20 ns (`50 MHz`) clock.
The sequential Full DSS tops constrain the actual `clk_i` port as `recam_clk`.
GE is `total cell area / NAND2X1 area`, with `NAND2X1 = 9.979200` area units.

The Full EARLY and Full GROUP tops each contain one shared multi-config
analyzer/scheduler, four registered ConfigPatternMaps, the Phase 3G fallback,
the registered physical ledger, commit logic, and the A->B->C->D allocator.
The policy is the sole intended difference. No Phase 3A--3I functional
semantics changed for this measurement.

## Functional pre-check

`bash scripts/simulation/run_recam_phase3hi_functional.sh` passed after the
synthesis-top preparation: Phase 3HI regression PASS, directed matrix 15/15
PASS, EARLY 50+1000 golden vectors 0 mismatches, GROUP 50+1000 golden vectors
0 mismatches, and invariant violations 0.

## Main comparison

| Target | Area | GE | ΔArea vs RECAM | ΔGE vs RECAM | WNS | Timing |
|---|---:|---:|---:|---:|---:|---|
| RECAM baseline (accepted Phase 3A) | 50,877.288438 | 5,098.33 | -- | -- | 0.00 ns | met |
| Full EARLY DSS | 117,219.010491 | 11,746.33 | +130.3956% | +130.3956% | +0.01 ns | met |
| Full GROUP DSS | 117,139.176890 | 11,738.33 | +130.2386% | +130.2387% | +0.01 ns | met |

The accepted RECAM baseline is retained from
`results/phase3a/dc_tsmc018_slow/phase3a_corrected_20ns_fast`; it is not
overwritten or relabelled. Phase 3D/3E partial results are historical and are
not Full DSS results.

## EARLY versus GROUP

GROUP is 79.833601 area units and 8.00 GE smaller than EARLY, a -0.0681% area
delta under the same flow. This is a synthesis optimization difference for the
frozen policy logic, not evidence of a different architectural scope.

## Cell and timing detail

| Target | Comb cells | Seq cells | Total leaf cells | Comb area | Seq area |
|---|---:|---:|---:|---:|---:|
| RECAM | 2,413 | 0 | 2,413 | 50,877.288438 | 0.000000 |
| Full EARLY | 5,218 | 485 | 5,703 | 91,336.291764 | 25,882.718727 |
| Full GROUP | 5,204 | 485 | 5,689 | 91,256.458163 | 25,882.718727 |

For both Full DSS targets the critical path is
`d/scheduler/scan_index_reg_reg[1]` to
`d/scheduler/config_pattern_map_o_reg[68]`, through the shared analyzer and
scheduler capture logic. Data arrival is 19.71 ns; required time is 19.72 ns;
slack is +0.01 ns. Reports show WNS/TNS 0.00/0.00 with no violating paths.

Hierarchical attribution is available for Full EARLY: shared analyzer
46,173.7588 area, scheduler 53,079.3653 area, allocator 16,555.4929 area,
and fallback selector 6,197.0832 area. These are synthesized hierarchy figures
and are provided as attribution, not as separately normalized standalone
measurements.

## Reproduction

```bash
bash scripts/synthesis/run_recam_phase3j_dc.sh
```

Reports are under `results/phase3j/dc_tsmc018_slow/phase3j_full_{early,group}_20ns/`.
