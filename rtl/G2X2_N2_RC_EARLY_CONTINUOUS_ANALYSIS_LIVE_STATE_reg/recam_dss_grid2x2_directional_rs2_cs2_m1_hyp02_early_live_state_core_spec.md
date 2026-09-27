# EARLY-CA-LIVE core specification

The controller services one active SA in serial A-to-D order. A matching
`state_update_i` starts or restarts the canonical role-local priority scan.
`test_done_valid_i` is a one-cycle event for the active SA and is retained in
one control bit until a legal candidate is available. A state update has
priority over candidate acceptance and HOLD release.

No selected configuration or PatternID payload is registered: while in HOLD,
the unchanged analyzer output remains the pending solution.

```wavejson
{signal:[{name:'state update',wave:'10......'},{name:'scan rank',wave:'x234....'},{name:'candidate legal',wave:'0..10...'},{name:'BIST done',wave:'0....10.'},{name:'commit',wave:'0.....10'}]}
```
