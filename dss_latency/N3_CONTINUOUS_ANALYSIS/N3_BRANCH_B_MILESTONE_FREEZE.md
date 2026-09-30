# N3 Branch-B milestone freeze

## Baseline identity

`N3_RC_GROUP_CA_LIVE_BASELINE: CHARACTERIZED_AND_FROZEN`

Representative point: G2X2 N3 RC-GROUP CA-LIVE; RS=3, CS=3, SHARE_M=1; address widths 9/13/13; MAX_K=7; 17 hybrid entries; six-bit PatternID; 524-bit analyzer payload; 112-bit GROUP store; 656-bit reconstruction retention; 616-bit private pivot capture; and four Config evaluations per active SA.

## Closed gates

| Gate | Status |
|---|---|
| N3 RC-GROUP functional closure | COMPLETE |
| Shared oracle Layer A | COMPLETE |
| Shared oracle Layer B | COMPLETE |
| Shared oracle Layer C | COMPLETE |
| Latency characterization | COMPLETE |
| Source freeze | PASS |
| Pre-freeze structural audit | PASS |
| Pre-DC / post-DC source hash match | PASS / PASS |
| N2→N3 scaling | COMPLETE |

Functional evidence is frozen as: analyzer random `10000` vectors / `0` mismatch; selector exhaustive `65536` masks / `81` legal tuples / `0` mismatch; full-top 1k and 10k `0` mismatch; PatternID >15, seventh pivot, Hybrid >7, and physical columns 1 versus 257 all PASS.  Production RTL semantic change after the functional closure is `NO`.

Latency is five cycles.  The 10,000-case formal GENERAL control corpus has histogram `5:10000`, zero functional mismatches, zero stale-generation mix, zero earlier-SA replay, zero partial-scan finalization, and zero old candidate acceptance.  Its `0` repairable / `10000` unrepairable mix means it is a latency/control characterization, not a repair-rate result.  The independent six-case directed repairable/N3-specific sidecar also measured five cycles.

## Matched 20 ns characterization

DC W-2024.09-SP2; TSMC018 `slow.db`; slow corner; 20.0 ns; zero I/O delay; `compile -map_effort low`; `NAND2X1 = 9.979200 um²`.

| Metric | Value |
|---|---:|
| Total area / GE | 685591.004318 um² / 68702.0006 |
| Comb / seq area | 640132.420814 / 45458.583504 um² |
| Total cells | 27448 |
| DESIGN_WNS_NS / TNS_NS | −24.41 / −2736.25 |
| Critical path / slack | 44.23 ns / −24.41 ns |
| Setup violations | 119 |
| Max-cap / transition / fanout violations | 251 / 0 / 155 |
| 20 ns setup | FAILED |

Critical path classification is `MIXED`: `core/active_sa_q_reg[1]` → slot decode → analyzer → `core/candidate_store/store_q_reg[12]`.  It must not be described as analyzer-only.  The 20 ns setup failure is retained as baseline evidence and is not an authorization for RTL optimization.

## Scaling interpretation

The canonical N2 comparator is `G2X2_N2_RC_GROUP_CA_LIVE`: 171399.415025 um², 17175.6669 GE, design WNS `0.00 ns`, critical path `19.88 ns`, critical-path slack `+0.02 ns`, and five-cycle policy latency.  The `+0.02 ns` field is critical-path slack, not Design WNS.

N3/N2 area ratio is `3.999961168`; comb-area ratio `4.618080`; seq-area ratio `1.386567`; critical-path ratio `2.224849`; policy latency change `0` cycles.  N3 retains the four-Config/five-cycle policy schedule, while cost growth is primarily combinational rather than additional policy cycles.  These results apply only to this matched architecture point and synthesis boundary.

## Evidence authority and historical records

Current source-freeze and DC authority is `provenance/N3_RC_GROUP_SOURCE_FREEZE.md`, `N3_RC_GROUP_20NS_SYNTHESIS_SUMMARY.md`, `N3_RC_GROUP_20NS_PPA.csv`, and the N2→N3 scaling CSV/Markdown.

Some earlier phase documents and the formal-latency manifest intentionally say `SOURCE_FREEZE=NOT_STARTED`, `DC_RUN=NO`, or `COMMIT=NOT_CREATED`.  Those statements are preserved, hash-bound records of their own execution time; they are not current baseline status and must not be edited retroactively.  This milestone record is the current-state authority that supersedes only that phase-status interpretation, not the immutable earlier evidence data.

No other clock period, N3 R-GROUP implementation, priority study, push, tag, or merge is part of this baseline.
