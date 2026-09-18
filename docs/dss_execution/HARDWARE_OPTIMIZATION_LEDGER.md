# Hardware Optimization Ledger

> Append-only evidence ledger.  Historical labels are retained verbatim and
> mapped to canonical semantics in a separate column.  A dash means the
> retained authoritative source did not freeze that field.

> **P1-NAME annotation (2026-09-18):** this ledger's Canonical Streaming EARLY
> and Canonical GLOBAL-NoScratch rows use the active names Normalized Streaming
> EARLY and Normalized GLOBAL-NoScratch. Historical V2/Phase-3J labels below
> remain provenance names; the authoritative mapping is
> `CANONICAL_NAMING_MAP.md`.

## Common accepted methodology

Unless an entry explicitly says otherwise:

```text
tool: Synopsys Design Compiler W-2024.09-SP2
technology: TSMC018 Arm CBDK slow.db, slow corner
clock period: 20.0 ns
IO delay: zero
compile: compile -map_effort low
GE divisor: slow/NAND2X1 = 9.979200
```

Area comparisons are valid only between entries with the same methodology and
synthesis boundary.

## Historical/reference entries

| Architecture / lineage | Policy interpretation | RS/CS | Storage | Revision / provenance | Synthesis boundary | Area | GE | Comb. area | Seq. area | Seq. cells | Local architectural bits | Reused scratch bits | WNS | Critical path | Decision cycles | Functional status |
|---|---|---:|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---|---|
| RECAM analyzer baseline | analyzer only; no canonical sharing policy | 2/2 | N/A | retained P0-B reference | analyzer | 50,877.29 | ~5,098.33 | — | — | — | — | 0 | 0.00 ns | — | combinational | historical evidence |
| Phase 3J historical EARLY | noncanonical B/C `L,B,R,RB` | 2/2 | STREAMING | exact RTL revision NOT_FROZEN | analyzer + controller + ledger | 117,219.01 | 11,746.33 | — | 25,882.72 | 485 | legacy inclusive state | 0 | +0.01 ns | — | variable, ≤16 action checks | verified historical policy; not canonical |
| Phase 3J historical GROUP | normalized EARLY at decision/resource boundary | 2/2 | historical retained | exact RTL revision NOT_FROZEN | analyzer + controller + ledger | 117,139.18 | 11,738.33 | 91,256.46 | 25,882.72 | — | legacy inclusive state | 0 | +0.01 ns | — | collect then sequential first-legal | verified historical boundary |
| V2 specialized EARLY | noncanonical B/C `L,B,R,RB` | 2/2 | STREAMING | Phase 4E retained result | analyzer + controller + ledger | 80,871.44 | 8,104.00 | 76,769.99 | 4,101.45 | 75 | 73 decision-boundary bits | 0 | +0.01 ns | `sa_q[1]` to ledger borrower ID | ≤16 action checks | 1,000 random, historical policy |
| V2 GROUP-NoScratch | normalized EARLY at decision/resource boundary | 2/2 | NOSCRATCH history | Phase 4F retained result | analyzer + collect/controller + ledger | 105,393.66 | 10,561.33 | 96,611.96 | 8,781.70 | 157 | 80 candidate-history bits; 157 inclusive | 0 | +0.00 ns | 19.74 ns path | 16 collect + ≤16 allocate | 1,000 random, 0 mismatch at audited boundary |
| RS3 original generic GROUP-NoScratch | normalized EARLY at decision/resource boundary | 3/3 | NOSCRATCH history | `3304c86`; H5 retained reports | analyzer + collect/controller + ledger | 12,053,273.51 | 1,207,839.66 | 12,042,276.44 | 10,997.08 | 197 | 112 candidate-history bits; 197 inclusive | 0 | -44.24 ns | 64.14 ns, core state to store | 16 collect + ≤16 allocate | H4 1,000 random; timing fail |
| RS3 fixed-table GROUP-NoScratch | normalized EARLY at decision/resource boundary | 3/3 | NOSCRATCH history | `ff4e9a7`; P0-C retained `/tmp` report | analyzer + collect/controller + ledger | 546,667.23 | 54,780.67 | — | — | — | 112 candidate-history bits | 0 | -23.29 ns | 43.20 ns, core state to store | no added latency | retained functional lineage; timing fail |
| RS3 one-stage P1 GROUP-NoScratch | normalized EARLY diagnostic | 3/3 | NOSCRATCH history | `721d539`; P0-C retained `/tmp` report | analyzer + pipelined controller + ledger | 527,693.45 | 52,879.33 | — | — | — | — | 0 | -12.52 ns | 32.41 ns, core state to matrix prep; 169 levels | +1 analysis cycle | retained P1 evidence; timing fail |

## Canonical implementation entries

| Architecture | Policy | RS/CS | Storage | Git SHA | Source manifest | Boundary | Area / GE | State accounting | WNS | Decision cycles | Functional status |
|---|---|---:|---|---|---|---|---|---|---|---|---|
| Canonical Streaming EARLY core | NORMALIZED_STREAMING_EARLY | 2/2 | STREAMING | working tree on `3304c86` | canonical core + V2 packages/table/topology/feasibility/ledger/diagnostic | candidate-map + policy + physical ledger | NOT_RUN | no candidate history; common 6-bit PatternID trace | NOT_RUN | max 16 candidate checks after start | E1–E5 + 1,000 seeded random, 0 mismatch |
| Canonical Streaming EARLY RS2 top | NORMALIZED_STREAMING_EARLY | 2/2 | STREAMING | working tree on `3304c86` | prior row plus `recam_shared_config_analyzer.sv` and canonical RS2 top | analyzer + policy + physical ledger | NOT_RUN | no candidate history | NOT_RUN | max 16 analyzer/action checks | compile/lint PASS; analyzer regression PASS |
| Canonical Streaming EARLY RS2 top (2026-09-18 attempt) | NORMALIZED_STREAMING_EARLY | 2/2 | STREAMING | working tree on `3304c86`; manifest SHA256 `555f54e9565aa696b3da512f9baffbdbf8cc4587757656ccae378f268118e62b` | `recam_canonical_rs2_early_sources.tcl` | analyzer result through committed physical ledger | BLOCKED: no mapped report / GE | architectural state and cycles unchanged from functionally closed row above | BLOCKED | max 16 analyzer/action checks | Verilator lint PASS; DC W-2024.09-SP2 launched, then `DCSH-1 Design Compiler is not enabled` before analyze/map |
| Canonical Streaming EARLY RS2 top (2026-09-18 completed run) | NORMALIZED_STREAMING_EARLY | 2/2 | STREAMING | working tree on `3304c86`; manifest SHA256 `555f54e9565aa696b3da512f9baffbdbf8cc4587757656ccae378f268118e62b` | `recam_canonical_rs2_early_sources.tcl` | analyzer result through committed physical ledger | 83,831.933613 / 8,400.67 GE; comb. 78,819.048729; seq. 5,012.884884; 91 seq. cells | 73 policy/trace bits plus 18 physical-ledger bits = 91 boundary bits; no candidate history | +0.01 ns; 19.79 ns; 75 levels; `policy_core/sa_q_reg[1]` to `policy_core/resource_ledger/borrower_valid_q_reg[0]` | max 16 analyzer/action checks | E1-E5 + W1 + 1,000 seeded random, 0 mismatch; mapped PASS |
| Canonical Streaming EARLY core | NORMALIZED_STREAMING_EARLY | 3/3 | STREAMING | working tree on `3304c86` | canonical core + RS3 config/topology + V2 ledger/diagnostic | candidate-map + policy + physical ledger | NOT_RUN | no candidate history; 6-bit PatternID | NOT_RUN | max 16 candidate checks after start | E1–E5 + 1,000 seeded random, 0 mismatch |
| Canonical GLOBAL-NoScratch RS2 core | NORMALIZED_GLOBAL | 2/2 | NOSCRATCH | working tree on `3304c86`; manifest SHA256 `a01832dbbfe578b1ebb07854dff38704b4db0b38afd7144a5eb32f034d6d91c0`; RTL SHA256 `40244bdc77546dc9f1c81c1aecf18c8fb8bb9d1885a8e90813d2ba242a2dd362` | `recam_canonical_rs2_global_noscratch_sources.tcl` | complete candidate/effect map through selected speculative tuple; committed ledger remains downstream | BLOCKED: no mapped report / GE | 630 architectural bits: 480 candidate/effect, 48 masks, 8 borrow-count snapshots, 24 cursors, 70 control/result | BLOCKED | min 4 candidate visits; bounded worst-case 2,625,640 visits | 6 directed + 1,000 seeded candidate maps, 0 mismatch; DC `DCSH-1` before mapping |
| Canonical GLOBAL-NoScratch RS2 core (2026-09-18 completed run) | NORMALIZED_GLOBAL | 2/2 | NOSCRATCH | working tree on `3304c86`; manifest SHA256 `a01832dbbfe578b1ebb07854dff38704b4db0b38afd7144a5eb32f034d6d91c0`; RTL SHA256 `40244bdc77546dc9f1c81c1aecf18c8fb8bb9d1885a8e90813d2ba242a2dd362` | `recam_canonical_rs2_global_noscratch_sources.tcl` | complete candidate/effect map through selected speculative tuple; committed ledger remains downstream | 61,751.290432 / 6,188.00 GE; comb. 26,917.228756; seq. 34,834.061676; 616 seq. cells | 630 architectural bits: 480 candidate/effect, 48 masks, 8 borrow-count snapshots, 24 cursors, 70 control/result | +0.02 ns; 19.87 ns; 41 levels; `depth_q_reg[0]` to `selected_release_q_reg[3]` | min 4 candidate visits; bounded worst-case 2,625,640 visits | 6 directed + 1,000 seeded candidate maps, 0 mismatch; mapped PASS |
| Canonical GLOBAL-WithScratch | NORMALIZED_GLOBAL | 2/2, 3/3 | WITHSCRATCH | — | — | — | NOT_IMPLEMENTED | 0 reused bits proven | NOT_RUN | — | BLOCKED: CandidateStore scratch contract absent |

Canonical RS2 EARLY synthesis was started with the accepted methodology, but
the installed W-2024.09-SP2 executable stopped before analysis/mapping because
the Design Compiler feature was not enabled (`DCSH-1`).  Therefore area, GE,
combination/sequential area, sequential cells, WNS, critical path, and logic
levels remain unavailable; no estimate is substituted for a mapped report.

## Optimization lineage

```text
legacy generalized design
  -> V2 specialized architecture
  -> RS3 fixed candidate representation (retained P0 evidence)
  -> canonical semantic controller correction (current EARLY)
  -> timing-oriented restructuring (not authorized in this phase)
  -> scratch-aware GLOBAL storage reuse (blocked; no scratch contract)
```

## 2026-09-18 canonical RS2 mapped-result deltas

The completed canonical EARLY top uses the same analyzer + controller +
physical-ledger boundary as the retained RS2 references. Relative to the V2
specialized historical EARLY (`80,871.44` area, `8,104.00` GE), canonical
EARLY adds `2,960.493613` area (`+3.6607%`) and `296.67` GE (`+3.6608%`). Its
sequential area adds `911.434884` (`+22.2223%`), mapped sequential cells rise
from 75 to 91 (`+16`), the 73 decision/trace bits are unchanged, WNS is
unchanged at `+0.01 ns`, and the maximum action-check count remains 16. This
baseline remains the **noncanonical historical streaming baseline**.

Relative to V2 GROUP-NoScratch (`105,393.66` area, `10,561.33` GE), canonical
EARLY reduces area by `21,561.726387` (`20.4583%`), GE by `2,160.66`
(`20.4582%`), and sequential area by `3,768.815116` (`42.9167%`); mapped
sequential cells fall from 157 to 91 (`-66`) and WNS improves by `0.01 ns`.
That reference is the **historical exact normalized-EARLY decision/resource
boundary with deferred candidate history**, not a streaming implementation.

Relative to Phase 3J historical GROUP (`117,139.18` area, `11,738.33` GE),
canonical EARLY reduces area by `33,307.246387` (`28.4339%`), GE by `3,337.66`
(`28.4339%`), and sequential area by `20,869.835116` (`80.6323%`); WNS is
unchanged at `+0.01 ns`. A state-bit or decision-cycle percentage is not
reported because that retained reference did not freeze those exact fields.

Canonical GLOBAL-NoScratch is a policy/search core ending at the selected
speculative tuple; canonical EARLY and V2 GROUP-NoScratch include the analyzer
and/or committed physical ledger. Their total-area, GE, and WNS numbers are
therefore **not boundary-comparable**, so no misleading percentage is
reported. The valid architectural comparison is that true complete search
retains 630 bits, including 480 candidate/effect-history bits, versus EARLY's
73 policy/trace bits (`+557` core bits) and no candidate history. Compared with
V2 GROUP-NoScratch's 80 candidate-history bits, canonical GLOBAL retains 400
additional candidate/effect-history bits. Search latency changes from EARLY's
maximum 16 checks to GLOBAL's minimum 4 and bounded worst case 2,625,640
candidate visits. The mapped GLOBAL core has 616 sequential cells and
34,834.061676 sequential area and meets 20 ns with `+0.02 ns` WNS.
