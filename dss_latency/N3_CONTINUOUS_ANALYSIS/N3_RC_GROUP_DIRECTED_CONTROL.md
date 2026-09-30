# N3 RC-GROUP directed CA-LIVE control evidence

Permanent harness:
`tb/n3_continuous_analysis/tb_n3_rc_group_control.cpp`.

It drives a fault update during the active SA's second configuration,
simultaneously presents an old candidate, and checks restart at slot zero and
the suppressed store location.  It then completes A through D and checks that
earlier frozen SA results persist while later SA work starts.

```
N3_RC_GROUP_CONTROL_PASS update_wins=PASS old_write=PASS \
partial_finalize=FORBIDDEN earlier_sa_replay=0 stale_generation_mix=0 \
generation_tags=ABSENT
```

```
DIRECTED_CA_LIVE_CONTROL: PARTIAL (Config #2 update and completion path covered; Config #1, #3, and #4 update cases remain open)
FAULT_UPDATE_WINS: PASS (covered at Config #2)
SAME_EDGE_OLD_RESULT_SUPPRESSED: PASS (candidate-write boundary)
OLD_CANDIDATE_WRITE_SUPPRESSED: PASS
PARTIAL_SCAN_FINALIZATION: FORBIDDEN_AND_VERIFIED
EARLIER_SA_REPLAY: 0
STALE_GENERATION_MIX: 0
GENERATION_TAGS: ABSENT
```
