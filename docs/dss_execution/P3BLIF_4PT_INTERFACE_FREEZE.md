# P3-BL-IF — four-point integrated baseline interface freeze

## Status and scope

```text
P3BLIF_STATUS: COMPLETE
SCOPE: architecture/interface audit, boundary freeze, and RTL backlog only
PRODUCTION_RTL_MODIFIED: NO
OPT2_STARTED: NO
OPT3_STARTED: NO
DC_SYNTHESIS_STARTED: NO
WITHSCRATCH_STARTED: NO
RS3_STARTED: NO
FORMAL_100K_TOUCHED: NO
DEVICE_SWEEP_STARTED: NO
```

This is the implementation contract for the later `P3-BL-RTL` phase. It is
not a claim that SYN-B, SYN-C, or SYN-D RTL exists today, and it does not
authorize their implementation in this phase.

## Frozen comparable boundary

Every later four-point synthesis top must include this whole path:

```text
stable collector snapshot
  -> RECAM candidate producer
  -> topology candidate-transition adapter
  -> EARLY controller or GROUP_GLOBAL search
  -> complete selected solution
  -> commit adapter
  -> committed physical resource ledger
```

The synthesis top boundary is therefore the collector-snapshot ports on the
input side and the registered group result plus committed-ledger observation on
the output side. A policy-search core alone is not a comparable synthesis
point. The architecture identity, including layout, topology, `RS=2`, `CS=2`,
`m=1`, policy, no-scratch mode, and any implemented optimization level, must
be present in every architecture-specific top/module name and manifest.

The four frozen identities are:

```text
GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_STREAMING_EARLY
GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_STREAMING_EARLY
LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
```

## Common transaction stages

| Stage | Frozen responsibility | Commonality |
|---|---|---|
| Collector snapshot | Hold one SA-local RECAM fault snapshot stable for its candidate evaluation. | Logical contract is common; existing RTL implementation is 2x2-only. |
| Candidate producer | Enumerate locally repairable candidates and retain canonical identity and local demand. | Common role; configuration/action mapping is architecture-specific. |
| Transition adapter | Translate local demand into topology-specific release, borrow, donor, and future-obligation facts. | Topology-specific. |
| EARLY / GLOBAL policy | Select candidates under the stated policy contract. | Controller/search differs by policy, not by collector format. |
| Commit adapter | Convert an accepted EARLY candidate or complete GLOBAL tuple into ledger transactions. | Common atomicity rule; transaction encoding is topology-specific. |
| Physical ledger | Publish only committed ownership/release state. | Same abstract role; existing 2x2 RTL is not a 1x4 implementation. |

## Policy and visibility rules

`STREAMING_EARLY` evaluates one candidate request at a time in SA order
`A -> B -> C -> D`. Each accepted candidate commits on its acceptance edge;
earlier commits remain visible and are never rolled back. If an SA exhausts its
ordered candidates, the group fails at that SA.

`GROUP_GLOBAL` searches candidates without mutating the physical ledger. It
may retain speculative internal state, including a future-owner obligation, but
that state is not committed resource state. It exposes a success only after a
complete A/B/C/D tuple is selected and an atomic group commit succeeds. No
partial tuple state may be externally visible as committed ledger state.

## Boundary decisions

```text
COMMON_INTEGRATED_BOUNDARY_FROZEN: YES
2X2_GLOBAL_INTEGRATION_CONTRACT_FROZEN: YES
1X4_SINGLE_HOP_EARLY_CONTRACT_FROZEN: YES
1X4_SINGLE_HOP_GLOBAL_CONTRACT_FROZEN: YES
GLOBAL_COMMIT_INTERFACE_FROZEN: YES
COMMON_CANDIDATE_SEMANTICS_FROZEN: YES
```

The common candidate contract freezes meanings and handshakes, not an invalid
claim that 2x2's three one-bit effects are sufficient for 1x4. Its topology
adapter is deliberately parameterized by the architecture-specific candidate
descriptor defined in `P3BLIF_CANDIDATE_INTERFACE.md`.

## Evidence

* `rtl/dss_canonical/top/recam_dss_canonical_rs2_streaming_early_top.v` is
  the existing integrated SYN-A reference boundary.
* `rtl/dss_canonical/policy/global/recam_dss_canonical_global_noscratch_core.v`
  is a search-only SYN-B core: it captures a 160-entry map plus two effect
  bitmaps and does not instantiate a physical ledger.
* `rtl/dss_v2/resource/dss_v2_resource_ledger.sv` establishes the existing
  one-transaction committed-ledger behavior used by SYN-A.
* `src/DynamicRepairSimulator.cpp` and `src/PhysicalResourceLedger.cpp` are
  semantic provenance for 1x4, not hardware evidence.
