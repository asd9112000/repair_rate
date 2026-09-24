# Archive policy

`dss_final/` preserves immutable experiment packages rather than serving as a
development RTL tree.

| Lifecycle | Meaning |
|---|---|
| `FINAL` | Verified, synthesized, self-contained canonical case used for the current comparison. |
| `SUPERSEDED` | Historically valid implementation replaced by a corrected canonical case. |
| `HISTORICAL` | Prototype or experiment evidence that is not part of the final canonical comparison. |

Every FINAL package contains `CASE.md`, `README.md`, `src/`, `verification/`,
and `synthesis/`. It has no symlink dependency on mutable `rtl/` source; its
synthesis source hash corresponds to archived source. Historical evidence is
preserved and is never silently overwritten.

The authoritative lifecycle list is [INDEX.md](../INDEX.md). Corrected final
PPA belongs only in [N2_HARDWARE_SUMMARY.md](../N2_HARDWARE_SUMMARY.md).
