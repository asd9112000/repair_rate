# P3-BL-RTL challenges

```text
CLAIM: SYN-B reaches a boundary comparable to SYN-A.
POSSIBLE_CHALLENGE: EARLY commits per accepted SA while GLOBAL commits only after full search.
DEFENSE: Both tops include collector/analyzer-side input and persistent committed physical resource state; only policy-defined internal timing differs.
```

```text
CLAIM: One analyzer is shared rather than replicated.
POSSIBLE_CHALLENGE: Sequential enumeration increases latency.
DEFENSE: The phase freezes correctness first, records sixteen producer requests/cycles, and retains the shared-analyzer architectural intent.
```

```text
CLAIM: OPT1 remains optional without PPA contamination.
POSSIBLE_CHALLENGE: Supporting OPT0 and OPT1 might create a runtime mux.
DEFENSE: Separate tops fix a generate-time USE_OPT1 constant; no runtime selection port or mux exists.
```

```text
CLAIM: Raw candidate state remains observable.
POSSIBLE_CHALLENGE: Two 480-bit maps increase integrated state.
DEFENSE: Producer retention and frozen-core capture coexist during search and are counted explicitly; OPT2/OPT3 compression was not started.
```

```text
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_STREAMING_EARLY
CHALLENGE: Compact SA-ID arithmetic can accidentally introduce toroidal/wraparound connectivity that does not exist in a physical linear topology.
DESIGN_IMPLICATION: For small fixed physical sharing graphs such as LINE1X4_SINGLE_HOP, explicit adjacency encoding is safer and more auditable than relying on modulo/index-difference arithmetic.
VERIFICATION_IMPLICATION: Directed adjacency tests alone should include boundary nodes, and end-to-end oracle comparison is valuable because this bug passed earlier local semantic reasoning but was exposed by shared C++↔RTL corpus testing.
CORRECTED_RTL_REFERENCE: rtl/dss_1x4/policy/recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_core.v
```

```text
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
CHALLENGE: A GROUP_GLOBAL search must evaluate the whole A→B→C→D tuple without exposing a successful prefix as committed state.
DEFENSE: The DFS holds private prefix ledgers and the atomic commit validates A/B/C/D in shadow before one publish edge changes persistent ownership.
VERIFICATION_IMPLICATION: The shared C++↔RTL corpus compares selected tuple, lexicographic objective, final owners, boundary adjacency, and no-forwarding behavior; directed atomic tests independently check no partial visibility.
SCOPE_BOUNDARY: No 2×2 action/effect pruning, OPT2, OPT3, DC synthesis, or timing/area claim is included in this correctness-first baseline.
```

```text
PHASE: P3-4PT-GLOBAL-OPT-AUDIT
CHALLENGE: Why are different topology-specific optimization keys used for 2×2 and 1×4?
DEFENSE: The optimization class is identical—semantics-preserving pre-DFS candidate-equivalence collapse—but the downstream resource state differs by topology, so the minimal safe equivalence relation is architecture-specific.
SYN-B: (explicit_release, actual_release, actual_borrow) fully determines its frozen resource-effect transition at a depth.
SYN-D: equal (usedRows, usedColumns) fully determines the transition only together with the preserved physical prefix ledger; donor and owner are derived state, not removable key bits.
```

```text
PHASE: P3-4PT-GLOBAL-OPT-AUDIT
CHALLENGE: Why not compare optimized SYN-B against baseline SYN-D?
DEFENSE: That would confound topology cost with optimization strength; the formal four-point comparison uses the highest common verified optimization class.
RESULT: Both GLOBAL synthesis points use COMMON_OPT1, with separate static elaborations and no runtime optimization-selection mux.
SCOPE_BOUNDARY: OPT2/DFS dominance, OPT3/state compression, and DC synthesis were not started by this audit.
```
