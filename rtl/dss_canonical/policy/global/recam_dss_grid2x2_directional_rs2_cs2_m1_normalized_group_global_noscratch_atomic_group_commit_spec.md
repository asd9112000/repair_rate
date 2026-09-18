# Atomic group commit specification

The commit unit consumes a complete selected four-SA tuple. It validates and stages A, B, C, and D against private shadow release, borrower, owner, and future-obligation state. After four successful stages, one publish edge updates the persistent ledger and pulses `commit_accepted_o`. An illegal stage discards shadow state, leaves persistent state unchanged, and pulses `commit_error_o`.

The persistent ledger is single-group state. A further start without reset after a nonempty persistent commit reports `commit_error_o`; the associated GLOBAL search core has no input for a nonempty pre-state.

```json
{"signal":[{"name":"start_i","wave":"010......"},{"name":"busy_o","wave":"0.1....0"},{"name":"commit_accepted_o","wave":"0.....10"},{"name":"resource_released_o","wave":"=.....=.","data":["old","new"]}]}
```
