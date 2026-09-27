# N2 CA-LIVE matched 20 ns synthesis summary

## Scope and methodology

Only the frozen CA-LIVE policy-engine tops were synthesized:

- `G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg`
- `G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg`

The flow used DC W-2024.09-SP2, `slow.db`, slow operating conditions,
the `tsmc18_wl10` top wire-load model, a 20.0 ns clock, zero max/min I/O
delay, `set_load 0.05`, and `compile -map_effort low`.  `NAND2X1 =
9.979200 um^2` is used for GE normalization.  No 10, 15, or 25 ns run was
launched.  CA-LIVE excludes the upstream persistent fault/CAM collector, as
does the matched old-canonical policy-engine boundary.

## Frozen-source gates

The pre- and post-synthesis checks both passed every entry in
`provenance/CA_LIVE_SOURCE_FREEZE.md`.

| Case | aggregate frozen core/top digest |
| --- | --- |
| EARLY CA-LIVE | `4393c2e33bf27bedb4e4733183def16196cdbacf26aa21a997e619d20f56b229` |
| GROUP CA-LIVE | `772fbfac5bac58191f4bed577a8e7cfa3fea7575bf6877b9288f8b0cc62e413a` |

The runner rechecked its exact synthesis manifest immediately before and
after DC for each case.

## Matched PPA comparison

All areas are total cell area in um^2.  WNS is the unrounded critical-path
slack reported in `qos.rpt`; the report's rounded design-WNS footer is `0.00`
for all rows.

| Family | Variant | Area | GE | Combinational area | Sequential area | Critical path | WNS |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| EARLY | old canonical | 102296.779959 | 10251.0001 | 101518.402346 | 778.377613 | 19.97 ns | +0.03 ns |
| EARLY | CA-SB | 222576.078660 | 22304.0002 | 153969.077432 | 68607.001228 | 19.64 ns | +0.01 ns |
| EARLY | CA-LIVE | 104668.503253 | 10488.6668 | 103780.354438 | 888.148815 | 19.76 ns | +0.05 ns |
| GROUP | old canonical | 166140.376634 | 16648.6669 | 133448.516656 | 32691.859978 | 19.90 ns | +0.02 ns |
| GROUP | CA-SB | 289948.985900 | 29055.3337 | 192009.788791 | 97939.197109 | 19.83 ns | +0.00 ns |
| GROUP | CA-LIVE | 171399.415025 | 17175.6669 | 138614.415895 | 32784.999130 | 19.88 ns | +0.02 ns |

### Old canonical to CA-LIVE

| Case | Area delta | Ratio | Percent | Comb. delta | Seq. delta | GE delta | Critical-path delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| EARLY | +2371.723294 | 1.023185 | +2.318% | +2261.952092 | +109.771202 | +237.6667 | -0.21 ns |
| GROUP | +5259.038391 | 1.031654 | +3.165% | +5165.899239 | +93.139152 | +527.0000 | -0.02 ns |

### CA-SB to CA-LIVE

| Case | Area delta | Ratio (live/SB) | Percent | Comb. delta | Seq. delta | GE delta | Critical-path delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| EARLY | -117907.575407 | 0.470259 | -52.974% | -50188.722994 | -67718.852413 | -11815.3334 | +0.12 ns |
| GROUP | -118549.570875 | 0.591136 | -40.886% | -53395.372896 | -65154.197979 | -11879.6668 | +0.05 ns |

The CA-SB reductions are measured PPA deltas only; this summary does not
attribute all of them to flip-flop removal.

## Live 20 ns details

| Case | Total / comb. / seq. cells | Timing | Design rules |
| --- | --- | --- | --- |
| EARLY | 4941 / 4923 / 15 | WNS +0.05 ns, TNS 0.00, path 19.76 ns | no max-transition, max-capacitance, or max-fanout violation |
| GROUP | 7969 / 7356 / 586 | WNS +0.02 ns, TNS 0.00, path 19.88 ns | 9 max-capacitance violations; no max-transition or max-fanout violation |

The EARLY critical path is
`core/active_sa_q_reg[1] -> core/a_released_q_reg`, 19.76 ns.  It spends
about 16.52 ns from analyzer input through `analyzer/solution_valid_o`; its
dominant class is **analyzer**.

The GROUP critical path is
`core/scan_slot_q_reg[0] -> core/candidate_store/store_q_reg[77]`, 19.88 ns.
It reaches `analyzer/pattern_id_o[0]` at 18.59 ns, then enters the candidate
store for the final 1.29 ns.  Its dominant class is **analyzer feeding a
candidate-write endpoint**, not an 80-bit map write alone.

For GROUP max capacitance, four nets report 0.16 against a 0.15 requirement
with -0.01 slack (`candidate_store_image_o[65]`, `analyzer/n2401`,
`analyzer/n3538`, and `core/candidate_store/n101`).  Five additional nets
are reported as `0.15 / 0.15` with `VIOLATED: increase significant digits`.
The raw wording is retained in the GROUP `constraints.rpt`; this is not
classified as clean.

Post-compile `check_design` reports normal LINT warnings for intentionally
unconnected/shorted/constant observable outputs; they are preserved in the
raw reports and are distinct from timing and design-rule constraints.

## Structural checks

Both synthesized netlists contain exactly one `recam_shared_config_analyzer`
instance.  Neither contains a `[1087:0]` state-bank vector.  The CA-LIVE RTL
also has no state-bank/state-selection identifier, consistent with direct
live analysis rather than replicated 4-by-272 state storage or a wide 4:1
state-selection cone.

## Result locations and latency contract

- EARLY raw reports: `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/synthesis/by_period/20ns/`
- GROUP raw reports: `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/synthesis/by_period/20ns/`

The verified policy latency contract remains:

- EARLY CA-LIVE: `1 / 1.31114 / 1 / 3 / 4` cycles (min / mean / median / p95 / max).
- GROUP CA-LIVE: `5 / 5 / 5 / 5 / 5` cycles.
- Raw fault-to-solution latency: architectural estimate only; the upstream raw-fault collector is not implemented in this policy-engine boundary.

No old-canonical, CA-SB, or `dss_final` source was changed in this phase.
No commit was made and N3 was not started.
