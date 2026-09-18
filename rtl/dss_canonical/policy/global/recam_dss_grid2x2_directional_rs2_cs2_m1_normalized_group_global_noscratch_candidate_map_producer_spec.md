# Candidate map producer specification

One `recam_shared_config_analyzer` enumerates sixteen requests: four distinct captured SA snapshots times stored actions L, R, B, RB. For each request it captures the complete ten-bit analyzer bitmap in `SA*40 + action*10 + PatternID-1` and derives descriptor-based effect bits. It produces 480 retained bits and pulses `done_o` after the sixteenth capture.

`start_i` is accepted only in IDLE. Input snapshots must remain stable throughout the build transaction. Storage ordering is not DFS priority; the frozen core still searches R, L, RB, B.

```json
{"signal":[{"name":"start_i","wave":"010................"},{"name":"active_sa_o","wave":"x=...=...=...=...","data":["A","B","C","D"]},{"name":"done_o","wave":"0................1"}]}
```
