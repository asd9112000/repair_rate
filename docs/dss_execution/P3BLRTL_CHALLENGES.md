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
