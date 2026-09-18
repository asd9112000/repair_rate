# Canonical Naming Map

> Status: authoritative P1-NAME reference (2026-09-18).
>
> Historical source files, modules, result directories, and immutable datasets
> retain their original names.  This map controls new C++/CSV-facing labels
> and the documentation vocabulary used for active work.

## Policy and implementation map

| Canonical semantic name | Canonical RTL name | C++ canonical label | Historical aliases / implementation | Status | Safe usage |
|---|---|---|---|---|---|
| Normalized Local-First | No active canonical RTL; a controller correction remains distinct from historical V2 EARLY | `normalized_local_first` | `directional_v2_early`, `directional_m1_v2_early`; historical V2 EARLY | historical C++ semantic label | Use for the frozen directional-V2 local-first C++ policy only. Do not call its historical streaming RTL canonical. |
| Normalized Streaming EARLY | `canonical_streaming_early` | `normalized_streaming_early` | `group_greedy_rtl_canonical`, `directional_m1_early`; V2 GROUP controller is provenance/reference only | active | Use for the normalized streaming policy and the canonical RTL implementation. |
| Normalized EARLY Deferred | No active canonical RTL | `normalized_early_deferred` | `group_no_scratch_v2`; V2 GROUP-NoScratch | historical | Use when the deferred candidate-history policy is specifically meant. |
| Normalized GLOBAL | `canonical_global_noscratch` today; `canonical_global_withscratch` is future work | `normalized_global` | `directional_v2_group_global_canonical`, `directional_m1_global_canonical` | active semantic policy | Use for the complete tuple search with exact future-donor release obligations. |
| Normalized GLOBAL-NoScratch | `canonical_global_noscratch` | hardware/report label `normalized_global_noscratch` | no historical exact RTL equivalent | active | Hardware storage realization of Normalized GLOBAL; not a different C++ policy. |
| Normalized GLOBAL-WithScratch | `canonical_global_withscratch` | future hardware/report label `normalized_global_withscratch` | none | future, not started | Reserve for a future storage realization; P1-NAME does not implement it. |
| Historical Noncanonical Streaming EARLY | `recam_dss_v2_early_core` and related historical V2 RTL | historical module/result name | V2 EARLY; `directional_v2_early` as a compatibility input | historical provenance | Preserve verbatim in historical RTL, result names, and provenance records. It is not Normalized Streaming EARLY. |
| Historical Noncanonical EARLY Legacy | Phase 3J historical EARLY RTL | historical module/result name | Phase 3J EARLY | historical provenance | Preserve verbatim; cite as a noncanonical legacy policy. |
| Normalized EARLY Legacy | Phase 3J historical GROUP RTL | historical module/result name | Phase 3J GROUP | historical provenance | Preserve verbatim; use only with its decision/resource-boundary qualification. |
| Historical directional-V2 GLOBAL oracle | no canonical RTL | `historical_directional_v2_global` | `directional_v2_group_global`, `directional_m1_v2_group_global` | historical C++ provenance | Keep readable and replayable. It is distinct from `normalized_global`: it lacks the frozen explicit future-donor release-obligation rule. |

## Semantic policy versus hardware storage

`normalized_global` names the C++ policy contract.  `normalized_global_noscratch`
and `normalized_global_withscratch` name hardware storage realizations of that
policy.  They must not be interchanged in repair-rate comparisons or synthesis
area claims.

## Input/output compatibility

New C++/CSV output uses the `normalized_*` labels above, except that the
historical directional-V2 GLOBAL oracle emits `historical_directional_v2_global`
to prevent a semantic collision.  The legacy CLI aliases remain accepted in
`DynamicSpareSharing`; its canonical `normalized_global` input selects the
separate release-obligation oracle.  The hierarchical and SRAM entry points
accept their supported normalized aliases (`normalized_streaming_early` and
`normalized_early_deferred`) alongside the historical names.

Frozen CSV, corpus, and synthesis evidence are immutable provenance and are
not relabelled by this map.
