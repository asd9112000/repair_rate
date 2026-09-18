# P3-BL-IF — candidate and policy interfaces

## Collector/analyzer input contract

The upstream snapshot is sampled when an integrated top accepts `start_i` and remains stable through candidate production.

| Field | Existing SYN-A port | Meaning |
|---|---|---|
| Transaction | `clk_i`, synchronous active-low `rst_ni`, `start_i` | Start is accepted only while not busy. |
| Pivot state | `pivot_valid_i[4:0]`, `pivot_rows_flat_i`, `pivot_cols_flat_i` | Compact valid pivot prefix and addresses. |
| Threshold state | `row_gt{1,2,3}_i[4:0]`, `col_gt{1,2,3}_i[4:0]` | RECAM matrix relation bits. |
| Hybrid state | `hybrid_valid_i`, `hybrid_pointer_flat_i`, `hybrid_descriptor_i`, `hybrid_differing_flat_i` | Additional pivot relation records. |
| Overflow | `conventional_overflow_i` | A candidate cannot bypass collector overflow. |
| Config request | `current_config_id_o[2:0]` in SYN-A | Selects a local configuration interpretation. |
| Analyzer result | `candidate_valid_o[9:0]`, `pattern_id_o[3:0]`, `solution_valid_o`, `repairable_o`, `dictionary_overflow_o` | Per-config candidate results. |

`ROW_ADDR_W=9`, `COL_ADDR_W=5`, `DIFF_ADDR_W=9`, and `HYBRID_ENTRIES=7` are the existing SYN-A defaults.
For the frozen RS2/CS2/m1 comparison, a 1x4 top must accept this same logical snapshot or publish an interface-revision review before synthesis.
The raw snapshot is topology-independent, but analyzer repairability is local RECAM feasibility only: configuration-to-demand/action and topology acceptance are not topology-independent.

## Minimum common candidate semantic record

| Field | Frozen meaning | 2x2 Directional | 1x4 Single-Hop |
|---|---|---|---|
| `candidate_valid` | Local RECAM candidate exists. | Map bit. | Candidate exists for capacity attempt and PatternID. |
| `sa_id` | A/B/C/D owner; 2 bits. | DFS depth. | Traversal identity. |
| `candidate_token` | Stable producer-local identity. | Action plus one-based PatternID. | Capacity-attempt identity plus one-based PatternID. |
| `order_key` | Policy total order. | R,L,RB,B then PatternID. | Local capacity, increasing extra-row capacity, then PatternID for EARLY. |
| `pattern_id` | One-based RECAM identity; 4 bits at RS2 analyzer boundary. | Existing core output. | C++ `candidateIndex + 1`. |
| `local_demand` | Required rows/columns before topology allocation. | New producer derives effects from it. | Mandatory `(usedRows, usedColumns)`; columns remain local. |
| `transition_metadata` | Input to topology adapter. | Explicit/actual release and actual borrow. | Donor availability/identity and row-only demand. |
| `mapping_ref` | Selected-remap reconstruction identity. | Action/config/PatternID. | PatternID plus capacity attempt. |

No candidate field may be inferred from a 2x2-only encoding. `candidate_token` and `local_demand` are architecture-specific parameterized buses; P3-BL-RTL must derive their widths from a reviewed RS2/CS2/m1 candidate table, not the 2x2 160-entry layout.

## Topology transition result

```text
candidate_valid, candidate_token, local_demand, transition_legal,
release_required, release_resource_valid, release_resource_id,
borrow_required, donor_set_valid, donor_set / selected_donor_id,
future_obligation, mapping_ref
```

For 2x2 GLOBAL, raw-map fields are `candidate_valid`, `candidate_release`, and `candidate_borrow`; action derives `explicit_release`.
The safe OPT1 key remains `(explicit_release, actual_release, actual_borrow)` and is neither the common candidate interface nor a 1x4 key.
For 1x4, donor identity is mandatory before allocation; a one-bit `actual_borrow` cannot replace it.
1x4 `explicit_release` cannot be inferred from a 2x2 action because 1x4 uses capacity attempts, not L/R/B/RB actions.

## EARLY handshake

```text
policy -> producer/adapter: candidate_request_valid, sa_id, order_key
producer/adapter -> policy: candidate_response_valid, candidate record, transition result
policy -> commit adapter: commit_request_valid, selected candidate transaction
commit adapter -> policy: commit_accepted or commit_error, committed ledger view
policy -> top: next_sa or failure_position, done, group_repairable
```

The controller advances only after a response; it advances to the next SA only after `commit_accepted`.
Donor priority belongs to topology/ledger logic, not a generic EARLY controller.
SYN-A is an existing 2x2 realization with combinational implicit response rather than named ready/valid ports.

## GROUP_GLOBAL handshake

```text
candidate producer -> search: complete per-SA candidate records
search -> commit adapter: tuple_valid, four selected records, final speculative obligation state
commit adapter -> search/top: group_commit_accepted or group_commit_error, committed ledger snapshot
```

Search depth is four for both layouts (`A -> B -> C -> D`).
2x2 identity is action/config/PatternID and its future obligation is the four-bit release requirement mask.
1x4 identity is capacity-attempt plus PatternID; its future-donor state is adjacent row ownership, not the 2x2 release mask.
The 1x4 GLOBAL objective is minimum borrowed rows, minimum used rows, lexicographic PatternID tuple, then lexicographic attempt tuple.
Equal `(usedRows, usedColumns)` candidates retain the lowest `(PatternID, attemptIndex)` before search, as recorded in R2F.
