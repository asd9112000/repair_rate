# N2 Continuous-Analysis Live-State / State-Duplication Audit

## Scope

This is a source-and-existing-report audit only.  It does not modify RTL,
rerun synthesis, or alter the frozen canonical cases.

## Finding

`EARLY-CA-SB` and `GROUP-CA-SB` each instantiate a four-SA captured
analyzer-input bank in their top wrapper.  Each SA record is 272 bits:

| Field | Bits per SA | Four-SA bits |
| --- | ---: | ---: |
| `pivot_valid` | 5 | 20 |
| `pivot_rows_flat` | 45 | 180 |
| `pivot_cols_flat` | 65 | 260 |
| six row/column threshold vectors | 30 | 120 |
| `hybrid_valid` | 7 | 28 |
| `hybrid_pointer_flat` | 21 | 84 |
| `hybrid_descriptor` | 7 | 28 |
| `hybrid_differing_flat` | 91 | 364 |
| `conventional_overflow` | 1 | 4 |
| **Total** | **272** | **1088** |

The banks write on `state_update_i`, indexed by `state_sa_i`, and the
analyzer reads the selected `scan_sa` bank.  No distinct `state_update_i` or
`state_sa_i` register is introduced by either wrapper.  DC reports exactly
1088 top-level registers in each CA wrapper.

The data is the analyzer-facing projection of persistent fault/CAM state.  It
is not an additional repair semantic.  The canonical N2 synthesis tops expose
the same fields as input ports and contain no internal collector; therefore
the CA experiments moved four copies of an external state projection across
their synthesis boundary.

## Fault-update priority

EARLY suppresses a scan completion when an update names the active SA:
`!(state_update_i && scan_sa_q == state_sa_i)`.  Therefore its old candidate
is not accepted on that edge.

GROUP uses the same suppression for FSM advancement and clears the affected
`frozen_q` bit.  However its candidate-store write enable is only
`state_q == SCAN`.  An update for the active SA can consequently write the
old analyzer result into the current slot on the update edge.  The slot is
overwritten by the restarted CFG0 scan and cannot become a final selection
while `frozen_q` is clear, but this does not meet the literal requirement that
the old result not be captured on that edge.  A LIVE_STATE implementation must
give the update condition priority at the candidate-store write boundary.

## GROUP persistent state

The existing candidate store is exactly 80 bits: four SAs times four canonical
slots times `{PatternID[3:0], valid}`.  The offset is
`(SA * 4 + slot) * 5`; `valid` is the low bit and PatternID occupies the next
four bits.  Every scan slot writes a complete record, including zero for an
analyzer-invalid candidate.  With an explicit per-SA latest-generation
complete/frozen state, restarting at CFG0 and overwriting all four slots does
not require clearing the 80-bit store or adding generation tags.

The existing 440-bit `dss_group_pivot_address_regs` is separate.  The store
does not contain physical pivot addresses, while final Pattern-dependent line
reconstruction uses the retained 4 x 5 row/column pivot addresses.  Its
necessity is unchanged by this audit.

## Area boundary

| Case | Canonical cell area (um2) | CA-SB cell area (um2) | Delta (um2) |
| --- | ---: | ---: | ---: |
| EARLY | 102296.780 | 222576.079 | 120279.299 |
| GROUP | 166140.377 | 289948.986 | 123808.609 |

Each CA wrapper has 1088 state-bank registers.  Its local area is
118672.647 um2 (EARLY) or 113057.684 um2 (GROUP), dominated by the 1088-bit
bank and the wide four-way state-selection cone.  The reports show one shared
analyzer in every canonical and CA case; analyzer replication is not the
cause.

## Classification and LIVE_STATE decision

The synthesized variants are retained evidence and are classified as
`CONTINUOUS_ANALYSIS_STATEBANK_PROTOTYPE` (`EARLY-CA-SB`, `GROUP-CA-SB`).

The lean architecture is feasible in principle: live authoritative fault/CAM
state drives one shared analyzer; an update preempts the active scan; EARLY
holds the accepted analyzer output; and GROUP uses the existing 80-bit map
plus completion/freeze control.  The repository does not provide an integrated
BIST scheduler or live collector-to-CA top, so whether BIST completion is
sticky and whether it serializes fault updates remains an integration contract
that must be confirmed before RTL work.

Proposed future variants, not implemented by this audit:

- `G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg` (`EARLY-CA-LIVE`)
- `G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg` (`GROUP-CA-LIVE`)

They must preserve the analyzer, configuration priority, PatternID semantics,
prefix legality, 81-path selector, repair semantics, and address widths.
