# P3-BL-RTL-A — 2x2 GLOBAL candidate producer

```text
GROUP_SNAPSHOT_CONTRACT_FROZEN: YES
GROUP_SNAPSHOT_ENCODING: four SA-flattened records in A/B/C/D order
GROUP_SNAPSHOT_STORAGE_BITS: 816
CANDIDATE_PRODUCTION_MICROARCH: SHARED_ANALYZER_SEQUENTIAL
SHARED_ANALYZER_INSTANCE_COUNT: 1
CANDIDATE_PRODUCTION_REQUESTS: 16 (4 SA × 4 stored actions)
CANDIDATE_GENERATION_CYCLES: 16 capture cycles after producer acceptance
CANDIDATE_MAP_BITS: 480
```

SYN-A has one collector snapshot input and requires its external owner to present the snapshot selected by `current_sa_o`; it contains no four-SA snapshot storage. SYN-B therefore captures four distinct logical snapshots in its new top. It does not duplicate one SA's snapshot across all DFS depths.

The producer reuses `dss_v2_group_slot_decode` for every SA/action ConfigID and `recam_shared_config_analyzer` for the full ten-bit bitmap. Storage is L/R/B/RB, while search priority remains R/L/RB/B. The directed zero-snapshot producer trace reaches `23,23,23,23` valid slots and finishes after 16 requests.

```text
CONFIG_ACTION_MAPPING_MISMATCHES: 0
CANDIDATE_VALID_MISMATCHES: 0
CANDIDATE_EFFECT_MISMATCHES: 0
CANDIDATE_MAP_MISMATCHES: 0
GROUP_SNAPSHOT_ALIAS_ERRORS: 0
```
