# N2 CA-LIVE pre-synthesis evidence closure

## Measured policy-side boundary

The measured origin is the accepted active-SA live policy state update. The
measured destination is policy solution ready. No raw-fault collector or
collector-to-policy state-update path is included in either CA-LIVE top.

## EARLY 10k selected-rank distribution

The live EARLY lockstep harness was rerun for 10,000 candidate-image vectors.
The distribution includes the 34,277 legal selected SA results; vectors that
terminate at an earlier unrepaired SA do not contribute a selected rank.

| Selected rank | Count | Fraction | Measured policy latency |
| ---: | ---: | ---: | ---: |
| 1 | 26,014 | 0.758934563 | 1 config cycle |
| 2 | 6,263 | 0.182717274 | 2 config cycles |
| 3 | 1,598 | 0.046620183 | 3 config cycles |
| 4 | 402 | 0.011727981 | 4 config cycles |

```text
N:      34277
min:    1
mean:   1.31114
median: 1
p95:    3
max:    4
```

The rerun retained zero semantic mismatches against the canonical policy;
both BIST-done-before-solution and solution-before-BIST paths passed.

## GROUP 10k policy latency

The current GROUP CA-LIVE lockstep rerun confirmed the four active-SA slot
scans plus one selector/register response cycle. BIST waiting is excluded.

```text
N:                 10000
min:               5
mean:              5
median:            5
p95:               5
max:               5
earlier-SA replay: 0
stale generation:  0
```

The 1,000-vector full-top lockstep separately passed physical addresses, line
type, valid-line results, and frozen analyzer/reconstruction integration.

## Source freeze and frozen hardware

The detailed path/hash/role source closure is
[`CA_LIVE_SOURCE_FREEZE.md`](provenance/CA_LIVE_SOURCE_FREEZE.md). Its 23
entries were rehashed after the latency rerun and all matched.

```text
EARLY_LIVE_SHA: 4393c2e33bf27bedb4e4733183def16196cdbacf26aa21a997e619d20f56b229
GROUP_LIVE_SHA: 772fbfac5bac58191f4bed577a8e7cfa3fea7575bf6877b9288f8b0cc62e413a
ANALYZER_HASH_MATCH: PASS
SELECTOR_HASH_MATCH: PASS
```

`EARLY_LIVE_SHA` and `GROUP_LIVE_SHA` are SHA256 digests of the ordered
`sha256sum` records for each live controller and top pair.

## Strict quality tool SystemVerilog audit

The installed strict readable-Verilog command exposes no extension,
include-glob, or SystemVerilog-mode option. Its formatter/validation discovery
uses only `.v`: `VERILOG_EXTENSIONS = {".v"}` and the comment scanner checks
`suffix.lower() == ".v"`. Explicit `.sv` file arguments therefore still
produce zero parsed source files. The tool is not applicable to these frozen
SystemVerilog sources and was not modified.

Meaningful source evidence remains the successful Verilator lint/compile and
directed, 1k, 10k, and GROUP full-top functional simulations.

## Architecture invariants

```text
EARLY per-SA analyzer-input bank: NONE
EARLY solution payload register:  NONE
EARLY pending solution:           held canonical analyzer output
EARLY same-edge old result:       suppressed by fault update

GROUP candidate storage:          existing 80-bit map
GROUP map clear on update:        NO
GROUP generation tags:            NONE
GROUP per-SA analyzer-input bank: NONE
GROUP earlier-SA replay:          NONE
GROUP reconstruction retention:   unchanged 440 bits
GROUP same-edge old map write:    suppressed by fault update

RAW_FAULT_END_TO_END:             NOT_IMPLEMENTED
EARLY raw-fault 2..5 cycles:      ARCHITECTURAL_ESTIMATE_ONLY
GROUP raw-fault 6 cycles:         ARCHITECTURAL_ESTIMATE_ONLY
```
