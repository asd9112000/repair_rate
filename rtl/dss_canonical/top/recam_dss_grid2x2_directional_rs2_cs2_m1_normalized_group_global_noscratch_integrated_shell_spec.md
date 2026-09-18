# Integrated shell specification

The shell captures four flattened collector snapshots on an accepted start, drives the shared sequential candidate producer, elaborates exactly one search core through `USE_OPT1`, and invokes atomic group commit only after a successful search. `USE_OPT1` is a generate-time parameter, never a runtime selector.

Outputs publish a successful selected tuple only after commit acknowledgement. A failed search reports `done_o` with `group_repairable_o=0`; a commit error likewise reports failure while retaining the pre-start persistent ledger.

```json
{"signal":[{"name":"start_i","wave":"010........................"},{"name":"busy_o","wave":"0.1......................0"},{"name":"done_o","wave":"0........................1"},{"name":"group_repairable_o","wave":"0........................1"}]}
```
