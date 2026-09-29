# N2 CA-LIVE analyzer-width provenance

## Frozen conclusion

The historical V2 summary boundary is 204 bits.  The current N2 canonical
analyzer payload is 272 bits.  The interfaces have the same logical field
schema, but the current payload is a physical-address-width superset.  No
CA-LIVE payload field or payload register was introduced.

| Field group | Historical V2 | Current N2 canonical | Delta |
| --- | ---: | ---: | ---: |
| Five pivot columns | 5 x 5 = 25 | 5 x 13 = 65 | +40 |
| Seven Hybrid differing addresses | 7 x 9 = 63 | 7 x 13 = 91 | +28 |
| All other analyzer inputs | 116 | 116 | 0 |
| Total | 204 | 272 | +68 |

```text
204 + 68 = 272
```

The current G2X2-R canonical wrapper and shared analyzer declare
`PHYS_COL_ADDR_W=13` and `HYBRID_LINE_ADDR_W=13`.  These fields existed in
the frozen canonical source before the CA-LIVE sibling.  CA-LIVE passes the
same ports directly into the same analyzer, adding only scheduling ports.

```text
CA_LIVE_NEW_PAYLOAD_FIELDS: NONE
CA_LIVE_NEW_PAYLOAD_REGISTER_BITS: 0
RELATIONSHIP: SUBSET_SUPERSET
```

The `204-bit/7-Hybrid` reference in the current canonical wrapper is a
`MISAPPLIED_HISTORICAL_COMMENT`: the real historical source is
`rtl/dss_v2/top/recam_dss_v2_early_overlap_core.sv`, whose
`summary_payload_i` is explicitly `[203:0]` and uses 5-bit column and 9-bit
differing-address fields.  It must not be used to truncate the current
13-bit physical-address payload.
