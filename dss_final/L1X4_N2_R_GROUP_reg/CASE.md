# L1X4 N2 R GROUP corrected final archive

`LIFECYCLE: FINAL`

The policy is the frozen directed `A -> B -> C -> D` row-only
GROUP/static-global implementation. `ROW_ADDR_W=9`, `PHYS_COL_ADDR_W=13`,
`WORD_COL_ADDR_W=5`, and `HYBRID_LINE_ADDR_W=13`. Five pivot slots per SA
retain 440 address-state bits over the four-SA GROUP. Retention is passive
during analysis and selection and is consumed only during final reconstruction.

`src/` is self-contained. Functional and synthesis source hashes match the
archived source hashes in `synthesis/`.

## Lifecycle

```text
LIFECYCLE: FINAL
FUNCTIONAL_EVIDENCE: VALID
PHYSICAL_ADDRESS_RTL: VALID
PPA_EVIDENCE: VALID
```
