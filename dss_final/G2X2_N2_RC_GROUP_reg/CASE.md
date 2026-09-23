# G2X2 N2 RC GROUP corrected final archive

`LIFECYCLE: FINAL`

The policy is the frozen directional HYP02 GROUP/static-global implementation.
The correction changes physical address representation only: rows are 9 bits,
physical repair columns are 13 bits, logical word columns remain 5 bits, and
hybrid line addresses are 13 bits. Five fixed pivot slots per SA retain 440
address-state bits across the four-SA GROUP. Retention is passive during
analysis and selection and is consumed only for final reconstruction.

`src/` is self-contained. Functional and synthesis source hashes match the
archived source hashes in `synthesis/`.

## Lifecycle

```text
LIFECYCLE: FINAL
FUNCTIONAL_EVIDENCE: VALID
PHYSICAL_ADDRESS_RTL: VALID
PPA_EVIDENCE: VALID
```
