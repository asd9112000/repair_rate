# DSS V2 — DATE Paper Figure and Table Plan

DATE has a six-page constraint.  The minimum evidence set should communicate
the policy tradeoff without duplicating tables.

| Item | Content | Priority | Source-ready artifact | Rationale |
|---|---|---|---|---|
| Table A | EARLY vs GROUP hardware/state/latency/repairability tradeoff | MUST_HAVE | `PHASE4E_TO_4K_MASTER_EVIDENCE.md` | One compact table answers the central cost/benefit question | 
| Figure A | Repair rate versus faults/group, EARLY and GROUP, moderate imbalance | MUST_HAVE | `phase4j_policy_tradeoff_summary.csv` | Shows where the measured GROUP benefit is material | 
| Figure B | Post-BIST exposed latency: mean/P95 `L_post` or zero-latency fraction versus `G` | NICE_TO_HAVE | `phase4i4_latency_plot_data.csv`; `phase4i4_zero_latency_plot_data.csv` | Establishes the latency cost if space permits | 
| Figure C/inset | A `CFG0 → CFG4` release at A followed by downstream resource availability | OMIT_IF_SPACE_LIMITED | `phase4k_representative_traces.csv` | Strong mechanism explanation, but can be a compact text example instead | 
| Table B | Full hardware comparison including legacy baselines | NICE_TO_HAVE | `PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md` | Useful only if Table A cannot carry baseline context | 

## Recommended narrative order

1. State the specialized boundary and functional equivalence.
2. Use Table A to show AREA × STATE × LATENCY × REPAIRABILITY.
3. Use Figure A for severity-dependent repairability.
4. Use one sentence or a small inset for the causal release mechanism.
5. Include Figure B only if latency is a central paper contribution and space
   remains.

Do not generate final visual styling, alter datasets, or begin a new simulation
from this plan.  All candidates remain derived from accepted artifacts.
