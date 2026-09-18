# P3-BL-IF — common versus architecture-specific blocks

| Block | 2x2 Directional | 1x4 Single-Hop | EARLY | GLOBAL | Reusable? | Classification and reason |
|---|---|---|---|---|---|---|
| Collector snapshot interface | Existing port set | Required future port set | Yes | Yes | **CONDITIONAL** | `COMMON` logical snapshot; current producer/analyzer binding proves only 2x2 RTL reuse. |
| RECAM/shared analyzer | `recam_shared_config_analyzer` exists | No RTL binding | Yes | Candidate producer | **ADAPT** | `RESOURCE_POINT_SPECIFIC`: fault payload may be reused, but its seven 2x2 ConfigIDs/action interpretation is not a 1x4 candidate contract. |
| ConfigID/action mapping | `dss_v2_group_slot_decode` | C++ capacity attempts, no RTL ConfigID map | Yes | Yes | **NO** | `TOPOLOGY_SPECIFIC` and `POLICY_SPECIFIC`; 1x4 must not inherit L/R/B/RB. |
| Candidate-map generation | 160-entry map is a core input only | No RTL | N/A one request at a time | Required | **NO** | `INTEGRATION_SPECIFIC`; a 2x2 map producer is still missing. |
| Candidate-effect generation | valid/release/borrow maps consumed by 2x2 core | C++ demand `(usedRows, usedColumns)` and ledger allocation | Yes | Yes | **NO** | `TOPOLOGY_SPECIFIC`; 1x4 demand and transfer metadata cannot be compressed to 2x2 bits. |
| Resource ledger | Four directional resources, existing SV ledger | Neighbor-owned shareable rows, C++ only | Commit per accepted SA | Atomic full tuple commit | **NO direct RTL reuse** | `TOPOLOGY_SPECIFIC`; abstract transaction/visibility is common. |
| Topology legality checker | `dss_topology_2x2_directional` exists | Adjacent A-B-C-D rows only, C++ ledger | Yes | Search/commit | **NO** | `TOPOLOGY_SPECIFIC`: 1x4 middle donor order and no-transitive rule differ. |
| EARLY controller skeleton | Existing four-SA state/rank progression | Needed | Yes | No | **ADAPT** | `POLICY_SPECIFIC`: generic sequencing may be reused only behind a topology response interface. |
| GLOBAL DFS core | Existing fixed 2x2 DFS | Needed | No | Yes | **NO direct RTL reuse** | `POLICY_SPECIFIC` plus topology state; 1x4 needs demand/neighbor state and independent pruning proof. |
| Selected-tuple commit logic | Missing integrated adapter | Missing | No | Yes | **COMMON role, NEW implementations** | `INTEGRATION_SPECIFIC`: both require atomic all-or-nothing external commit. |
| Collector-to-result top | SYN-A exists | Missing | Yes | Required | **NO** | `INTEGRATION_SPECIFIC` architecture-specific top. |

`COMMON` means a stable responsibility/handshake, not source-file reuse. Only
a module with no layout, topology, RS/CS/m, or policy dependency may omit
architecture identity from its name.
