# G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg mapping audit

Status: `PROVENANCE_BACKED_COMPOSITE_ABLATION`.

The dense map contract was recovered from SYN-B source revision
`93f6384002029f97fb47106847367ad379316a59` and closure `c81e733`:

```text
index = SA * 40 + stored_action * 10 + (PatternID - 1)
stored_action: L=0, R=1, B=2, RB=3
candidate_valid/release/borrow: 3 * 160 registered bits = 480 bits
```

The new producer retains that representation and schedule (four SA times four
stored actions) but evaluates ConfigID 0 through 6 concurrently for the active
SA. The selected lane is the current frozen canonical GROUP slot map:

| SA role | L | R | B | RB |
| --- | ---: | ---: | ---: | ---: |
| A (0) | 0 | 4 | 5 | 6 |
| B (1) | 0 | 1 | 2 | 3 |
| C (2) | 0 | 1 | 2 | 3 |
| D (3) | 0 | 4 | 5 | 6 |

This is derived directly from `dss_v2_group_slot_decode.sv`; it is not an
invented action map. Every lane's 10-bit `candidate_valid_o` is live through a
role/action-selected map write. For its selected slot, each valid pattern bit
is copied unchanged, while `release` and `borrow` replicate the recovered
transition-adapter effect and are ANDed with that valid bit.

The adapter preserves SYN-B resource semantics: A releases resource 0/borrows
2; B releases 2/borrows 1; C releases 3/borrows 0; D releases 1/borrows 3.
Descriptors are the selected canonical config dimensions.

Boundary indices are therefore 0, 39, 40, 79, 80, 119, 120, and 159 for the
first/last slots of SA0 through SA3. ## Canonicalization for controlled ablation

`ROLE_CONFIG_CONTRACT_SOURCE: dss_v2_group_slot_decode.sv`. The frozen source
contract is SA0/SA3 `{0,4,5,6}` and SA1/SA2 `{0,1,2,3}`.

SYN-B storage order remains `L,R,B,RB` and its historical DFS priority remains
`R,L,RB,B`. `HISTORICAL_DFS_PRIORITY_USED_FOR_FINAL_ABLATION: NO`: seed 1
proved it selects a different externally visible ConfigID/action than the
frozen DATE2026 policy. The final ablation derives first ascending PatternID
per dense-map slot and applies the exact canonical `path_valid[0..80]` priority
using `recam_n2_preopt_canonical_path_selector`. The complete machine-readable
path universe is `G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg_CANONICAL_PATH_PRIORITY.csv`.

The historical OPT1/DFS and atomic-commit blocks remain instantiated as
provenance hardware/diagnostic paths; canonical path selection controls the
externally visible tuple. Action equivalence is checked by inverting the frozen slot/Config
map in the lockstep wrapper. The atomic result is diagnostic-only because its
historical legality policy can reject a frozen-canonical legal tuple.
