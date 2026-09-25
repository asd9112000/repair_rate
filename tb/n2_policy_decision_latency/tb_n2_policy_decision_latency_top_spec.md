# N2 Policy-Decision Latency Testbench Contract

`tb_n2_policy_decision_latency_top` is a non-synthesizable verification
boundary. It fans one caller-owned, finalized 204-bit analyzer summary into
the frozen N2 EARLY and GROUP production tops without modifying either RTL.

The testbench samples rising edges. The edge accepting `start_i && !busy_o`
is cycle zero. A later event at edge `k` has latency `k - 0`. Inputs are held
constant from reset release through terminal completion. The wrapper has no
fault stream, BIST, device scheduler, or timing-backannotation interface.

```json
{"signal":[{"name":"clk_i","wave":"p..."},{"name":"start_i","wave":"010...."},{"name":"EARLY commit","wave":"0.1.0.."},{"name":"EARLY done","wave":"0...1.."},{"name":"GROUP done","wave":"0................1"}]}
```

The result is final-snapshot policy-decision latency only. It excludes BIST,
raw-fault collection, online CAM allocation, and device-level scheduling.
