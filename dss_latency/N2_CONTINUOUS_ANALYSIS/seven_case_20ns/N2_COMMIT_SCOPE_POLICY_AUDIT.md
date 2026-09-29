# Commit-scope policy audit

`AGENTS.md` requires experiment integrity, minimal evidence-backed changes,
and relevant validation; it does not prescribe automatic inclusion/exclusion
of `my_note/`, `01_my_note/`, or generic `scripts/` changes. The external
artifact manifest notes that `01_my_note/` can contain frozen research
artifacts, but does not make all such contents part of this experiment.

Therefore permanent scripts that reproduce this N2 CA-LIVE experiment belong
only when they are actually used by this experiment. The archived matched DC
Tcl is such a required script. The currently modified group repair-rate
scripts are not tied to this seven-case 20 ns evidence and must be excluded.

The tracked deletions under `my_note/` have no demonstrated relationship to
this experiment. They require user review before any separate commit; they
are not silently staged here. No modified/untracked `01_my_note/` path appears
in the current `git status --short` inventory.
