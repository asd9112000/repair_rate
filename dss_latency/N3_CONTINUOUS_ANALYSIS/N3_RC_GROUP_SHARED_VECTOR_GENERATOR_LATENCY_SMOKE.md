# N3 RC-GROUP Shared Vector Generator and Latency Smoke

## Scope

Layer C adds deterministic verification infrastructure only. It creates a shared full-top vector generator and connects the existing standalone latency executable to the closed Layer-A analyzer oracle and Layer-B selector/reconstruction oracles. Production RTL is unchanged. This run is a 100-case smoke, not the formal 10,000-case latency characterization and not paper-facing data.

## Generator contract

The shared files are:

- `tb/n3_continuous_analysis/n3_rc_group_vector_generator.hpp`
- `tb/n3_continuous_analysis/n3_rc_group_vector_generator.cpp`

The generator accepts an explicit 32-bit seed, 64-bit case index, and generation mode. It uses no wall clock, random device, or hidden mutable global state. Repeating seed `0x4e33524f`, case index 17, and `GENERAL` mode produced byte-equivalent semantic state, candidate map, selector result, retained state, and reconstruction result.

Supported modes are `GENERAL`, `REPAIRABLE`, `UNREPAIRABLE`, `HIGH_PATTERN`, `SEVENTH_PIVOT`, `HYBRID_GT_7`, and `PHYSICAL_COLUMN_WIDE`. Expected values are calculated only through:

```text
Layer-A analyzer oracle
-> Layer-B GROUP selector oracle
-> Layer-B reconstruction oracle
```

No production RTL output is copied into an expected result. The generated case contains all four 524-bit analyzer semantic states, the 16-record candidate map, selected Config/action/PatternID/resource metadata, 656-bit reconstruction semantics, and expected line-valid/type/full-address results.

## Latency boundary and marker convention

The measured boundary is:

```text
START: rising edge that accepts the final active-SA state update
END:   rising edge that registers current-generation solution_ready_o
latency_cycles = END cycle - START cycle
```

It does not begin at raw fault arrival or BIST detection. Cycle zero is not assigned to a signal edge; each accepted rising edge increments the harness counter once.

Externally observable markers are the final-SA accepted update, four candidate-store record updates, and registered `solution_ready_o`. Their measured offsets are `+1`, `+2`, `+3`, `+4`, and `+5` cycles from START. The internal combinational selector decision has no dedicated top-level marker and is recorded as `NOT_EXTERNALLY_OBSERVABLE`; production RTL was not modified to expose it.

The harness also checks that solution-ready stays low during all four Config writes, the update edge does not write an old candidate, earlier-SA records remain unchanged while later SAs scan, all four frozen bits belong to the current reset-delimited case, and final selector/resource/reconstruction outputs match the shared oracle chain.

## Build and smoke

The harness was generated with Verilator using the seven frozen N3 production RTL sources and normal linkage to:

- `n3_rc_analyzer_oracle.cpp`
- `n3_rc_group_selector_oracle.cpp`
- `n3_rc_reconstruction_oracle.cpp`
- `n3_rc_group_vector_generator.cpp`
- `tb_n3_rc_group_latency.cpp`

Every object was force-rebuilt from current sources before the final run. Smoke command:

```text
/tmp/n3_layer_c_latency/Vrecam_dss_n3_rc_group_live_state_top --seed 0x4e33524f --cases 100
```

Result:

```text
N3_RC_GROUP_LATENCY_SMOKE_PASS cases=100 seed=0x4e33524f functional_mismatches=0 repairable=5 unrepairable=95 deterministic_replay=PASS seventh_pivot=PASS hybrid_gt_7=PASS pattern_id_gt_15=PASS physical_column_wide=PASS latency_event_markers=PASS stale_generation_mix=0 earlier_sa_replay=0 latency_histogram=5:100, classification=SMOKE_ONLY_NON_FORMAL
```

The directed smoke prefix includes the validated PatternID-20 witness, a selected seven-slot Config using the seventh pivot, Hybrid entry 8, physical columns 1 and 257, and both repairable and unrepairable cases. The remaining cases use deterministic `GENERAL` generation.

## Closed-layer regression

Layer A was rerun for 10,000 vectors with seed `0x4e33524f`: 0 mismatches, 16 feasible, and 9,984 unrepairable. Layer B selector exhaustively retained 0 mismatches across 65,536 valid masks and 81 legal tuples. Layer B reconstruction retained 0 mismatches for the seventh pivot, columns 1 versus 257, and address 8191. Existing Layer-A and Layer-B source hashes are unchanged.

The existing full-top and high-pattern test sources and dependency paths were not modified in Layer C, so their 1k/10k/PatternID-20 regressions did not require another run.

## Phase boundary

- Generator main count: 0.
- Latency executable main count: 1.
- No test `.cpp` inclusion or main workaround exists.
- Production RTL hashes are identical before and after Layer C.
- Latency histogram `5:100` is `SMOKE_ONLY_NON_FORMAL`.
- Formal latency characterization, source freeze, synthesis, staging, commit, push, and merge were not started.

