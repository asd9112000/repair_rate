# P3 four-point GLOBAL OPT1 statistics

These measurements characterize pre-DFS candidate reduction. They are not
area, timing, latency, or cross-topology-capacity comparisons.

## Candidate counts per four-SA group

| Architecture / corpus | Raw mean | median | P95 | P99 | max | OPT1 mean | median | P95 | P99 | max | Reduction |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| SYN-B, 1,000 seeded RTL candidate maps (`20260918`, 18% valid-slot sampling) | 28.830 | 29 | 36 | 40 | 46 | 19.338 | 19 | 23 | 25 | 27 | 0.329 |
| SYN-D, 1,000 shared C++ oracle groups (`20260921`) | 13.071 | 13 | 19 | 21 | 24 | 7.833 | 8 | 11 | 12 | 13 | 0.401 |

SYN-B counts use raw valid slots and its three-effect class key. SYN-D counts
use C++ source candidates and unique `(usedRows, usedColumns)` demands. The
absolute values must not be compared as front-end capacity: the two topologies
have different candidate interfaces, action families, and resource semantics.

## Search candidate visits

| Architecture / model / corpus | Mode | aggregate | mean | median | P95 | P99 | max |
|---|---|---:|---:|---:|---:|---:|---:|
| SYN-B C++ DFS audit model, 1,000 maps (`20260918`, 12% valid-slot sampling) | OPT0 exhaustive | 19,126 | 19.126 | 7 | 72 | 184 | 736 |
| SYN-B C++ DFS audit model, same corpus | OPT1 three-effect collapse | 12,151 | 12.151 | 6 | 42 | 90 | 187 |
| SYN-D RTL core / shared C++ oracle, 1,000 groups (`20260921`) | OPT0 exhaustive | 2,047,022 | 2,047.022 | 1,485 | 5,871 | 9,097 | 15,430 |
| SYN-D RTL core / shared C++ oracle, same corpus | OPT1 equal-demand collapse | 555,831 | 555.831 | 495 | 1,215 | 1,530 | 2,163 |

The SYN-B visit study has a deliberately different 12%-valid audit corpus
than its 18%-valid candidate-count corpus; its distributions must not be
paired numerically. It is retained only as historical search-model evidence.
The SYN-D visit pair uses the same 1,000-case shared C++ corpus and reports
the RTL `candidate_evaluations_o` counter for each separate elaboration.

A candidate visit is a search-model/DFS candidate evaluation. It is not
automatically an RTL cycle. The current SYN-D FSM increments its counter once
per search cycle, but this audit makes no timing or PPA inference from that
implementation detail. SYN-B figures are from a C++ audit model and must not
be interpreted as RTL cycles.
