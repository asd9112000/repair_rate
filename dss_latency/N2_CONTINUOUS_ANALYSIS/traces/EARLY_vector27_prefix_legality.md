# EARLY vector-27 prefix-legality regression

For SA D, CFG4 is analyzer-invalid. CFG0/Pattern1 is analyzer-valid but is
rejected because B has borrowed and CFG0 does not release. CFG6/Pattern5 is
analyzer-valid and prefix-compatible, so it is accepted and committed once.
This trace is the permanent regression for the CA acceptance condition.
