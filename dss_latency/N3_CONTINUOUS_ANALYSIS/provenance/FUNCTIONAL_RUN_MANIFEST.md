# N3 RC-GROUP functional run manifest

```
WORKTREE: /home/asd9112000/repair_rate_date2026_n3
BRANCH: integration/date2026-n3-ca-live-scaling
BASELINE_COMMIT: a2a61b96280275019ab06f9e120c4951466b9777
SIMULATOR: Verilator 4.028
```

| Check | Command result |
| --- | --- |
| structural compile | PASS |
| fixed-mask analyzer directed | PASS, 9 cases, 0 mismatch |
| CA-LIVE control directed | PASS |
| full-top 1k | PASS, seed `0x4e335247`, 0 mismatch |
| full-top 10k no-conflict | PASS, seed `0x4e335247`, 0 mismatch |

Generated build directories are intentionally in `/tmp` and are not part of
the experiment archive.  The three C++ harnesses are permanent source inputs.
