# S3-SIM-MAINT-01: formal sweep policy compatibility

Status: complete maintenance note. Scope: legacy **group-level**
`DynamicSpareSharing` R3 runner only; no device-level, RTL, or synthesis
semantics are changed.

## Root cause and compatibility contract

The R3 descriptor supplied legacy CLI invocation names while the C++ producer
emitted normalized `SimulationConfig::toString(SolutionTakePolicy)` names in
`implementation_policy_id`, `policy_id`, and `solution_policy`. For example,
the descriptor invoked `directional_v2_early`, but the sidecar correctly wrote
`normalized_local_first`. The prior exact-string validation therefore rejected
an equivalent `DIRECTIONAL_V2` LOCAL_FIRST sidecar.

`POLICY_ID_CONTRACT_FROZEN: YES`

The authoritative mapping in
`scripts/group/r3_formal_group_repair_rate.py` is deliberately closed:

| Legacy invocation / old sidecar name | Canonical emitted implementation ID |
|---|---|
| `directional_v2_early`, `directional_m1_v2_early` | `normalized_local_first` |
| `group_greedy_rtl_canonical` | `normalized_streaming_early` |
| `directional_v2_group_global`, `directional_m1_v2_group_global` | `historical_directional_v2_global` |
| each existing `one_by_four_*_v1` name | itself |

The experiment-facing `canonical_policy_id` (such as
`directional_m1_local_first`) remains separate from both the display/invocation
name and the stable emitted implementation ID. The manifest now records the
separate `display_name` for every scheduled policy. Resume accepts the audited
aliases only after exact checks of canonical experiment ID, layout, topology,
row/column sharing, N/RS/CS/F_GROUP/seed, corpus ID, and every policy semantic
field. Unknown aliases and different EARLY/GLOBAL, topology, resource-point,
or donor/priority contracts still fail. Existing sidecars need no migration
and are never rewritten automatically.

## F_GROUP contract

`CANONICAL_F_GROUP_LIST = (8, 12, 16, 20, 24, 28, 32)` remains the immutable
historical default for every supported N. `--f-group-list` is an authoritative
runtime override only; it accepts positive, distinct comma-separated integers
in the supplied deterministic order, including
`4,8,12,16,20,24,28,32,36,40`. It rejects empty elements, non-integers, zero,
negative values, and duplicates. An extended list is an experiment choice,
not a modification of the historical definition.

## Validation

`tests/r3_formal_sweep_policy_contract_test.py` covers a renamed equivalent
sidecar, a non-equivalent implementation ID, a topology mismatch, and valid /
invalid F_GROUP lists. A small fresh formal sweep and a resume of its sidecars
exercise producer output and existing-sidecar reuse.
