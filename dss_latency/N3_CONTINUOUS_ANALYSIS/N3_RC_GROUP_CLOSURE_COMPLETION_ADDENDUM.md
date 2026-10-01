# N3 RC-GROUP Functional-Closure Addendum

## New verification evidence

- Strict readable-Verilog gate: PASS.  `dss_n3_rc_group_candidate_store.sv` is explicitly confirmed as `design` by `provenance/N3_READABLE_VERILOG_ROLE_SPEC.json`; this is role metadata only.  The equivalent N2 production module is `rtl/g2x2_r_group/dss_v2_group_candidate_store.sv`.
- Independent analyzer oracle: PASS, 10,000 deterministic cases, seed `0x4e33524f`, 0 mismatches; feasible 16 and unrepairable 9,984.  Each of the seven N3 capacity classes appears 1,428 or 1,429 times.  The oracle independently consumes all 524 production-state bits and the 80-row fixed-mask CSV.
- Directed controller preemption: PASS for Config #1, #2, #3, and #4.  Each case verifies same-edge update dominance, candidate-write suppression, restart at Config #1, no partial finalization, and preservation of frozen SA A records.
- N3 distinction search: PASS for seventh-pivot and Hybrid entry #9 materiality, and observes a legal PatternID greater than 15 in the analyzer.
- Structural Verilator compile: PASS.

## Width and connection audit

| Field | Count | Width | Total | Private pivot | Purpose |
|---|---:|---:|---:|---|---|
| pivot rows | 4 SA | 7 x 9 | 252 | yes | captured row addresses |
| pivot columns | 4 SA | 7 x 13 | 364 | yes | captured physical-column addresses |
| SA commit valid | 4 | 1 | 4 | no | final commit qualification |
| selected config | 4 | 3 | 12 | no | mask/config reconstruction selection |
| selected PatternID | 4 | 6 | 24 | no | fixed-mask reconstruction selection |
| **total** |  |  | **656** |  |  |

`4*7*9 + 4*7*13 = 252 + 364 = 616` private pivot bits.  The remaining metadata is `4 + 12 + 24 = 40`; `616 + 40 = 656`.

PatternID is six bits throughout: analyzer output, top candidate wire, candidate-store write/read and 7-bit `{PatternID[5:0],valid}` records, selector arrays/outputs, registered `selected_pattern_flat[23:0]`, pivot-register input, and its six-bit fixed-mask lookup.  No narrowing declaration exists.

Hybrid entries 8--17 are top-level 17-entry inputs and are passed as full 17/51/17/221-bit vectors to the single analyzer.  The analyzer's loop is `hybrid = 0..16`; the directed Hybrid #9 witness proves functional observation.  Hybrid state is analyzer input, not retained reconstruction state.

## Closure boundary

The original 1k/10k full-top lockstep corpus remains **CONTROL / RECONSTRUCTION / REGISTERED-OUTPUT** evidence with seed `0x4e335247`, T0/PatternID 1 only; it is not arbitrary-analyzer-state closure.

The remaining required closure item is a compact full-top directed case that carries a PatternID >15 through analyzer, store, selector, and reconstruction in one execution.  Therefore this addendum does **not** yet declare `N3_RC_GROUP_FUNCTIONAL: COMPLETE`.  No RTL was changed; DC, latency characterization, commit, N3 R-GROUP, priority changes, and synthesis remain out of scope.
