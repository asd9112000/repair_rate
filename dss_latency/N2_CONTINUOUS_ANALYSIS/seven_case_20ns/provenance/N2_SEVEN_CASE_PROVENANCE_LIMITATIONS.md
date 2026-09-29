# Seven-case provenance limitations

FULL provenance: RECAM and both G2X2-RC CA-LIVE rows retain per-run source
manifest/hash sidecars with raw reports. RECONSTRUCTED provenance: none.

PARTIAL provenance: G2X2-R EARLY/GROUP and L1X4-R EARLY/GROUP. Their raw DC
reports, matched runner, constraints, and pre-synthesis freeze evidence remain
available. However, each result tree lacks its original
`functional_source_manifest.txt`, `functional_source_sha256.txt`, and DC log;
no contemporaneous Git revision is recorded. GROUP additionally lacks frozen
hash coverage for every runner input. Exact synthesis-source hash identity
cannot therefore be independently re-established and is not claimed.

Raw PPA reports remain research evidence because their identity, methodology,
tool, library, constraints, paths, and numerical outputs are retained. A PPA
number can be retained as experiment evidence **does not mean** exact historical
source-hash match can be claimed. Prohibited claims: `SOURCE_HASH_MATCH=PASS`,
FULL provenance, or equivalence to a missing original manifest for these four
rows.

```text
DATASET_RESEARCH_USABLE: YES
SOURCE_PROVENANCE_COMPLETE: NO
PROVENANCE_STATUS: PARTIAL_WITH_DOCUMENTED_LIMITATION
```
