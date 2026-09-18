# P3 four-point synthesis boundary audit

The required four synthesis points are not currently boundary-comparable. This
audit is recorded before any final synthesis, as required.

| Point | Collector/analyzer | Candidate generation | Policy | Topology | Selected tuple | Committed ledger | Boundary comparable? |
|---|---|---|---|---|---|---|---|
| SYN-A 2x2 Streaming EARLY | present in existing canonical RS2 top | shared analyzer | NORMALIZED_STREAMING_EARLY | 2x2 directional feasibility | present | present | reference integrated boundary |
| SYN-B 2x2 GROUP_GLOBAL OPT1 | absent | external 160x3 map only | canonical GLOBAL search core / OPT1 filter | encoded inside search core | present | absent | NO: search-core boundary only |
| SYN-C 1x4 Streaming EARLY | no RTL | no RTL | C++ policy only | C++ neighbor ledger | C++ only | C++ only | NO: no hardware point |
| SYN-D 1x4 GROUP_GLOBAL | no RTL | no RTL | C++ policy only | C++ neighbor ledger | C++ only | C++ only | NO: no hardware point |

The preferred comparable boundary remains:

```text
collector / RECAM state
-> candidate generation / analyzer
-> policy
-> topology/resource feasibility
-> selected solution
-> committed physical ledger
```

Neither a 2x2 integrated GLOBAL top nor an RTL 1x4 Single-Hop baseline exists.
Synthesizing present points would compare an integrated EARLY top with a
search-only GLOBAL core and C++-only policies, which would be misleading.

```text
ALL_BOUNDARIES_COMPARABLE: NO
P3_4PT_SYNTH: BLOCKED
```
