# H1 — RS=CS=3, SHARE_M=1 Legal Configuration-Space Freeze

> 文件狀態：Complete
> 適用範圍：2x2 Directional CAM DSS V2-derived target configuration semantics
> 建立時間：2026-09-13T06:51:30+08:00
> 最後修改時間：2026-09-13T06:51:30+08:00

## Frozen inputs and independent derivation

For A/D, R is in [RS-m, RS] and C is in [CS, CS+m]. For B/C,
R is in [RS, RS+m] and C is in [CS-m, CS]. Substitution of RS=CS=3 and
m=1 gives:

~~~
A/D = {2,3} × {3,4}
B/C = {3,4} × {2,3}
~~~

This is a Cartesian derivation from availability equations, not a renamed
seven-ConfigID table.

## Role-specific action table

| Role | R | C | ΔR | ΔC | Action class | Release | Borrow | Local? |
|---|---:|---:|---:|---:|---|---|---|---:|
| A/D | 3 | 3 | 0 | 0 | LOCAL | none | none | Yes |
| A/D | 2 | 3 | -1 | 0 | RELEASE_ONLY | own row: A_ROW/D_ROW | none | No |
| A/D | 3 | 4 | 0 | +1 | BORROW_ONLY | none | B_COL then C_COL / C_COL then B_COL | No |
| A/D | 2 | 4 | -1 | +1 | RELEASE_AND_BORROW | own row: A_ROW/D_ROW | same directional column donor priority | No |
| B/C | 3 | 3 | 0 | 0 | LOCAL | none | none | Yes |
| B/C | 3 | 2 | 0 | -1 | RELEASE_ONLY | own column: B_COL/C_COL | none | No |
| B/C | 4 | 3 | +1 | 0 | BORROW_ONLY | none | A_ROW then D_ROW / D_ROW then A_ROW | No |
| B/C | 4 | 2 | +1 | -1 | RELEASE_AND_BORROW | own column: B_COL/C_COL | same directional row donor priority | No |

All four action classes are legal and sufficient for each role. Every entry
has at most one release and at most one borrow.

## Unique physical configuration union

| Semantic name | R | C | K | A/D legal | B/C legal | A/D action | B/C action | Transpose partner | Self-transpose |
|---|---:|---:|---:|---:|---:|---|---|---|---:|
| LOCAL_3R3C | 3 | 3 | 6 | Yes | Yes | LOCAL | LOCAL | itself | Yes |
| AD_RELEASE_2R3C | 2 | 3 | 5 | Yes | No | RELEASE_ONLY | — | BC_RELEASE_3R2C | No |
| AD_BORROW_3R4C | 3 | 4 | 7 | Yes | No | BORROW_ONLY | — | BC_BORROW_4R3C | No |
| AD_RELEASE_BORROW_2R4C | 2 | 4 | 6 | Yes | No | RELEASE_AND_BORROW | — | BC_RELEASE_BORROW_4R2C | No |
| BC_RELEASE_3R2C | 3 | 2 | 5 | No | Yes | — | RELEASE_ONLY | AD_RELEASE_2R3C | No |
| BC_BORROW_4R3C | 4 | 3 | 7 | No | Yes | — | BORROW_ONLY | AD_BORROW_3R4C | No |
| BC_RELEASE_BORROW_4R2C | 4 | 2 | 6 | No | Yes | — | RELEASE_AND_BORROW | AD_RELEASE_BORROW_2R4C | No |

~~~
NUM_UNIQUE_CONFIGS = 7
NUM_ROLE_SLOTS_AD = 4
NUM_ROLE_SLOTS_BC = 4
R_MAX = 4
C_MAX = 4
K_MAX = 7
~~~

## Candidate-count derivation

For each configuration the candidate count is C(K,R), with K=R+C.

| R | C | K | C(K,R) |
|---:|---:|---:|---:|
| 3 | 3 | 6 | 20 |
| 2 | 3 | 5 | 10 |
| 3 | 4 | 7 | 35 |
| 2 | 4 | 6 | 15 |
| 3 | 2 | 5 | 10 |
| 4 | 3 | 7 | 35 |
| 4 | 2 | 6 | 15 |

The maximum valid one-based PatternID is 35. Zero remains invalid, so the
value range is 0..35 and requires ceil(log2(36)) = 6 bits. The valid-candidate
bitmap has one bit per possible candidate at the largest class.

~~~
MAX_CANDIDATE_COUNT = 35
PATTERN_ID_W_REQUIRED = 6
CANDIDATE_BITMAP_W_REQUIRED = 35
~~~

## Transpose strategy

The mathematical canonical classes are 3R3C, 3R2C/2R3C,
4R3C/3R4C, and 4R2C/2R4C: four total classes.

Transpose preserves candidate count. It can preserve candidate ordering only
when the target ConfigID table defines a canonical pattern order and a
role-aware inverse mapping for PatternID reconstruction. It also requires H2
to design explicit swaps for Must-row/Must-column semantics and Hybrid
descriptor normalization. No transpose RTL is selected or implemented here.

~~~
CANONICAL_ANALYZER_CLASS_COUNT = 4
~~~

## Frozen policy-order requirements

Order is stored as action semantics, never inferred from a new numeric ID:

| Policy | A/D required action order | B/C required action order |
|---|---|---|
| EARLY | LOCAL → RELEASE_ONLY → BORROW_ONLY → RELEASE_AND_BORROW | LOCAL → BORROW_ONLY → RELEASE_ONLY → RELEASE_AND_BORROW |
| GROUP-NoScratch | RELEASE_ONLY → LOCAL → RELEASE_AND_BORROW → BORROW_ONLY | RELEASE_ONLY → LOCAL → RELEASE_AND_BORROW → BORROW_ONLY |

These are the frozen 2,2,1 V2 semantic orders mapped onto corresponding 3,3,1
action classes. H2 may choose a control encoding, but cannot embed a different
order accidentally through ConfigID numbering.

## Physical conservation and ledger compatibility

The four-SA base physical budget is:

~~~
physical_spares_per_SA = 3 + 3 = 6
physical_spares_per_group = 4 × 6 = 24
~~~

A release marks one already-physical own line eligible for redistribution.
A borrow consumes one such released donor line. Neither adds a line; a
release-and-borrow action exchanges eligibility/ownership and does not create
new physical capacity.

Every legal envelope needs at most one own release and one directional borrow.
The donor is one of the same four resources A_ROW, D_ROW, B_COL, C_COL, and
each remains single-owner. Therefore the frozen ledger is compatible.

~~~
LEDGER_REUSE_CONFIRMED = YES
H0_LEDGER_REUSE_ASSUMPTION_INVALIDATED = NO
~~~

## Configuration representation decision

| Option | Evaluation |
|---|---|
| A. New global fixed CFG0..CFG6 meanings | Rejected: silently changes frozen 2,2,1 meanings. |
| B. Architecture-point ConfigID index + descriptor/action/transpose table | Preferred: distinct versioned tables preserve old interpretation and provide explicit target metadata. |
| C. Action-only role descriptor | Insufficient alone: it derives R/C at m=1 but is not enough for analyzer pattern ordering, transpose, or result interpretation. |

The H2 recommendation is Option B with an architecture/config-table version in
every interpretable result. A target ConfigID may numerically reuse 0..6 only
inside a declared 3,3,1 table context; it must never globally redefine frozen
2,2,1 ConfigIDs.

~~~
CONFIG_REPRESENTATION_RECOMMENDATION =
  VERSIONED_ARCHITECTURE_POINT_CONFIG_TABLE
  {ConfigID, R_count, C_count, role applicability, action, transpose metadata}
~~~

## H2 inputs and phase control

~~~
NUM_UNIQUE_CONFIGS = 7
NUM_ROLE_SLOTS_AD = 4
NUM_ROLE_SLOTS_BC = 4
R_MAX = 4
C_MAX = 4
K_MAX = 7
MAX_CANDIDATE_COUNT = 35
PATTERN_ID_W_REQUIRED = 6
CANDIDATE_BITMAP_W_REQUIRED = 35
CANONICAL_ANALYZER_CLASS_COUNT = 4
CONFIG_REPRESENTATION_RECOMMENDATION = VERSIONED_ARCHITECTURE_POINT_CONFIG_TABLE
LEDGER_REUSE_CONFIRMED = YES

H0S0-CL-004: BLOCKS_H1=NO, BLOCKS_S1=YES, BLOCKS_E0=YES, BLOCKS_E1=YES
NEXT_PHASE_AUTHORIZED = NONE
~~~

No RTL, simulator semantics, golden fixture, synthesis, or experiment was
modified or executed.
